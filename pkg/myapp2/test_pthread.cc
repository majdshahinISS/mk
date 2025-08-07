
// normal pthread

#include "Tests.h"

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <cstdio>
#include <pthread.h>

static void* thread_function(void* arg) {
    int thread_num = *(int*)arg;
    std::cout << "Thread " << thread_num << " is running." << std::endl;
    sleep(1); // Simulate some work
    std::cout << "Thread " << thread_num << " has finished." << std::endl;
    return nullptr;
}
void test_pthread() {
    std::cout << "Running thread test..."<<std::endl<<"it is normal pthread!!" << std::endl;

    const int num_threads = 5;
    pthread_t threads[num_threads];
    int thread_args[num_threads];

    // Create multiple threads
    for (int i = 0; i < num_threads; ++i) {
        thread_args[i] = i + 1;
        if (pthread_create(&threads[i], nullptr, thread_function, &thread_args[i]) != 0) {
            std::cerr << "Error creating thread " << i + 1 << std::endl;
            return;
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], nullptr);
    }

    std::cout << "All threads have finished." << std::endl;
}