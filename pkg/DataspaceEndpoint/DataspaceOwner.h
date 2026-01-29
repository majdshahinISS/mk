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
#include <string>
#include <iostream>

struct IDataspaceOwner : L4::Kobject_t<IDataspaceOwner, L4::Kobject, 0x47>
{
  L4_INLINE_RPC(int, init, (const l4_size_t size, const u_int64_t timeout));
  L4_INLINE_RPC(int, getDS, (L4::Ipc::Out<L4::Cap<L4Re::Dataspace>> out_ds));
  L4_INLINE_RPC(int, new_req, (u_int64_t id ,u_int8_t type, l4_size_t write_index, l4_size_t size));
  L4_INLINE_RPC(int, free_your_data, (u_int64_t id ,u_int8_t type, l4_size_t write_index, l4_size_t size));
  typedef L4::Typeid::Rpcs< init_t, getDS_t, new_req_t, free_your_data_t> Rpcs;
};

using free_peer_req_callback_h = std::function<int(u_int64_t id ,u_int8_t type, l4_size_t write_index, l4_size_t size)>;
using new_req_callback_h = std::function<int(u_int64_t id ,u_int8_t type, l4_size_t write_index, l4_size_t size)>;

class DataspaceOwner : public L4::Epiface_t<DataspaceOwner, IDataspaceOwner>
{
  L4::Cap<L4Re::Dataspace>  ds; // shared dataspace
  void * p_ds = nullptr;
  l4_size_t size_ds;
  u_int64_t timeout;
  //L4Re::Util::Registry_server<> *server;
  //char* ipc_name[11]; // must be 11 characters long maximum
  free_peer_req_callback_h new_req_callback_fn;
  new_req_callback_h free_peer_req_callback_fn;
  L4Re::Util::Registry_server<> *server;
  const char*  ipc_name;
  std::atomic<bool> is_ready{false};

public:
  void set_free_peer_req_callback(free_peer_req_callback_h h);
  void set_new_req_callback(new_req_callback_h h );
  void * get_pointer();
  l4_size_t get_size();
  u_int64_t get_timeout();
  bool get_is_ready();
  DataspaceOwner(
    L4Re::Util::Registry_server<> *server,
    const char*  ipc_name
    //Handler new_req_callback = nullptr,
    //Handler free_peer_req_callback = nullptr
  );
  ~DataspaceOwner();

  int op_init(IDataspaceOwner::Rights, const l4_size_t size, const u_int64_t timeout);
  int op_getDS(IDataspaceOwner::Rights, L4::Ipc::Cap<L4Re::Dataspace> &out_ds);
  int op_new_req(IDataspaceOwner::Rights, u_int64_t id ,u_int8_t type, l4_size_t write_index_peer, l4_size_t size);
  int op_free_your_data(IDataspaceOwner::Rights, u_int64_t id ,u_int8_t type, l4_size_t write_index, l4_size_t size);
};
