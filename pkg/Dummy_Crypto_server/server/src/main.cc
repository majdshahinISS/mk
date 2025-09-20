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


#include <l4/re/util/cap_alloc> // For capability allocation
#include <l4/re/dataspace>      // For Dataspace interface
#include <l4/re/mem_alloc>      // For the memory allocator

#include <l4/re/util/object_registry>
#include <l4/sys/cxx/ipc_server>
static L4Re::Util::Registry_server<> server;
  const char *CTS_ipc_name = "CTS_ipc"; // name must be 11 characters long maximum
  const char *STC_ipc_name = "STC_ipc"; // name must be 11 characters long maximum

int
main()
{
  std::printf("server\n");
  //if (start_server_loop() != 0) return 1;

  

  // register dataspace owner of the server side
  DataspaceOwner ds_side = DataspaceOwner(&server, STC_ipc_name); // name must be 11 characters long maximum
  std::printf("server: %d\n",__LINE__);


  // get interface to the dataspace owner of the server side
  L4::Cap<IDataspaceOwner> dss  =L4Re::Env::env()->get_cap<IDataspaceOwner>(CTS_ipc_name);
  if (!dss.is_valid()) {
    std::printf("Failed to get dss capability\n");
    return 1;
  }
  dss->init(1024, 1000); // create a dataspace of 4096 bytes
  std::printf("server: %d\n",__LINE__);

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
