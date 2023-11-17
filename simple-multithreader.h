// simple-multithreader.h

#ifndef SIMPLE_MULTITHREADER_H
#define SIMPLE_MULTITHREADER_H

#include <functional>
#include <pthread.h>
#include <chrono>
#include <iostream>

// Placeholder for array_sum, replace with your actual implementation
int array_sum(int low, int high);

// Placeholder for matrix_sum, replace with your actual implementation
int matrix_sum(int start_row, int end_row, int start_col, int end_col);

struct ThreadData1D {
    int* array;
    int low;
    int high;
    int result;
};

struct ThreadData2D {
    int** matrix;
    int start_row;
    int end_row;
    int start_col;
    int end_col;
    int result;
};

std::chrono::high_resolution_clock::time_point get_current_time();

double calculate_elapsed_time(std::chrono::high_resolution_clock::time_point start,
                              std::chrono::high_resolution_clock::time_point end);

void* thread_func_1d(void* ptr);

void* thread_func_2d(void* ptr);

template <typename Func>
void parallel_for_1d(int low, int high, Func func, int numThreads);

template <typename Func>
void parallel_for_2d(int start_row, int end_row, int start_col, int end_col, Func func, int numThreads);

// Function to get the current time
std::chrono::high_resolution_clock::time_point get_current_time() {
    return std::chrono::high_resolution_clock::now();
}

// Function to calculate the elapsed time in milliseconds
double calculate_elapsed_time(std::chrono::high_resolution_clock::time_point start,
                              std::chrono::high_resolution_clock::time_point end) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

// Thread function for a 1D loop
void* thread_func_1d(void* ptr) {
    auto start_time = get_current_time();
    ThreadData1D* t = static_cast<ThreadData1D*>(ptr);
    t->result = array_sum(t->low, t->high);
    auto end_time = get_current_time();
    std::cout << "Thread 1D Elapsed Time: " << calculate_elapsed_time(start_time, end_time) << " ms\n";
    return nullptr;
}

// Thread function for a 2D loop
void* thread_func_2d(void* ptr) {
    auto start_time = get_current_time();
    ThreadData2D* t = static_cast<ThreadData2D*>(ptr);
    t->result = matrix_sum(t->start_row, t->end_row, t->start_col, t->end_col);
    auto end_time = get_current_time();
    std::cout << "Thread 2D Elapsed Time: " << calculate_elapsed_time(start_time, end_time) << " ms\n";
    return nullptr;
}

template <typename Func>
void parallel_for_1d(int low, int high, Func func, int numThreads) {
    pthread_t tid[numThreads];
    ThreadData1D td[numThreads];
    int chunk = (high - low) / numThreads;

    for (int i = 0; i < numThreads; i++) {
        td[i].low = low + i * chunk;
        td[i].high = td[i].low + chunk;
        td[i].array = nullptr;  // Set array pointer as needed

        pthread_create(&tid[i], nullptr, func, static_cast<void*>(&td[i]));
    }

    for (int i = 0; i < numThreads; i++) {
        pthread_join(tid[i], nullptr);
    }
}

template <typename Func>
void parallel_for_2d(int start_row, int end_row, int start_col, int end_col, Func func, int numThreads) {
    pthread_t tid[numThreads];
    ThreadData2D td[numThreads];
    int chunk_row = (end_row - start_row) / numThreads;

    for (int i = 0; i < numThreads; i++) {
        td[i].start_row = start_row + i * chunk_row;
        td[i].end_row = td[i].start_row + chunk_row;
        td[i].start_col = start_col;
        td[i].end_col = end_col;
        td[i].matrix = nullptr;  // Set matrix pointer as needed

        pthread_create(&tid[i], nullptr, func, static_cast<void*>(&td[i]));
    }

    for (int i = 0; i < numThreads; i++) {
        pthread_join(tid[i], nullptr);
    }
}

#endif // SIMPLE_MULTITHREADER_H

// int main() {
//     // Example for 1D parallel loop
//     int array_size = 100;
//     int num_threads_1d = 4;
//     int* my_array = new int[array_size];

//     parallel_for_1d(0, array_size, parallel_loop_1d, num_threads_1d);

//     // Example for 2D parallel loop
//     int matrix_size = 10;
//     int num_threads_2d = 2;
//     int** my_matrix = new int*[matrix_size];
//     for (int i = 0; i < matrix_size; i++) {
//         my_matrix[i] = new int[matrix_size];
//     }

//     parallel_for_2d(0, matrix_size, 0, matrix_size, parallel_loop_2d, num_threads_2d);

//     // Cleanup
//     delete[] my_array;
//     for (int i = 0; i < matrix_size; i++) {
//         delete[] my_matrix[i];
//     }
//     delete[] my_matrix;

//     return 0;
// }