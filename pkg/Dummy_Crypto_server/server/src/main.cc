#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>

// #include <pthread-l4.h>

#include "DataspaceEndpoint.hpp"
#include "LocalMemoryManager.hpp"

static L4Re::Util::Registry_server<> server;
  const char *CTS_ipc_name = "CTS_ipc"; // name must be 11 characters long maximum
  const char *STC_ipc_name = "STC_ipc"; // name must be 11 characters long maximum

static pthread_t th;

static void * worker_local_mem_allocator(void * arg)
{
  DataspaceEndpoint * ds = (DataspaceEndpoint *) arg;
  while (ds->all_is_ready.load() == false)
  {
    usleep(1000);
  }
  std::printf("Dataspace endpoint is ready now!\n");

  void * addr = nullptr;
  do 
  {
    addr = ds->allocate_local(64);
    std::printf("new address at : %p , size: %d\n",addr, 64);
  }while (addr != nullptr);
  
  

  return nullptr;
}

void test(DataspaceEndpoint * ds)
{
  int rc = pthread_create(&th, NULL, worker, ds);
  if (rc != 0) {
    std::printf("pthread_create failed at (rc=%d)", rc);
    return ;
  }
}

////////////////////////////
int
main()
{
  std::printf("server\n");

  DataspaceEndpoint obj = DataspaceEndpoint(
    &server,
    STC_ipc_name,
    CTS_ipc_name,
    5*1024,
    1000 
  );
  test(&obj);
  // Wait for client requests
  server.loop();
  //pthread_join(&thread, nullptr);
  l4_sleep_forever();
  return 0;
}

/*
dataspace get from .cfg

local L4 = require("L4")
local ld = L4.default_loader

-- Create a shared dataspace 
local shm = L4.Env.user_factory:create(
      L4.Proto.Dataspace,
      6 * 1024,                   -- size in MB
      L4.Mem_alloc_flags.Continuous |
        L4.Mem_alloc_flags.Pinned |
        L4.Mem_alloc_flags.Super_pages,
      21                                   -- alignment
    ):m("rw");


local crypto_ipc = ld:new_channel()

-- Server gets: shm + server end of channel
ld:start(
  { caps = { shm = shm, crypto_ipc = crypto_ipc:svr() }, log = { "Crypto_server", "yellow" } },
  "rom/Dummy_Crypto_server"
)

ld:start(
  { caps = { shm = shm, crypto_ipc = crypto_ipc }, log = { "Crypto_client", "green" } },
  "rom/Dummy_Crypto_client"
)

*/
