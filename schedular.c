#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdbool.h>
#include <signal.h>

#define MAX_TASKS 100

// Shared data structure
struct SharedData {
    int ncpu;      // Number of CPUs
    int tslice;    // Time slice
};

struct CommandInfo {
    pid_t pid;
    struct timeval start_time;
    struct timeval end_time;
};

// Task data structure
struct Task {
    char command[100];
    int status; // 0: Not started, 1: Running, 2: Completed
    pid_t pid;   // Process ID of the running task
};

// Global variables to track scheduling
struct Task task_queue[MAX_TASKS];
int task_count = 0;

// Signal handler function for Ctrl+C (SIGINT)
void sigint_handler(int signo) {
    if (signo == SIGINT) {
        printf("Received Ctrl+C. Exiting...\n");
        exit(0);  // Exit gracefully when Ctrl+C is received
    }
}

void execute_command(const char *command, struct CommandInfo *cmd_info) {
    // Record start time
    gettimeofday(&cmd_info->start_time, NULL);

    pid_t pid = fork();
    if (pid == -1) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Child process
        char *args[32]; // You can adjust the size as needed
        int arg_count = 0;
        char *token = strtok((char *)command, " ");

        for (; token != NULL && arg_count < 32; token = strtok(NULL, " ")) {
            args[arg_count++] = token;
        }
        args[arg_count] = NULL; // Null-terminate the argument list

        if (execvp(args[0], args) == -1) {
            perror("Error executing command");
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent process
        waitpid(pid, NULL, 0);

        // Record end time
        gettimeofday(&cmd_info->end_time, NULL);

        // Record the process ID for this command
        cmd_info->pid = pid;
    }
}

// Function to schedule a task in round-robin
void schedule_round_robin(const char *user_input) {
    if (task_count < MAX_TASKS) {
        struct CommandInfo cmd_info;
        execute_command(user_input, &cmd_info);
        strcpy(task_queue[task_count].command, user_input);
        task_queue[task_count].status = 2; // Mark the task as completed
        task_queue[task_count].pid = cmd_info.pid; // Store the process ID
        task_count++;
    } else {
        printf("Task queue is full. Cannot schedule more tasks.\n");
    }
}

int main() {
    // Set up the signal handler for Ctrl+C
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        perror("Failed to set up signal handler");
        exit(1);
    }

    int shmid;
    struct SharedData *shdata;
    key_t key = 1234;  // Change this key to a unique value

    // Create the shared memory segment
    if ((shmid = shmget(key, sizeof(struct SharedData), 0666 | IPC_CREAT)) < 0) {
        perror("shmget");
        exit(1);
    }

    // Attach the shared memory segment
    if ((shdata = shmat(shmid, NULL, 0)) == (struct SharedData *) -1) {
        perror("shmat");
        exit(1);
    }

    while (1) {
        if (task_count > 0) {
            // Check if there are tasks in the queue
            // Execute tasks based on available CPUs and time slice
            for (int i = 0; i < shdata->ncpu; i++) {
                if (i < task_count) {
                    if (task_queue[i].status == 0) {
                        // Task is not started
                        task_queue[i].status = 1; // Mark the task as running
                        execute_command(task_queue[i].command, &task_queue[i]);
                        printf("Scheduled task: %s\n", task_queue[i].command);
                    }
                }
            }
            // Sleep for the time slice
            sleep(shdata->tslice);
            // Update the task status and remove completed tasks
            int new_task_count = 0;
            for (int i = 0; i < task_count; i++) {
                if (task_queue[i].status == 1) {
                    // Check if the task is still running
                    int status;
                    if (waitpid(task_queue[i].pid, &status, WNOHANG) == 0) {
                        // Task is still running
                        task_queue[new_task_count] = task_queue[i]; // Move the task to the new list
                        new_task_count++;
                    } else {
                        // Task has completed
                        task_queue[i].status = 2; // Mark the task as completed
                    }
                }
            }
            task_count = new_task_count; // Update the task count
        }
    }

    // Detach the shared memory segment
    shmdt(shdata);

    return 0;
}