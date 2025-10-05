#include <l4/re/env>
#include <l4/sys/arm_smccc>
#include <cstdio>
#include<iostream>
#include <limits> // For std::numeric_limits

L4::Cap<L4::Arm_smccc> smc;
static inline l4_umword_t zmp_fid(unsigned pm_api_id)
{
  // SMC64 | fastcall | SiP owner | pm_api_id
  const l4_umword_t SMC_64   = 1ul << 31;
  const l4_umword_t FASTCALL = 1ul << 30;
  const l4_umword_t OEN_SIP  = 0x02ul << 24;
  return FASTCALL | SMC_64 | OEN_SIP | (pm_api_id & 0xFFFF);
}

/*
void shutdown(l4_umword_t restart = 0) // 0 = shutdown , 1 = reboot
{
    l4_umword_t out[4] = {~0ul,0,0,0};
unsigned long flags = 0; // 0 => SMC

// PM_SYSTEM_SHUTDOWN API id = 12
l4_umword_t func = zmp_fid(12);
std::cout << "shutdown func id is " <<std::hex<< func << std::endl;

auto tag = smc->call(func, restart, 1, 0, 0, 0, 0,
                       &out[0], &out[1], &out[2], &out[3], flags);
  if (l4_ipc_error(tag, l4_utcb())) {
    printf("SMC IPC error on SYSTEM_SHUTDOWN (shutdown)\n");
    std::cout << "SMC IPC error on SYSTEM_SHUTDOWN (shutdown)\n";
  } else {
    printf("PM_SYSTEM_SHUTDOWN shutdown returned status=%lu\n", (unsigned long)out[0]);
    
  }

}
void psci_version()
{
  l4_umword_t fid = 0x84000000ul; // PSCI_VERSION (Fast call, 32-bit)
  l4_umword_t out[4] = {~0ul, 0, 0, 0};
  unsigned long flags = 0; // 0 = SMC (not HVC)

  // Perform the SMC call
  auto tag = smc->call(fid, 0, 0, 0, 0, 0, 0,
                       &out[0], &out[1], &out[2], &out[3], flags);

  // Check IPC status
  if (!tag.has_error()) {
    l4_uint64_t status = out[0];

    if (status >> 31) { // Negative value in SMCCC means error (in 2's complement)
      printf("PSCI_VERSION failed: status=0x%llx\n", status);
    } else {
      // Format version: bits [15:8] = Major, [7:0] = Minor
      int major = (status >> 16) & 0xffff;
      int minor = status & 0xffff;
      printf("PSCI_VERSION success: version = %d.%d (0x%llx)\n", major, minor, status);
    }
  } else {
        printf("SMC IPC error (label=%lu words=%u)\n",
           (unsigned long)tag.label(), tag.words());
  }

}

void get_pm_version()
{
  l4_umword_t out[4] = {~0ul, 0, 0, 0};
  unsigned long flags = 0; // 0 = SMC (use HVC if your setup needs it)

  // ---- PM_GET_API_VERSION (id = 1), no args
  l4_umword_t func = zmp_fid(1);
  std::cout << "func id PM_GET_API_VERSION is " <<std::hex<< func << std::endl;

  auto tag = smc->call(func, 0, 0, 0, 0, 0, 0,
                       &out[0], &out[1], &out[2], &out[3], flags);
  if (l4_ipc_error(tag, l4_utcb())) {
    printf("SMC IPC error (label=%lu words=%u)\n",
           (unsigned long)tag.label(), tag.words());
    return ;
  }
  unsigned status = (unsigned)out[0];
  unsigned maj = (unsigned)(out[1] >> 16);
  unsigned min = (unsigned)(out[1] & 0xFFFF);
  printf("PM_GET_API_VERSION: status=%u ver=%u.%u\n", status, maj, min);

}

void get_chipid()
{
  l4_umword_t out[4] = {~0ul,0,0,0};
  unsigned long flags = 0; // 0 => SMC

  // ---- PM_GET_CHIPID (id = 24), no args
  l4_umword_t func = zmp_fid(24);
  std::cout << "func id is " <<std::hex<< func << std::endl;
  auto tag = smc->call(func, 0, 0, 0, 0, 0, 0,
                  &out[0], &out[1], &out[2], &out[3], flags);
  if (l4_ipc_error(tag, l4_utcb())) {
    printf("SMC IPC error on CHIPID\n");
    return;
  }
  unsigned status = (unsigned)out[0];
  printf("PM_GET_CHIPID: status=%u id_hi=0x%08lx id_lo=0x%08lx\n",
         status, (unsigned long)out[1], (unsigned long)out[2]);
}
*/
int main()
{
    // Build the SMCCC cap from the base selector
  /*auto smc_from_base = L4::cap_cast<L4::Arm_smccc>(L4::Cap<void>(L4_BASE_ARM_SMCCC_CAP));
  if (!smc_from_base.is_valid()) {
    printf("SMCCC base cap not present. Recheck ARM_SMC_USER in kernel config.\n");
    return 1;
  }
  else 
  {
    printf("smc_from_base cap presents ok \n");
  }
  */


  L4::Cap<L4::Arm_smccc> smc = L4Re::Env::env()->get_cap<L4::Arm_smccc>("smc");
  if (!smc.is_valid()) {
    printf("SMC cap 'smc' invalid. Pass it via .cfg: caps = { smc = L4.Env.arm_smc }\n");
    return 1;
  }
  else 
  {
    printf("cap presents ok\n");
  }
  l4_umword_t fid = 0x84000000ul; // PSCI_VERSION (Fast call, 32-bit)
  l4_umword_t out[4] = {~0ul, 0, 0, 0};
  unsigned long flags = 0; // 0 = SMC (not HVC)

  // Perform the SMC call
  auto tag = smc->call(fid, 0, 0, 0, 0, 0, 0,
                       &out[0], &out[1], &out[2], &out[3], flags);

  // Check IPC status
  if (!tag.has_error()) {
    l4_uint64_t status = out[0];

    if (status >> 31) { // Negative value in SMCCC means error (in 2's complement)
      printf("PSCI_VERSION failed: status=0x%llx\n", status);
    } else {
      // Format version: bits [15:8] = Major, [7:0] = Minor
      int major = (status >> 16) & 0xffff;
      int minor = status & 0xffff;
      printf("PSCI_VERSION success: version = %d.%d (0x%llx)\n", major, minor, status);
    }
  } else {
        printf("SMC IPC error (label=%lu words=%u)\n",
           (unsigned long)tag.label(), tag.words());
  }
  std::cout << "If you see this message the shutdown/restart did not work"<<std::endl;
  return 0;
}

/*
in u-boot , smc is working
smc 0xc2000001 
Res: 0x1000100000000 0x0 0x0 0x0 

smc 0x84000000 
Res: 0x10001 0x0 0x0 0x0

*/