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



#endif // __CRYPTO_SHARED_