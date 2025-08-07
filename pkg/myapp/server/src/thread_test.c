#include <stdio.h>
#include <unistd.h>
#include <pthread-l4.h>
#include "thread_test.h"

static pthread_t t2;
static void *thread_func1(void *arg)
{
  (void)arg;
  for (;;)
    {
      puts("myapp: Hello World! , using thread_func1 (main thread) !!!");
      sleep(1);
    }
  return NULL;
}
static void *thread_func2(void *arg)
{
  (void)arg;
  for (;;)
    {
      puts("myapp: Hello World! , using thread_func2 !!!");
      sleep(1);
    }
  return NULL;
}

void run_thread_test(void)
{
    if (pthread_create(&t2, NULL, thread_func2, NULL))
    {
        fprintf(stderr, "Thread creation failed\n");
        //
    }
    thread_func1(NULL);
}