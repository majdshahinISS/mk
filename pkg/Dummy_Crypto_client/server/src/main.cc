#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <l4/sys/ipc.h>
#include <l4/sys/utcb.h>


#include <l4/sys/err.h>
#include <l4/sys/types.h>
#include <l4/re/env>
#include <l4/re/util/cap_alloc>
 
#include <stdio.h>

#include <l4/util/util.h>
#include "/home/iss/L4Re_d/l4/pkg/user_shared/crypto_shared.h"

struct Crypto_Request {
  l4_size_t write_index;
  l4_size_t size;
  l4_uint8_t type; // 1: CTS, 2: STC
  l4_uint64_t id;
  l4_uint64_t req_timestamp;
  l4_uint8_t tries;
};

class Client_{
public:
  Client_(const char * ipc_name)
  {
    crypto_ipc = L4Re::Env::env()->get_cap<ICrypto>(ipc_name);
    if (!crypto_ipc.is_valid()) {
      std::printf("Failed to get crypto_ipc capability\n");
      ready = false; return ;
    }
    int x = 0;
    crypto_ipc->dummy(x );
    std::printf("dummy returned x = %d\n", x); // ok it prints 3

    ds_CTS = L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();
    size_CTS = 0;
    ds_STC = L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();
    size_STC = 0;

    
    p_CTS = nullptr;
    p_STC = nullptr;
    if (!attach_ds(ds_CTS, &p_CTS, &size_CTS)) {
      std::printf("Client: attached CTS dataspace at %p, size=%lu bytes\n",
                  p_CTS, static_cast<unsigned long>(size_CTS));
    } else {
      std::printf("Client: failed to attach CTS dataspace\n");
      ready = false; return;
    }
    if (!attach_ds(ds_STC, &p_STC, &size_STC)) {
      std::printf("Client: attached STC dataspace at %p, size=%lu bytes\n",
                  p_STC, static_cast<unsigned long>(size_STC));
    } else {
      std::printf("Client: failed to attach STC dataspace\n");
      ready = false; return;
    }
    if (!crypto_ipc.is_valid()) {
      std::printf("Client: invalid crypto capability\n");
      ready = false; return;
    }
    ready = true;
  }
  bool is_ready() const { return ready; }
  private:
    L4::Cap<ICrypto> crypto_ipc;
    L4::Cap<L4Re::Dataspace> ds_CTS; // client to server
    l4_size_t size_CTS;
    void * p_CTS; // pointer to attached CTS dataspace
    L4::Cap<L4Re::Dataspace> ds_STC; // server to client
    l4_size_t size_STC;
    void * p_STC; // pointer to attached STC dataspace
    bool ready = false;
};

int
main()
{
  sleep(1);
  Client_ client("crypto_ipc");
  if (!client.is_ready()) {
    std::printf("Client not ready\n");
    return 1;
  }



  // attach the dataspace
  void *addr = nullptr;
  l4_size_t size = ds->size();
  long err = L4Re::Env::env()->rm()->attach(
      &addr, size,
      L4Re::Rm::F::Search_addr | L4Re::Rm::F::RW,   // find VA, map RW
      L4::Ipc::make_cap(ds, L4_CAP_FPAGE_RW));                    // grant RW rights
  if (err < 0) {
    std::printf("attach_ds: attach failed (%ld)\n", err);
    return 1;
  }
  int i = 0;
  l4_size_t write_index = 0;
  while (addr) {
    std::printf("Client: writing to dataspace at %p, size=%lu\n",
                addr, static_cast<unsigned long>(size));
    char buffer[100];
    std::snprintf(buffer, sizeof(buffer), "Hello from Client %d", i++);
    l4_size_t str_len = std::strlen(buffer);

    if( write_index + str_len +1 > size) {
      std::printf("Client: no more space in dataspace\n");
      break;
    }

    std::strcpy(static_cast<char*>(addr + write_index),  buffer);

    crypto->CTS_ready(i,1,write_index, str_len); // notify server that client to server is ready
    write_index += ((str_len+1 +3)/4)*4; // align to 4 bytes

    sleep(1);
    if (i == 5) break;
  }
  return 0;
}