#ifndef __CRYPTO_SHARED__
#define __CRYPTO_SHARED__

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

#endif // __CRYPTO_SHARED_