#include <l4/re/env>
#include <l4/sys/arm_smccc>
#include <cstdio>

static inline l4_umword_t zmp_fid(unsigned pm_api_id)
{
  // SMC64 | fastcall | SiP owner | pm_api_id
  const l4_umword_t SMC_64   = 1ul << 31;
  const l4_umword_t FASTCALL = 1ul << 30;
  const l4_umword_t OEN_SIP  = 0x02ul << 24;
  return FASTCALL | SMC_64 | OEN_SIP | (pm_api_id & 0xFFFF);
}

int main()
{
  auto smc = L4Re::Env::env()->get_cap<L4::Arm_smccc>("smc");
  if (!smc.is_valid()) {
    printf("SMC cap 'smc' invalid. Pass it via .cfg: caps = { smc = L4.Env.arm_smccc }\n");
    return 1;
  }

  l4_umword_t out[4] = {~0ul, 0, 0, 0};
  unsigned long flags = 0; // 0 = SMC (use HVC if your setup needs it)

  // ---- PM_GET_API_VERSION (id = 1), no args
  l4_umword_t func = zmp_fid(1);
  auto tag = smc->call(func, 0, 0, 0, 0, 0, 0,
                       &out[0], &out[1], &out[2], &out[3], flags);
  if (l4_ipc_error(tag, l4_utcb())) {
    printf("SMC IPC error (label=%lu words=%u)\n",
           (unsigned long)tag.label(), tag.words());
    return 1;
  }
  unsigned status = (unsigned)out[0];
  unsigned maj = (unsigned)(out[1] >> 16);
  unsigned min = (unsigned)(out[1] & 0xFFFF);
  printf("PM_GET_API_VERSION: status=%u ver=%u.%u\n", status, maj, min);

  // ---- PM_GET_CHIPID (id = 24), no args
  func = zmp_fid(24);
  tag = smc->call(func, 0, 0, 0, 0, 0, 0,
                  &out[0], &out[1], &out[2], &out[3], flags);
  if (l4_ipc_error(tag, l4_utcb())) {
    printf("SMC IPC error on CHIPID\n");
    return 1;
  }
  status = (unsigned)out[0];
  printf("PM_GET_CHIPID: status=%u id_hi=0x%08lx id_lo=0x%08lx\n",
         status, (unsigned long)out[1], (unsigned long)out[2]);

  return (int)status;
}
