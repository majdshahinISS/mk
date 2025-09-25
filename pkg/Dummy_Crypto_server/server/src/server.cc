#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <random>
// #include <pthread-l4.h>

#include "DataspaceEndpoint.hpp"
//#include "LocalMemoryManager.hpp"
#include "helperFunctions.hpp"

static L4Re::Util::Registry_server<> server;
const char *CTS_ipc_name = "CTS_ipc"; // name must be 11 characters long maximum
const char *STC_ipc_name = "STC_ipc"; // name must be 11 characters long maximum

int new_data_callback_handler(DataspaceEndpoint * ds,u_int64_t id, u_int8_t type, const u_int8_t * read_addr , l4_size_t size);
void test(DataspaceEndpoint * ds);


int new_data_callback_handler(DataspaceEndpoint * ds,u_int64_t id, u_int8_t type, const u_int8_t * read_addr , l4_size_t size)
{
  // run in a thread ! 
  u_int64_t ids = get_id((char*)read_addr);
  // read_addr[0] = 'S'; // will cause an error ! ( compiletime , if workaround it it will cause runtime error)
  if(ids != id )
    std::printf("Error id %lu\n", id);
  std::printf("handle req. id %lu, read from address: %p : %s\n",id, static_cast<const void*>(read_addr), read_addr);
  //sleep(1);
  int res = ds->free_peer_req(id,type, read_addr, size);
  return res;
}
static pthread_t th;

static void * worker(void * arg)
{
  sleep(1);
  DataspaceEndpoint * ds = (DataspaceEndpoint *) arg;
  ds->wait_for_initialization();
  std::printf("Dataspace endpoint is ready now!\n");

  void * addr = nullptr;
  int i = 0;
  do 
  {
    thread_local std::mt19937_64 rng(std::random_device{}());
    std::uniform_int_distribution<std::size_t> size_dist(32, 256);
    // std::uniform_int_distribution<unsigned>     byte_dist(0, 255);

    // 1) random size in [32, 256]
    const std::size_t size = size_dist(rng);
    addr = ds->allocate_local_blocking(size);
    if(addr == nullptr)
    {
      std::printf("error return nullptr\n");
      break;
    }
    //std::printf("to serve req. %d , new address at : %p , size: %d\n",i,addr, size);

    sprintf((char*)addr, "req[%d], ISS-AG",i);

    int ret = ds->add_new_req(i, 0, addr, size);
    if(ret != 0)
    {
      std::printf("error\n");
    }
    i++;
    //if (i == 50) break;
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
    1000, 
    new_data_callback_handler
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
