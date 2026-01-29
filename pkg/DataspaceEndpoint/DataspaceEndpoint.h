#pragma once

#include <l4/sys/ipc.h>
#include <l4/sys/utcb.h>
#include <l4/re/env>
#include <l4/sys/err.h>
#include <l4/sys/types.h>
#include <l4/re/util/br_manager>
#include <l4/sys/cxx/ipc_epiface>
#include <l4/re/util/cap_alloc> // For capability allocation
#include <l4/re/dataspace>      // For Dataspace interface
#include <l4/re/mem_alloc>      // For the memory allocator
#include <l4/re/util/object_registry>
#include <l4/sys/cxx/ipc_server>
#include <l4/sys/capability>
#include <l4/sys/cxx/ipc_iface>
#include <l4/util/util.h>
#include <stdio.h>
#include <pthread-l4.h>
#include <atomic>
#include <functional>
#include <cassert>
#include <unistd.h> 
#include "DataspaceOwner.h"
#include "LocalMemoryManager.h"

class DataspaceEndpoint
{
private:
  struct WorkerArgs {
    DataspaceEndpoint *ds;      // not owned
    u_int64_t          id;
    u_int8_t           type;
    const u_int8_t    *addr; // must remain valid while worker runs
    l4_size_t          size;
  };
  LocalMemoryManager lmm;
  DataspaceOwner local_dataspaceOwner;
  std::atomic<bool> local_is_ready{false};

  pthread_t th_peer_intf, th_local_mem;
  pthread_attr_t attr;
  // server information (otherside)
  L4::Cap<IDataspaceOwner>  peer_owner_intf;
  l4_size_t peer_size;
  u_int64_t    peer_timeout;
  L4::Cap<L4Re::Dataspace>  peer_ds;
  void * peer_addr = nullptr;
  std::atomic<bool> peer_is_ready{false};
  std::atomic<bool> all_is_ready{false};
  std::function<int(DataspaceEndpoint* obj,u_int64_t id ,u_int8_t type, const u_int8_t *, l4_size_t size)> new_req_callback_handler;
  pthread_t g_main_tid; // just to store the main thread

  static void * server_intf_getter(void * args);
  int start_geting_server_intf();

  static void * localMemoryManager_initializer_th( void * arg);
  int start_init_LocalMemoryManager();

  static void * free_peer_req_callback_thread(void * arg);
  int free_peer_req_callback(u_int64_t id ,u_int8_t type, l4_size_t write_index, l4_size_t size);
  int new_data_callback_priv(u_int64_t id ,u_int8_t type, l4_size_t write_index, l4_size_t size);
  int new_data_callback_default(u_int64_t id, u_int8_t type, u_int8_t * read_addr , l4_size_t size);

  static void * add_new_req_worker(void * arg);
  static void * free_peer_req_worker(void* arg);

public:
  DataspaceEndpoint(
    L4Re::Util::Registry_server<> *server,
    const char *CTS_ipc_name,
    const char *STC_ipc_name,
    l4_size_t   peer_size,
    u_int64_t    peer_timeout,
    std::function<int(DataspaceEndpoint* obj,u_int64_t id ,u_int8_t type, const u_int8_t * addr, l4_size_t size)> new_req_callback_handler_= nullptr
  );

  ~DataspaceEndpoint();

  void * allocate_local( l4_size_t size);

  void * allocate_local_blocking(l4_size_t size);

  // the address must be in local dataspace using LocalMemeoryManager
  int add_new_req(u_int64_t id ,u_int8_t type, void * addr, l4_size_t size);

  int free_peer_req(u_int64_t id ,u_int8_t type, const void * addr, l4_size_t size);

  bool wait_for_initialization(int timeout);
};
