#ifndef __CRYPTO_SHARED__
#define __CRYPTO_SHARED__

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


#include <l4/sys/capability>
#include <l4/sys/cxx/ipc_iface>
#include <l4/re/dataspace> 
#include <l4/re/util/cap_alloc>

struct ICrypto : L4::Kobject_t<ICrypto, L4::Kobject, 0x45>
{
  L4_INLINE_RPC(int, dummy, (int &x));
  L4_INLINE_RPC(int, CTS_getDS, (L4::Ipc::Out<L4::Cap<L4Re::Dataspace>> out_ds));
  L4_INLINE_RPC(int, CTS_ready, (u_int64_t id ,u_int8_t type, l4_size_t write_index, l4_size_t size));
  typedef L4::Typeid::Rpcs<dummy_t, CTS_getDS_t, CTS_ready_t> Rpcs;
};

struct IDataspaceOwner : L4::Kobject_t<IDataspaceOwner, L4::Kobject, 0x47>
{
  L4_INLINE_RPC(int, init, (const l4_size_t size, const u_int64_t timeout));
  L4_INLINE_RPC(int, getDS, (L4::Ipc::Out<L4::Cap<L4Re::Dataspace>> out_ds));
  L4_INLINE_RPC(int, new_data, (u_int64_t id ,u_int8_t type, l4_size_t write_index, l4_size_t size));
  L4_INLINE_RPC(int, free_your_data, (u_int64_t id ,u_int8_t type, l4_size_t write_index, l4_size_t size));
  //L4_INLINE_RPC(int, placeholder, ());
  typedef L4::Typeid::Rpcs< init_t, getDS_t, new_data_t, free_your_data_t> Rpcs;
};

typedef int (* free_your_data_callback_t)(u_int64_t id ,u_int8_t type, l4_size_t write_index, l4_size_t size);
typedef int (* new_data_callback_t)(u_int64_t id ,u_int8_t type, l4_size_t write_index, l4_size_t size);
class DataspaceOwner : public L4::Epiface_t<DataspaceOwner, IDataspaceOwner>
{
  L4::Cap<L4Re::Dataspace>  ds; // shared dataspace
  void * p_ds = nullptr;

  //L4Re::Util::Registry_server<> *server;
  //char* ipc_name[11]; // must be 11 characters long maximum 
  new_data_callback_t new_data_callback_fn ; 
  free_your_data_callback_t free_your_data_callback_fn; 
  public:
  DataspaceOwner(
    L4Re::Util::Registry_server<> *server,
    const char*  ipc_name,
    new_data_callback_t new_data_callback = nullptr, 
    free_your_data_callback_t free_your_data_callback = nullptr 
  ) 
  {
    ds = L4::Cap<L4Re::Dataspace>();
    ds = L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();
    if (!ds.is_valid()) {
      std::printf("DataspaceOwner: Capability allocation failed\n");
      return;
    }
    new_data_callback_fn = new_data_callback;
    free_your_data_callback_fn = free_your_data_callback;
    if (!server->registry()->register_obj(this, ipc_name).is_valid())
    {
      printf("Could not register my service, is there a 'crypto_ipc' in the caps table?\n");
      return ;
    }


  }
  ~DataspaceOwner() {}

  int op_init(IDataspaceOwner::Rights, const l4_size_t size, const u_int64_t timeout)
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
    l4_size_t size_ds = ds->size();
    err = L4Re::Env::env()->rm()->attach(
        &addr, size_ds,
        L4Re::Rm::F::Search_addr | L4Re::Rm::F::RW,   // find VA, map RW
        L4::Ipc::make_cap_rw(ds));                    // grant RW rights
    if (err < 0) {
      std::printf("DataspaceOwner: attach_ds: attach failed (%ld)\n", err);
      return 1;
    }
    else {
      std::printf("DataspaceOwner: attach_ds: attached at %p, size=%lu\n",
                  addr, static_cast<unsigned long>(size_ds));
      p_ds = addr;
    } 
    return 0; // not implemented
  }
  int op_getDS(IDataspaceOwner::Rights, L4::Ipc::Cap<L4Re::Dataspace> &out_ds)
  {
    out_ds = L4::Ipc::make_cap(ds, L4_CAP_FPAGE_RW);
    return L4_EOK;
  }
  int op_new_data(IDataspaceOwner::Rights, u_int64_t id ,u_int8_t type, l4_size_t write_index, l4_size_t size)
  {
    std::printf("DataspaceOwner: new_data called id=%lu, type=%u, write_index=%lu, size=%lu\n",
                static_cast<unsigned long>(id),
                static_cast<unsigned int>(type),
                static_cast<unsigned long>(write_index),
                static_cast<unsigned long>(size));    
    if (p_ds != nullptr) {
      std::printf("DataspaceOwner read from dataspace: %s\n", (static_cast<char*>(p_ds) + write_index));
      return L4_EOK;
    }
    else {
      std::printf("DataspaceOwner: dataspace pointer is null\n");
      return 1;
    }
    if (new_data_callback_fn) {
      return new_data_callback_fn(id, type, write_index, size);
    }
    else
      std::printf("DataspaceOwner: new_data_callback_fn is null\n");
    
    return L4_EOK;
  }

  int op_free_your_data(IDataspaceOwner::Rights, u_int64_t id ,u_int8_t type, l4_size_t write_index, l4_size_t size)
  {
    std::printf("DataspaceOwner: free_your_data called id=%lu, type=%u, write_index=%lu, size=%lu\n",
                static_cast<unsigned long>(id),
                static_cast<unsigned int>(type),
                static_cast<unsigned long>(write_index),
                static_cast<unsigned long>(size));    
    if (free_your_data_callback_fn) {
      return free_your_data_callback_fn(id, type, write_index, size);
    }
    else
      std::printf("DataspaceOwner: free_your_data_callback_fn is null\n");
    return L4_EOK;
  } 

  int op_placeholder(IDataspaceOwner::Rights)
  {
    std::printf("DataspaceOwner: placeholder called\n");
    return 0;
  }
};
/*
#include <memory>
class DS_Side
{
private:
  std::unique_ptr<DataspaceOwner> this_side;  // null initially
  // new_data_callback_t incomming_new_data_handler;
  // free_your_data_callback_t free_your_data_handler; // handel free data req. from the other side
public:
  L4::Cap<IDataspaceOwner> other_side ;
  DS_Side(
    L4Re::Util::Registry_server<> *server = nullptr,
    const char *this_side_ipc_name = nullptr, 
    const char *other_side_ipc_name = nullptr,
    new_data_callback_t new_data_callback = nullptr,            // handle new available data from the other side
    free_your_data_callback_t free_your_data_callback = nullptr // handle free data req. from the other side
  ) 
  {
    if (server != nullptr && this_side_ipc_name != nullptr)
    {
        this_side = std::make_unique<DataspaceOwner>(
        server, 
        this_side_ipc_name,
        new_data_callback,
        free_your_data_callback // handle free data req. from the other side
      ); // name must be 11 characters long maximum
    }
    else 
      this_side = nullptr;
    if (other_side_ipc_name != nullptr)
    {
      other_side =  L4::Cap<IDataspaceOwner>();
      other_side =L4Re::Env::env()->get_cap<IDataspaceOwner>(other_side_ipc_name);
      if (!other_side.is_valid()) {
        std::printf("Failed to get dss capability\n");
        return ;
      }
    }
    //else
      //other_side = nullptr;
  }

};

*/

#endif // __CRYPTO_SHARED_