#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#include <dirent.h> // For directory listing
#include <fcntl.h>  // For file-related operations

#define MAX_HISTORY_SIZE 100
#define MAX_PIPE_SEGMENTS 10

char history[MAX_HISTORY_SIZE][100];
int history_count = 0;

struct CommandInfo {
    pid_t pid;
    struct timeval start_time;
    struct timeval end_time;

};

// Function to calculate the duration of a command in seconds
double calculate_duration(const struct CommandInfo *cmd_info) {
    return (cmd_info->end_time.tv_sec - cmd_info->start_time.tv_sec) +
           (cmd_info->end_time.tv_usec - cmd_info->start_time.tv_usec) / 1.0e6;
}


// Function to display command details
void display_command_info(const struct CommandInfo *cmd_info) {
    double duration = calculate_duration(cmd_info);
    // printf("Process ID: %d\n", cmd_info->pid);
    // printf("Start Time: %ld.%06ld seconds since epoch\n", cmd_info->start_time.tv_sec, cmd_info->start_time.tv_usec);
    // printf("End Time: %ld.%06ld seconds since epoch\n", cmd_info->end_time.tv_sec, cmd_info->end_time.tv_usec);
    // printf("Duration: %.6f seconds\n", duration);
}

int execute_cd(const char *path) {
    if (chdir(path) == -1) {
        perror("Error changing directory");
        return 1;
    }
    return 0;

}

// ... (Other command execution functions)

void execute_command(const char *command, struct CommandInfo *cmd_info) {
    // Record start time
    gettimeofday(&cmd_info->start_time, NULL);

    // Split command into pipe segments
    int pipe_count = 0;
    char *pipe_commands[MAX_PIPE_SEGMENTS];
    char *token = strtok((char *)command, "|");

    while (token != NULL && pipe_count < MAX_PIPE_SEGMENTS) {
        pipe_commands[pipe_count++] = token;
        token = strtok(NULL, "|");
    }

    int pipes[2];
    int input_fd = 0; // Initial input is from stdin

    for (int i = 0; i < pipe_count; i++) {
        if (pipe(pipes) == -1) {
            perror("Pipe creation failed");
            exit(EXIT_FAILURE);
        }

        pid_t pid = fork();
        if (pid == -1) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            // Child process
            close(pipes[0]); // Close the read end of the pipe

            // Redirect input to the previous command's output (if not the first command)
            if (i != 0) {
                dup2(input_fd, STDIN_FILENO);
                close(input_fd);
            }

            // Redirect output to the pipe (for all commands except the last one)
            if (i < pipe_count - 1) {
                dup2(pipes[1], STDOUT_FILENO);
            }

            // Tokenize and execute the command using execvp
            char *args[32]; // You can adjust the size as needed
            int arg_count = 0;
            token = strtok(pipe_commands[i], " ");

            while (token != NULL && arg_count < 32) {
                args[arg_count++] = token;
                token = strtok(NULL, " ");
            }
            args[arg_count] = NULL; // Null-terminate the argument list

            if (execvp(args[0], args) == -1) {
                perror("Error executing command");
                exit(EXIT_FAILURE);
            }

        } else {
            // Parent process
            close(pipes[1]); // Close the write end of the pipe
            waitpid(pid, NULL, 0);

            // Set the input for the next command to be the read end of the pipe
            input_fd = pipes[0];
        }

    }

    // Record end time
    gettimeofday(&cmd_info->end_time, NULL);

    // Record the process ID for this command
    cmd_info->pid = getpid();
}

// Function to display command history
void display_history() {
    int s = 0;

    printf("Command History:\n");
    while (s < history_count) {
        printf("%d: %s\n", s + 1, history[s]);
        s++;
    }

}



int main() {
    int flag = 1;

    while (flag) {
        char user_input[100]; // Define a buffer to store user input
        printf("os_assignment2$ ");
        if (fgets(user_input, sizeof(user_input), stdin) == NULL) {
            // Handle end of input (e.g., Ctrl+D for Linux/Unix)
            break; // Exit the loop on end-of-input
        }
        user_input[strcspn(user_input, "\n")] = '\0';
        if (strcmp(user_input, "exit") == !flag) {
            // If the user enters "exit," break the loop to exit the program
            break;
        } else if (strcmp(user_input, "history") == !flag) {
            // If the user enters "history," display the command history
            display_history();
        } else {
            // Execute the user's command and add it to history
            struct CommandInfo cmd_info;
            execute_command(user_input, &cmd_info);
            if (history_count < MAX_HISTORY_SIZE) {
                strcpy(history[history_count], user_input);
                history_count++;
            }
            // Display command details
            display_command_info(&cmd_info);
        }

    }

    return 0;
}

