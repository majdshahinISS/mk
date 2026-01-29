#include "DataspaceOwner.h"
#include <utility>

void DataspaceOwner::set_free_peer_req_callback(free_peer_req_callback_h h)
{
  free_peer_req_callback_fn = std::move(h);
}

void DataspaceOwner::set_new_req_callback(new_req_callback_h h )
{
  new_req_callback_fn = std::move(h);
}

void * DataspaceOwner::get_pointer()  {return p_ds;}

l4_size_t DataspaceOwner::get_size()  {return size_ds;}

u_int64_t DataspaceOwner::get_timeout() {return timeout;}

bool DataspaceOwner::get_is_ready()   { return is_ready.load();}

DataspaceOwner::DataspaceOwner(
  L4Re::Util::Registry_server<> *server,
  const char*  ipc_name
  //Handler new_req_callback = nullptr,
  //Handler free_peer_req_callback = nullptr
) : server(server), ipc_name(ipc_name)
{
  ds = L4::Cap<L4Re::Dataspace>();
  ds = L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();
  if (!ds.is_valid()) {
    std::printf("DataspaceOwner: Capability allocation failed\n");
    return;
  }
  //new_req_callback_fn = new_req_callback;
  //free_peer_req_callback_fn = free_peer_req_callback;
  if (!server->registry()->register_obj(this, ipc_name).is_valid())
  {
    printf("Could not register my service, is there a 'crypto_ipc' in the caps table?\n");
    return ;
  }
}

DataspaceOwner::~DataspaceOwner() {}

int DataspaceOwner::op_init(IDataspaceOwner::Rights, const l4_size_t size, const u_int64_t timeout)
{
  std::printf("DataspaceOwner: init called size=%lu, timeout=%lu\n",
              static_cast<unsigned long>(size),
              static_cast<unsigned long>(timeout));

  long err = L4Re::Env::env()->mem_alloc()->alloc(size, ds, 0);
  if (err < 0) {
    std::printf("DataspaceOwner: Memory allocation failed: %ld\n", err);
    return 1;
  }
  std::printf("DataspaceOwner: Dataspace created successfully, size=%lu bytes\n",
              static_cast<unsigned long>(ds->size()));
  // attach the dataspace
  void *addr = nullptr;
  size_ds = ds->size();
  err = L4Re::Env::env()->rm()->attach(
      &addr, size_ds,
      L4Re::Rm::F::Search_addr | L4Re::Rm::F::RW,   // find VA, map RW
      L4::Ipc::make_cap_rw(ds));                    // grant RW rights
  if (err < 0) {
    std::printf("DataspaceOwner: attach_ds: attach failed (%ld)\n", err);
    size_ds = 0;
    return 1;
  }
  else {
    std::printf("DataspaceOwner: attach_ds: attached at %p, size=%lu\n",
                addr, static_cast<unsigned long>(size_ds));
    p_ds = addr;
    is_ready.store(true);
  }
  return 0; // not implemented
}

int DataspaceOwner::op_getDS(IDataspaceOwner::Rights, L4::Ipc::Cap<L4Re::Dataspace> &out_ds)
{
  std::printf("getDS called\n\n");
  out_ds = L4::Ipc::make_cap(ds, L4_CAP_FPAGE_R);  // @MSTODO
  return L4_EOK;
}

// the data is in the other side (peer) and not local !!!
int DataspaceOwner::op_new_req(IDataspaceOwner::Rights, u_int64_t id ,u_int8_t type, l4_size_t write_index_peer, l4_size_t size)
{
  /*std::printf("DataspaceOwner: new_req called id=%lu, type=%u, write_index_peer=%lu, size=%lu\n",
              static_cast<unsigned long>(id),
              static_cast<unsigned int>(type),
              static_cast<unsigned long>(write_index_peer),
              static_cast<unsigned long>(size));
  */
  if (new_req_callback_fn) {
    return new_req_callback_fn(id, type, write_index_peer, size);
  }
  else
    std::printf("DataspaceOwner: new_req_callback_fn is null\n");

  return L4_EOK;
}

// request from peer to free data on local memory
int DataspaceOwner::op_free_your_data(IDataspaceOwner::Rights, u_int64_t id ,u_int8_t type, l4_size_t write_index, l4_size_t size)
{
  /*std::printf("DataspaceOwner: free_your_data called id=%lu, type=%u, write_index=%lu, size=%lu\n",
              static_cast<unsigned long>(id),
              static_cast<unsigned int>(type),
              static_cast<unsigned long>(write_index),
              static_cast<unsigned long>(size));  */
  if (free_peer_req_callback_fn) {
    return free_peer_req_callback_fn(id, type, write_index, size);
  }
  else
    std::printf("DataspaceOwner: free_peer_req_callback_fn is null\n");
  return L4_EOK;
}
