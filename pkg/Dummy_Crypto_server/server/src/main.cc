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

static void * worker(void * arg)
{
  sleep(1);
  DataspaceEndpoint * ds = (DataspaceEndpoint *) arg;
  while (ds->all_is_ready.load() == false)
  {
    usleep(1000);
  }
  std::printf("Dataspace endpoint is ready now!\n");

  void * addr = nullptr;
  int i = 0;
  do 
  {
    addr = ds->allocate_local(64);
    if(addr == nullptr)
    {
      std::printf("error return nullptr\n");
      break;
    }
    std::printf("new address at : %p , size: %d\n",addr, 64);

    //sprintf((char *) addr, "req[%d]",i);
    int ret = ds->add_new_req(i, 0, addr, 64);
    if(ret != 0)
    {
      std::printf("error\n");
    }
    i++;
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
