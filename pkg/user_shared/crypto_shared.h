#ifndef __CRYPTO_SHARED__
#define __CRYPTO_SHARED__

#include <l4/sys/capability>
#include <l4/sys/cxx/ipc_iface>
 
struct Calc : L4::Kobject_t<Calc, L4::Kobject, 0x45>
{
  L4_INLINE_RPC(int, sub, (l4_uint32_t a, l4_uint32_t b, l4_uint32_t *res));
  L4_INLINE_RPC(int, neg, (l4_uint32_t a, l4_uint32_t *res));
  typedef L4::Typeid::Rpcs<sub_t, neg_t> Rpcs;
};

#endif // __CRYPTO_SHARED_