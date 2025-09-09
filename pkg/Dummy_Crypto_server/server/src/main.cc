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
#include <l4/re/util/cap_alloc>

#include <l4/re/util/object_registry>
#include <l4/re/util/br_manager>
#include <l4/sys/cxx/ipc_epiface>

#include "/home/iss/L4Re_d/l4/pkg/user_shared/crypto_shared.h"

#include <l4/re/util/cap_alloc> // For capability allocation
#include <l4/re/dataspace>      // For Dataspace interface
#include <l4/re/mem_alloc>      // For the memory allocator

static L4Re::Util::Registry_server<> server;
 
class Calculation_server : public L4::Epiface_t<Calculation_server, Calc>
{
public:
  int op_sub(Calc::Rights, l4_uint32_t a, l4_uint32_t b, l4_uint32_t &res)
  {
    res = a - b;
    return 0;
  }
 
  int op_neg(Calc::Rights, l4_uint32_t a, l4_uint32_t &res)
  {
    res = -a;
    return 0;
  }
};

static int create_dataspace(L4::Cap<L4Re::Dataspace> &ds ,const l4_size_t size) {
  // Allocate a capability slot for the dataspace
  ds = L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();
  if (!ds.is_valid()) {
    std::printf("Capability allocation failed\n");
    return -1;
  }

  // Create the dataspace using the memory allocator
  long err = L4Re::Env::env()->mem_alloc()->alloc(size, ds, 0);
  if (err < 0) {
    std::printf("Memory allocation failed: %ld\n", err);
    return -1;
  }

  std::printf("Dataspace created successfully, size=%lu bytes\n",
              static_cast<unsigned long>(ds->size()));
  return 0;
}


static int attach_ds(L4::Cap<L4Re::Dataspace> ds, void **out_ptr, l4_size_t *out_size)
{
  if (!ds.is_valid()) {
    std::printf("attach_ds: invalid dataspace cap\n");
    return 1;
  }

  l4_size_t size = ds->size();
  void *addr = nullptr;

  long err = L4Re::Env::env()->rm()->attach(
      &addr, size,
      L4Re::Rm::F::Search_addr | L4Re::Rm::F::RW,   // find VA, map RW
      L4::Ipc::make_cap_rw(ds));                    // grant RW rights

  if (err < 0) {
    std::printf("attach_ds: attach failed (%ld)\n", err);
    return 1;
  }

  if (out_ptr)  *out_ptr  = addr;
  if (out_size) *out_size = size;

  std::printf("attach_ds: attached at %p, size=%lu\n",
              addr, static_cast<unsigned long>(size));
  return 0;
}


int
main()
{

  constexpr l4_size_t Size = 4096; // one page, adjust as needed
  L4::Cap<L4Re::Dataspace> ds;
  int res = create_dataspace(ds, Size);
  if (res < 0) {
    std::printf("Failed to create dataspace\n");
    return 1;
  }
  void *ptr;
  l4_size_t size;
  res = attach_ds(ds, &ptr, &size);
  if (res < 0) {
    std::printf("Failed to attach dataspace\n");
    return 1;
  }
  return res;

/*

  static Calculation_server calc;
 
  // Register calculation server
  if (!server.registry()->register_obj(&calc, "crypto_ipc").is_valid())
    {
      printf("Could not register my service, is there a 'crypto_ipc' in the caps table?\n");
      return 1;
    }
 
  printf("Welcome to the calculation server!\n"
         "I can do subtractions and negations.\n");
 
  // Wait for client requests
  server.loop();
 */
  return 0;
}

/*
dataspace create and attatch at runtime 
.cfg

local L4 = require("L4");
local l = L4.default_loader;

-- Create IPC gate for communication
local crypto_ipc = l:new_channel();

-- Start server application
l:start({
    log = {"C_Server", "red"},
    caps = {    }
}, "rom/Dummy_Crypto_server");

-- Start client application
l:start({
    log = {"C_Client", "green"},
    caps = {    }
}, "rom/Dummy_Crypto_client");


*/