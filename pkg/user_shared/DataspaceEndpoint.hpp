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
#include "DataspaceOwner.hpp"
#include "LocalMemoryManager.hpp"

class DataspaceEndpoint
{
  private:

    LocalMemoryManager lmm;
    DataspaceOwner local_dataspaceOwner;
    std::atomic<bool> local_is_ready{false};

    pthread_t th_peer_intf, th_local_mem;
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
      p->peer_is_ready.store(true);
      std::printf("peer is ready\n");
      if (p->local_is_ready.load() == true)
      {
        std::printf("all is ready\n");
        p->all_is_ready.store(true);
      }
      return nullptr;
    }

    int start_geting_server_intf()
    {
      int rc = pthread_create(&th_peer_intf, nullptr, server_intf_getter, (void*)this);
      return rc;
    }

    static void * localMemoryManager_initializer_th( void * arg)
    {
      DataspaceEndpoint * p = (DataspaceEndpoint*) arg;
      while (p->local_dataspaceOwner.get_is_ready() == false)
      {
        usleep(1 * 10);
      }
      p->lmm.init(p->local_dataspaceOwner.get_pointer(), p->local_dataspaceOwner.get_size());
      p->local_is_ready.store(true);
      std::printf("local is ready\n");
      if (p->peer_is_ready.load() == true)
      {
        std::printf("all is ready\n");
        p->all_is_ready.store(true);
      }
      return nullptr;
    }
    int start_init_LocalMemoryManager()
    {
      int rc = pthread_create(&th_local_mem, nullptr, localMemoryManager_initializer_th, (void*)this);
      return rc;
    }

    int free_your_data_callback(u_int64_t id ,u_int8_t type, l4_size_t write_index, l4_size_t size)
    {
      std::printf("dummy free_your_data_callback , id: %lu\n", id);
      return 0;
    }


  public:
  DataspaceEndpoint(
    L4Re::Util::Registry_server<> *server,
    const char *CTS_ipc_name,
    const char *STC_ipc_name,
    l4_size_t   peer_size,
    u_int64_t    peer_timeout
    // new_req_callback_t new_req_callback_fn= nullptr 
    //free_your_data_callback_t free_your_data_callback_fn  = nullptr  
  ): 
      local_dataspaceOwner(server, CTS_ipc_name),
      peer_size(peer_size), 
      peer_timeout(peer_timeout)
  {
    //local_dataspaceOwner.set_free_your_data_callback(free_your_data_callback);
    local_dataspaceOwner.set_free_your_data_callback(
      [this](u_int64_t id ,u_int8_t type, l4_size_t write_index, l4_size_t size) -> int {
      return free_your_data_callback(id, type, write_index, size);}
    );
    peer_owner_intf = L4Re::Env::env()->get_cap<IDataspaceOwner>(STC_ipc_name);
    peer_ds = L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();

    // @TODO check
    start_geting_server_intf();
    start_init_LocalMemoryManager();
  }

  ~DataspaceEndpoint()
  {

  }
  void * allocate_local( l4_size_t size)
  {
    if ((local_is_ready.load()==false) || size > local_dataspaceOwner.get_size() )
      return nullptr;

    return lmm.allocate_local(size);
    
  }
  // the address must be in local dataspace using LocalMemeoryManager
  int add_new_req(u_int64_t id ,u_int8_t type, void * addr, l4_size_t size)
  {
    if (local_is_ready.load()==false)
    {
      std::printf("local dataspace is not ready yet\n");
      return -1;
    }
    auto base_ptr = static_cast<const std::byte*>(local_dataspaceOwner.get_pointer());
    auto ptr = static_cast<const std::byte*>(addr);
    if(
      ( ptr < base_ptr) 
      || ptr > base_ptr + local_dataspaceOwner.get_size())
    {
      std::printf("Error , the address is not in the local data space \nplease use only the address allocated using allocate_local\n");
      return -1;
    }
    l4_size_t write_index = ptr - base_ptr ;
    peer_owner_intf->new_req(id, type, write_index, size);

    return 0;
  }

  int free_peer_req(u_int64_t id ,u_int8_t type, void * addr, l4_size_t size)
  {
    if(peer_is_ready.load() == false)
    {
      std::printf("Error, the peer is not ready yet");
    }
    auto base_ptr = static_cast<const std::byte*>(peer_addr);
    auto ptr = static_cast<const std::byte*>(addr);
    
    if(
      ( ptr < base_ptr) 
      || ptr > base_ptr + local_dataspaceOwner.get_size())
    {
      std::printf("Error , the address is not in the local data space \nplease use only the address allocated using allocate_local\n");
      return -1;
    }
    l4_size_t write_index = ptr - base_ptr ;
    peer_owner_intf->free_your_data(id, type, write_index, size);
    return 0;
  }

  std::atomic<bool> all_is_ready{false};

};
