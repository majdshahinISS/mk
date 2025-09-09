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
// get dataspace from .cfg file
static int get_dataspace(L4::Cap<L4Re::Dataspace> &ds ,l4_size_t &size, const char *name) {
  // Get the shared dataspace capability named "shm" from the .cfg
  ds = L4Re::Env::env()->get_cap<L4Re::Dataspace>(name);
  if (!ds.is_valid()) {
    std::printf("Server: \'%s\' cap missing\n", name);
    return -1;
  };
  size = ds->size();

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
  L4::Cap<L4Re::Dataspace> ds;
  int res = 0;
  
  
  // constexpr l4_size_t Size = 4096; // one page, adjust as needed
  // res = create_dataspace(ds, Size);

  l4_size_t Size = 0;
  res = get_dataspace(ds, Size, "shm");
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