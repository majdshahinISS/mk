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

#include "DataspaceOwner.hpp"


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
      p->peer_is_ready.store(true);
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
