
#include "Tests.h"
#include <l4/sys/ipc.h>
 
#include <pthread-l4.h>
#include <unistd.h>
#include <stdio.h>
 
static pthread_t t1,t2;
 
/* Thread1 is the initiator thread, i.e. it initiates the IPC calls. In
 * other words, it takes the client role. It uses L4 IPC mechanisms to send
 * an integer value to thread2 and received a calculation result back. */
static void *thread1_fn(void *arg)
{
  l4_msgtag_t tag;
  int ipc_error;
  unsigned long value = 1;
  (void)arg;
 
  for(int i = 0; i < 3; i++)
    {
      printf("Sending:  %ld\n", value);
 
      /* Store the value which we want to have squared in the first message
       * register of our UTCB. */
      l4_utcb_mr()->mr[0] = value;
 
      /* To an L4 IPC call, i.e. send a message to thread2 and wait for a
       * reply from thread2. The '1' in the msgtag denotes that we want to
       * transfer one word of our message registers (i.e. MR0). No timeout. */
      tag = l4_ipc_call(pthread_l4_cap(t2), l4_utcb(),
                        l4_msgtag(0, 1, 0, 0), L4_IPC_NEVER);
      /* Check for IPC error, if yes, print out the IPC error code, if not,
       * print the received result. */
      ipc_error = l4_ipc_error(tag, l4_utcb());
      if (ipc_error)
        fprintf(stderr, "thread1: IPC error: %x\n", ipc_error);
      else
        printf("Received: %ld\n", l4_utcb_mr()->mr[0]);
 
      /* Wait some time and increment our value. */
      sleep(1);
      value++;
    }
    printf("Thread1 finished\n");

    sleep(1);

    printf("Thread2 finished\n");
    // Clean up
    pthread_cancel(t2);

  return NULL;
}
 
/* Thread2 is in the server role, i.e. it waits for requests from others and
 * sends back the calculation results. */
#define THREAD2_TIMEOUT 5
static void *thread2_fn(void *arg)
{
  l4_msgtag_t tag;
  l4_umword_t label;
  int ipc_error;
  (void)arg;
//   l4_timeout_t timeout ;
//   //timeout.raw = 0;
//   timeout.p.snd.t = 0;
//   timeout.p.rcv.t = THREAD2_TIMEOUT;
  /* Wait for requests from any thread. No timeout, i.e. wait forever. */
  tag = l4_ipc_wait(l4_utcb(), &label, L4_IPC_NEVER);  // L4_IPC_NEVER
  while (1)
    {
      /* Check if we had any IPC failure, if yes, print the error code
       * and just wait again. */
      ipc_error = l4_ipc_error(tag, l4_utcb());
      if ((ipc_error == L4_IPC_RETIMEOUT) || (ipc_error == L4_IPC_SETIMEOUT))
        {
          printf("thread2: IPC timeout\n");
          return NULL;
        }


      if (ipc_error)
        {
          fprintf(stderr, "thread2: IPC error: %x\n", ipc_error);
          tag = l4_ipc_wait(l4_utcb(), &label, L4_IPC_NEVER); // L4_IPC_NEVER
          continue;
        }
 
      /* So, the IPC was ok, now take the value out of message register 0
       * of the UTCB and store the square of it back to it. */
      l4_utcb_mr()->mr[0] = l4_utcb_mr()->mr[0] * l4_utcb_mr()->mr[0];
 
      /* Send the reply and wait again for new messages.
       * The '1' in the msgtag indicated that we want to transfer 1 word in
       * the message registers (i.e. MR0) */
      tag = l4_ipc_reply_and_wait(l4_utcb(), l4_msgtag(0, 1, 0, 0),
                                  &label, L4_IPC_NEVER);
    }
  return NULL;
}
 
void test_l4_thread_ipc_1(void)
{
  // We will have two threads, one is already running the main function, the
  // other (thread2) will be created using pthread_create.
 
  if (pthread_create(&t2, NULL, thread2_fn, NULL))
    {
      fprintf(stderr, "Thread 2 creation failed\n");
      return ;
    }

    if (pthread_create(&t1, NULL, thread1_fn, NULL))
    {
        fprintf(stderr, "Thread 1 creation failed\n");
        return ;
    }
  // Just run thread1 in the main thread
  sleep(6);

  return ;
}