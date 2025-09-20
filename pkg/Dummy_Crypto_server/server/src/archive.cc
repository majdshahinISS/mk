#if 0

/*
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


/*

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
      L4Re::Rm::F::Search_addrstatic void *server_loop_th(void *arg)
{
  puts("server_loop_th: starting");
  server.loop(l4_utcb());              // should never return
  puts("server_loop_th: loop returned (error)"); // if it does, print it
  return nullptr;
}
  pthread_t t;

int start_server_loop()
{
  pthread_attr_t attr;
  pthread_attr_init(&attr);

  // Bump stack: 64 KiB is a good start on L4Re
  pthread_attr_setstacksize(&attr, 64 * 1024);

  int rc = pthread_create(&t, &attr, server_loop_th, nullptr);
  pthread_attr_destroy(&attr);

  if (rc != 0) {
    printf("pthread_create failed: %d\n", rc);   // EAGAIN => resources
    return rc;
  }

 // pthread_detach(t);  // fire-and-forget
  return 0;
} | L4Re::Rm::F::RW,   // find VA, map RW
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
  /*printf("Welcome to the Crypto server!\n"
         "I can provide a shared dataspace.\n");*/


 
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

//#include <l4/irq/irq.h>
#include <l4/util/util.h>
#include <stdio.h>
#include <pthread-l4.h>
static void *server_loop_th2(void *data)
{
  L4::Cap<IDataspaceOwner> * p_dss = (L4::Cap<IDataspaceOwner> *)data;
  std::printf("serverloop client\n");
  //server.loop();

  // get interface to the dataspace owner of the server side
  *p_dss =L4Re::Env::env()->get_cap<IDataspaceOwner>(CTS_ipc_name);
  if (!(*p_dss).is_valid()) {
    std::printf("Failed to get dss capability\n");
    return nullptr;
  }
  (*p_dss)->init(4096, 1000); // create a dataspace of 4096 bytes
  
  return 0;
}
#include <l4/sys/utcb.h>
static void *server_loop_th(void *arg)
{
  puts("server_loop_th: starting");
  server.loop(l4_utcb());              // should never return
  puts("server_loop_th: loop returned (error)"); // if it does, print it
  return nullptr;
}
  pthread_t t;

int start_server_loop()
{
  pthread_attr_t attr;
  pthread_attr_init(&attr);

  // Bump stack: 64 KiB is a good start on L4Re
  pthread_attr_setstacksize(&attr, 64 * 1024);

  int rc = pthread_create(&t, &attr, server_loop_th, nullptr);
  pthread_attr_destroy(&attr);

  if (rc != 0) {
    printf("pthread_create failed: %d\n", rc);   // EAGAIN => resources
    return rc;
  }

 // pthread_detach(t);  // fire-and-forget
  return 0;
}


#endif