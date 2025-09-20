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

#include <l4/util/util.h>
#include <stdio.h>
#include <pthread-l4.h>

#include <atomic> 

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
  L4_INLINE_RPC(int, new_req, (u_int64_t id ,u_int8_t type, l4_size_t write_index, l4_size_t size));
  L4_INLINE_RPC(int, free_your_data, (u_int64_t id ,u_int8_t type, l4_size_t write_index, l4_size_t size));
  //L4_INLINE_RPC(int, placeholder, ());
  typedef L4::Typeid::Rpcs< init_t, getDS_t, new_req_t, free_your_data_t> Rpcs;
};

typedef int (* free_your_data_callback_t)(u_int64_t id ,u_int8_t type, l4_size_t write_index, l4_size_t size);
typedef int (* new_req_callback_t)(u_int64_t id ,u_int8_t type, l4_size_t write_index, l4_size_t size);
class DataspaceOwner : public L4::Epiface_t<DataspaceOwner, IDataspaceOwner>
{
  L4::Cap<L4Re::Dataspace>  ds; // shared dataspace
  void * p_ds = nullptr;
  l4_size_t size_ds;
  u_int64_t timeout;
  //L4Re::Util::Registry_server<> *server;
  //char* ipc_name[11]; // must be 11 characters long maximum 
  new_req_callback_t new_req_callback_fn ; 
  free_your_data_callback_t free_your_data_callback_fn; 
  L4Re::Util::Registry_server<> *server;
  const char*  ipc_name;
  std::atomic<bool> is_ready{false};
  #include <l4/sys/utcb.h>


  public:
  void * get_pointer()  {return p_ds;}
  l4_size_t get_size()  {return size_ds;}
  u_int64_t get_timeout() {return timeout;}
  bool get_is_ready()   { return is_ready.load();}
  DataspaceOwner(
    L4Re::Util::Registry_server<> *server,
    const char*  ipc_name,
    new_req_callback_t new_req_callback = nullptr, 
    free_your_data_callback_t free_your_data_callback = nullptr 
  ) : server(server), ipc_name(ipc_name)
  {
    ds = L4::Cap<L4Re::Dataspace>();
    ds = L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();
    if (!ds.is_valid()) {
      std::printf("DataspaceOwner: Capability allocation failed\n");
      return;
    }
    new_req_callback_fn = new_req_callback;
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
  int op_getDS(IDataspaceOwner::Rights, L4::Ipc::Cap<L4Re::Dataspace> &out_ds)
  {
    out_ds = L4::Ipc::make_cap(ds, L4_CAP_FPAGE_RW);  // @MSTODO
    return L4_EOK;
  }
  int op_new_req(IDataspaceOwner::Rights, u_int64_t id ,u_int8_t type, l4_size_t write_index, l4_size_t size)
  {
    std::printf("DataspaceOwner: new_req called id=%lu, type=%u, write_index=%lu, size=%lu\n",
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
    if (new_req_callback_fn) {
      return new_req_callback_fn(id, type, write_index, size);
    }
    else
      std::printf("DataspaceOwner: new_req_callback_fn is null\n");
    
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



class DataspaceEndpoint
{
  protected:
    DataspaceOwner local_dataspaceOwner;

    pthread_t t;
    // server information (otherside)
    L4::Cap<IDataspaceOwner>  peer_owner_intf;
    l4_size_t peer_size;
    u_int64_t    peer_timeout;
    L4::Cap<L4Re::Dataspace>  peer_ds;
    void * peer_addr = nullptr;
    std::atomic<bool> peer_is_ready{false};

    static void * server_intf_getter(void * args)
    {
      DataspaceEndpoint * p = (DataspaceEndpoint *) args;
      if ( p->peer_is_ready.load() == true)
        return nullptr;
      
      p->peer_owner_intf->init(p->peer_size, p->peer_timeout);
      
      int r = p->peer_owner_intf->getDS(p->peer_ds); 
      if (r != L4_EOK) {
        std::printf("getDS failed: error : 0x%x\n", r);
        return nullptr;
      }
      else 
      {
        std::printf("getDS succeeded, dataspace size=%lu bytes\n",
                    static_cast<unsigned long>(p->peer_ds->size()));
      } 

      long err = L4Re::Env::env()->rm()->attach(
          & p->peer_addr, p->peer_size,
          L4Re::Rm::F::Search_addr | L4Re::Rm::F::RW,   // find VA, map RW
          L4::Ipc::make_cap(p->peer_ds, L4_CAP_FPAGE_RW));                    // grant RW rights
      if (err < 0) {
        std::printf("attach_ds: attach failed (%ld)\n", err);
        return nullptr;
      }

      return nullptr;
    }

    int start_geting_server_intf()
    {
      int rc = pthread_create(&t, nullptr, server_intf_getter, (void*)this);
      return rc;
    }

  public:
  DataspaceEndpoint(
    L4Re::Util::Registry_server<> *server,
    const char *CTS_ipc_name,
    const char *STC_ipc_name,
    l4_size_t   peer_size,
    u_int64_t    peer_timeout,
    new_req_callback_t new_req_callback_fn= nullptr , 
    free_your_data_callback_t free_your_data_callback_fn  = nullptr  
  ): 
      local_dataspaceOwner(server, CTS_ipc_name, new_req_callback_fn, free_your_data_callback_fn), 
      peer_size(peer_size), 
      peer_timeout(peer_timeout)
  {
    peer_owner_intf = L4Re::Env::env()->get_cap<IDataspaceOwner>(STC_ipc_name);
    peer_ds = L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();

    // @TODO check
    start_geting_server_intf();
  }

  ~DataspaceEndpoint()
  {

  }
};



#endif // __CRYPTO_SHARED_