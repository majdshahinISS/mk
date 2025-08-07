#ifndef TREAD_TEST_H
#define TREAD_TEST_H

// We will have two threads, one is already running the main function, the
// other (thread2) will be created using pthread_create.

void run_thread_test(void);
// This function creates a thread and runs a test function in it.

#endif // TREAD_TEST_H