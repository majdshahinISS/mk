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
#include "DataspaceOwner.hpp"
#include "LocalMemoryManager.hpp"


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
      int rc = pthread_create(&th_peer_intf, &attr, server_intf_getter, (void*)this);
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
      int rc = pthread_create(&th_local_mem, &attr, localMemoryManager_initializer_th, (void*)this);
      return rc;
    }

    static void * free_peer_req_callback_thread(void * arg)
    {
      WorkerArgs * p = (WorkerArgs*) arg;

      std::printf("free_peer_req_callback , id: %lu, type %d, on local address %p\n", p->id,p->type, p->addr);

      p->ds->lmm.free_local((void* )p->addr);
      delete p;
      return nullptr;
    }
    int free_peer_req_callback(u_int64_t id ,u_int8_t type, l4_size_t write_index, l4_size_t size)
    {
      u_int8_t * addr = ((u_int8_t *)local_dataspaceOwner.get_pointer())+write_index;
      WorkerArgs *p = new WorkerArgs{this, id, type, addr, size};

      pthread_t tid;
      int rc = pthread_create(&tid, &attr, free_peer_req_callback_thread, (void*)p);
      return rc;
    }
    int new_data_callback_priv(u_int64_t id ,u_int8_t type, l4_size_t write_index, l4_size_t size)
    {
      if(all_is_ready.load() == false)
        return -1;

      u_int8_t * read_addr = ((u_int8_t *)peer_addr) + write_index;      
      if ( new_req_callback_handler == nullptr)
      {
        std::printf(" new_req_callback_handler is null... using the default\n");
        return new_data_callback_default(id , type, read_addr, size);
      }
      else
        return new_req_callback_handler(this, id, type, read_addr, size);
    }
    
    int new_data_callback_default(u_int64_t id, u_int8_t type, u_int8_t * read_addr , l4_size_t size)
    {
      // run in a thread ! 
      std::printf("read from address: %p:%s\n",read_addr, read_addr);
      sleep(1);
      std::printf("request peer to free his data\n");
      int res = free_peer_req(id,type, read_addr, size);
      return res;
    }

  public:
  DataspaceEndpoint(
    L4Re::Util::Registry_server<> *server,
    const char *CTS_ipc_name,
    const char *STC_ipc_name,
    l4_size_t   peer_size,
    u_int64_t    peer_timeout,
    std::function<int(DataspaceEndpoint* obj,u_int64_t id ,u_int8_t type, const u_int8_t * addr, l4_size_t size)> new_req_callback_handler_= nullptr
  ): 
      local_dataspaceOwner(server, CTS_ipc_name),
      peer_size(peer_size), 
      peer_timeout(peer_timeout),
      new_req_callback_handler(new_req_callback_handler_)
  {
    g_main_tid = pthread_self();   // remember main thread id
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    local_dataspaceOwner.set_free_peer_req_callback(
      [this](u_int64_t id ,u_int8_t type, l4_size_t write_index, l4_size_t size) -> int {
      return free_peer_req_callback(id, type, write_index, size);}
    );


    local_dataspaceOwner.set_new_req_callback(
      [this]( u_int64_t id ,u_int8_t type, l4_size_t write_index, l4_size_t size) -> int {
          return new_data_callback_priv(id, type, write_index, size);          
      }
    );
    

    peer_owner_intf = L4Re::Env::env()->get_cap<IDataspaceOwner>(STC_ipc_name);
    peer_ds = L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();

    // @TODO check
    start_geting_server_intf();
    start_init_LocalMemoryManager();
  }

  ~DataspaceEndpoint()
  {
    pthread_attr_destroy(&attr);
  }
  void * allocate_local( l4_size_t size)
  {
    if ((local_is_ready.load()==false) || size > local_dataspaceOwner.get_size() )
      return nullptr;

    return lmm.allocate_local(size);
  }

  void * allocate_local_blocking(l4_size_t size)
  {
    
    if( size > local_dataspaceOwner.get_size())
      return nullptr;
    
    return lmm.allocate_local_wait(size);
  }

private:
  static void * add_new_req_worker(void * arg)
  {
    WorkerArgs * p = (WorkerArgs*) arg;
    u_int8_t* base_ptr = (u_int8_t*)(p->ds->local_dataspaceOwner.get_pointer());

    l4_size_t write_index = p->addr - base_ptr ;
    p->ds->peer_owner_intf->new_req(p->id, p->type, write_index, p->size);
    delete p;
    return nullptr;
  }

public:
  // the address must be in local dataspace using LocalMemeoryManager
  int add_new_req(u_int64_t id ,u_int8_t type, void * addr, l4_size_t size)
  {
    std::printf("sending new req. to peer , id %lu, type %d local addr %p , size %lu \n", id, type, addr, size);
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
    WorkerArgs * p = new WorkerArgs{this, id, type, (u_int8_t*)ptr, size};
    pthread_t tid;
    int rc = pthread_create(&tid, &attr, add_new_req_worker, (void*)p);
    return rc;
  }


private:
  static void * free_peer_req_worker(void* arg)
  {
    WorkerArgs * p = (WorkerArgs*) arg;
    const u_int8_t * base_ptr = (const u_int8_t *)(p->ds->peer_addr);

    l4_size_t write_index = p->addr- base_ptr ;
    p->ds->peer_owner_intf->free_your_data(p->id, p->type, write_index, p->size);
    
    delete p;
    return nullptr;
  }
public:
  int free_peer_req(u_int64_t id ,u_int8_t type, const void * addr, l4_size_t size)
  {
    std::printf("free_peer_req, id: %lu\n",id);
    if(peer_is_ready.load() == false)
    {
      std::printf("Error, the peer is not ready yet");
    }
    auto base_ptr = static_cast<const std::byte*>(peer_addr);
    auto ptr = static_cast<const std::byte*>(addr);
    
    if(
      ( ptr < base_ptr) 
      || ptr > base_ptr + peer_size)
    {
      std::printf("Error , the address is not in the local data space \nplease use only the address allocated using allocate_local\n");
      return -1;
    }
    pthread_t th;
    WorkerArgs * p = new WorkerArgs{this, id, type, (const u_int8_t*)addr, size};
    int rc = pthread_create(&th, &attr, free_peer_req_worker, (void*)p);
    return rc;
  }

  bool wait_for_initialization(int timeout)
  {
    assert(!pthread_equal(pthread_self(), g_main_tid) && "wait_for_initialization must not run on the main thread");

    int i = 0;
    while(all_is_ready.load()== false)
    {
      usleep(1000);
      if(timeout != 0)
      {
        i++;
        if (i >= timeout) 
        {
          return false;
        }
      }
    }
    return true;
  }

};

