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
 
class Crypto_server : public L4::Epiface_t<Crypto_server, ICrypto>
{
public:

  bool is_ready() const { return ready; }
  Crypto_server(const l4_size_t size_CTS, const l4_size_t size_STC)
  {
    ds_CTS = L4::Cap<L4Re::Dataspace>();
    ds_CTS = L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();
    if (!ds_CTS.is_valid()) {
      std::printf("Capability allocation failed\n");
      ready = false; return;
    }


    ds_STC = L4::Cap<L4Re::Dataspace>();
    ds_STC = L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();
    if (!ds_STC.is_valid()) {
      std::printf("Capability allocation failed\n");
      ready = false; return;
    }

    // Create the dataspace using the memory allocator
    long err = L4Re::Env::env()->mem_alloc()->alloc(size_CTS, ds_CTS, 0);
    if (err < 0) {
      std::printf("Memory allocation failed: %ld\n", err);
      ready = false; return;
    }
    err = L4Re::Env::env()->mem_alloc()->alloc(size_STC, ds_STC, 0);
    if (err < 0) {
      std::printf("Memory allocation failed: %ld\n", err);
      ready = false; return;
    }

    std::printf("Dataspace created successfully, size_CTS=%lu bytes, size_STC=%lu bytes\n",
                static_cast<unsigned long>(ds_CTS->size()),
                static_cast<unsigned long>(ds_STC->size()));

    // attach the dataspaces
    void *addr = nullptr;
    err = L4Re::Env::env()->rm()->attach(
        &addr, size_CTS,
        L4Re::Rm::F::Search_addr | L4Re::Rm::F::R,   // the server is allowed to read only
        L4::Ipc::make_cap(ds_CTS, L4_CAP_FPAGE_R));  // the server is allowed to read only
    if (err < 0) {
      std::printf("attach_ds_CTS: attach failed (%ld)\n", err);
      ready = false; return;
    }
    else {
      std::printf("attach_ds_CTS: attached at %p, size=%lu\n",
                  addr, static_cast<unsigned long>(size_CTS));
      p_CTS = addr;
    }

    addr = nullptr;
    err = L4Re::Env::env()->rm()->attach(
        &addr, size_STC,
        L4Re::Rm::F::Search_addr | L4Re::Rm::F::RW,   // find VA, map RW
        L4::Ipc::make_cap_rw(ds_STC));                    // grant RW rights
    if (err < 0) {
      std::printf("attach_ds_STC: attach failed (%ld)\n", err);
      ready = false; return;
    }
    else {
      std::printf("attach_ds_STC: attached at %p, size=%lu\n",
                  addr, static_cast<unsigned long>(size_STC));
      p_STC = addr;
    }
  
    ready = true;  
  }
  int op_dummy(ICrypto::Rights, int &x)
  {
    std::printf("Server: dummy called\n");
    x = _x;
    return 0;
  }

  int op_CTS_getDS(ICrypto::Rights, L4::Ipc::Cap<L4Re::Dataspace> &out_ds)
  {
    //out_ds = L4::Ipc::make_cap_rw(ds_CTS);
    out_ds = L4::Ipc::make_cap(ds_CTS, L4_CAP_FPAGE_RW);
    return L4_EOK;
  }

  int op_CTS_ready(ICrypto::Rights, u_int64_t id ,u_int8_t type, l4_size_t write_index, l4_size_t size)
  {
    std::printf("Server: CTS_ready called id=%lu, type=%u, write_index=%lu, size=%lu\n",
                static_cast<unsigned long>(id),
                static_cast<unsigned int>(type),
                static_cast<unsigned long>(write_index),
                static_cast<unsigned long>(size));    
    if (p_CTS != nullptr) {
      std::printf("Server read from CTS: %s\n", (static_cast<char*>(p_CTS) + write_index));
      return L4_EOK;
    }
    else {
      std::printf("Server: CTS pointer is null\n");
      return 1;
    }
    return L4_EOK;
  }

  private:
  
    L4::Cap<L4Re::Dataspace>  ds_CTS; // client to server
    void * p_CTS = nullptr;

    L4::Cap<L4Re::Dataspace>  ds_STC; // server to client
    void * p_STC = nullptr;
    int _x = 3;
    bool ready = false;
};



int
main()
{
  const char *CTS_ipc_name = "CTS_ipc"; // name must be 11 characters long maximum
  const char *STC_ipc_name = "STC_ipc"; // name must be 11 characters long maximum
  DataspaceOwner ds_side = DataspaceOwner(&server, STC_ipc_name); // name must be 11 characters long maximum

  /*
  L4::Cap<IDataspaceOwner> dss =L4Re::Env::env()->get_cap<IDataspaceOwner>(STC_ipc_name);
  if (!dss.is_valid()) {
    std::printf("Failed to get dss capability\n");
    return 1;
  }
  dss->init(4096, 1000); // create a dataspace of 4096 bytes
*/
  /*if (!server.registry()->register_obj(&ds_side, "crypto_ipc").is_valid())
  {
    printf("Could not register my service, is there a 'crypto_ipc' in the caps table?\n");
    return 1;
  }*/
  /*

  static Crypto_server crypto = Crypto_server(Size, Size);
  if (!crypto.is_ready()) {
    std::printf("Crypto server initialization failed\n");
    return 1;
  }
  else {
    std::printf("Crypto server initialized successfully\n");
  }
  // Register calculation server
  if (!server.registry()->register_obj(&crypto, "crypto_ipc").is_valid())
    {
      printf("Could not register my service, is there a 'crypto_ipc' in the caps table?\n");
      return 1;
    }
      */
  printf("Welcome to the Crypto server!\n"
         "I can provide a shared dataspace.\n");
  // Wait for client requests
  server.loop();
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




///////////////////////////////////////////////////////////////////////////////////
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


*/