#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>

// #include <pthread-l4.h>

#include "DataspaceEndpoint.hpp"

static L4Re::Util::Registry_server<> server;
const char *CTS_ipc_name = "CTS_ipc"; // name must be 11 characters long maximum
const char *STC_ipc_name = "STC_ipc"; // name must be 11 characters long maximum

struct WorkerArgs
{
  DataspaceEndpoint *ds;   // NOT owned
  u_int64_t          id;
  u_int8_t           type;
  u_int8_t          *addr; // pointer must stay valid while worker runs
  l4_size_t          size;
};


// ---- worker function (runs on detached pthread) ----
static void *handle_new_data_worker(void *opaque)
{
  // Take ownership of the args object to ensure it’s freed
  WorkerArgs *args = static_cast<WorkerArgs*>(opaque);

  std::printf("@MS Client serve req. id %d, read from address: %p : %s\n",args->id, static_cast<void*>(args->addr),args->addr);
  usleep(1);

  std::printf("request peer to free his data\n");
  int res = args->ds->free_peer_req(args->id, args->type, args->addr, args->size);
  if (res)
    std::printf("free_peer_req failed: %d\n", res);

  delete args; // free the envelope
  return nullptr;
}


// ---- callback that spawns the worker and returns immediately ----
int new_data_callback_handler(DataspaceEndpoint *ds,
                              u_int64_t id, u_int8_t type,
                              u_int8_t *addr, l4_size_t size)
{
  // Copy all needed values into a heap envelope for the thread
  WorkerArgs *args = new WorkerArgs{ds, id, type, addr, size};

  pthread_t tid;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

  int rc = pthread_create(&tid, &attr, &handle_new_data_worker, args);

  pthread_attr_destroy(&attr);

  if (rc != 0) {
    // Thread not created; clean up and propagate error
    delete args;
    return rc; // or translate as needed
  }

  // Success: thread is detached and running independently
  return 0;
}

int
main()
{
  std::printf("client\n");
  DataspaceEndpoint obj = DataspaceEndpoint(
    &server,
    CTS_ipc_name,
    STC_ipc_name,
    2*1024,
    1000 ,
    new_data_callback_handler
  );
  
  server.loop();
  //pthread_join(&thread, nullptr);
  l4_sleep_forever();
  return 0;
}