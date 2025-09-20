#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>


#include <l4/sys/ipc.h>
#include <l4/sys/utcb.h>
#include <l4/re/env>
#include <l4/sys/err.h>
#include <l4/sys/types.h>

#include <l4/re/util/br_manager>
#include <l4/sys/cxx/ipc_epiface>

#include "/home/iss/L4Re_d/l4/pkg/user_shared/crypto_shared.h"
#include "/home/iss/L4Re_d/l4/pkg/user_shared/LocalMemoryManager.hpp"


#include <l4/re/util/cap_alloc> // For capability allocation
#include <l4/re/dataspace>      // For Dataspace interface
#include <l4/re/mem_alloc>      // For the memory allocator

#include <l4/re/util/object_registry>
#include <l4/sys/cxx/ipc_server>
static L4Re::Util::Registry_server<> server;
  const char *CTS_ipc_name = "CTS_ipc"; // name must be 11 characters long maximum
  const char *STC_ipc_name = "STC_ipc"; // name must be 11 characters long maximum


class Server_ : DataspaceEndpoint
{

};
int
main()
{
  std::printf("server\n");

  uint8_t big [1024*10];
  LocalMemoryManager mm(big, 1024*10);



void *a = mm.allocate_local(1000);
  void *b = mm.allocate_local(2048);
  void *c = mm.allocate_local(3000);

  std::printf("a=%td b=%td c=%td\n",
    (std::ptrdiff_t)(static_cast<std::uint8_t*>(a) - big),
    (std::ptrdiff_t)(static_cast<std::uint8_t*>(b) - big),
    (std::ptrdiff_t)(static_cast<std::uint8_t*>(c) - big));

  mm.free_local(b, 2048);
  void *d = mm.allocate_local(1024);
  std::printf("d=%td\n",
    (std::ptrdiff_t)(static_cast<std::uint8_t*>(d) - big));

  mm.free_local(a, 1000);
  mm.free_local(c, 3000);
  mm.free_local(d, 1024);

  void *e = mm.allocate_local(1024*10 - 128);
  std::printf("e=%td\n",
    (std::ptrdiff_t)(static_cast<std::uint8_t*>(e) - big));





  DataspaceEndpoint obj = DataspaceEndpoint(
    &server,
    STC_ipc_name,
    CTS_ipc_name,
    5*1024,
    1000 
  );

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
