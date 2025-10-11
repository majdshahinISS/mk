#include <l4/re/env>
#include <l4/sys/arm_smccc>
#include <l4/sys/arm_smccc.h>

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

void shutdown(l4_umword_t restart = 0) ;// 0 = shutdown , 1 = reboot
void psci_version();
void get_pm_version();
void get_chipid();
void smc_command(l4_umword_t func);
l4_umword_t smc_command_check(l4_umword_t func)  ;
int perform_smc64_example(L4::Cap<L4::Arm_smccc> smc_cap) ;
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


  smc = L4Re::Env::env()->get_cap<L4::Arm_smccc>("smc");
  if (!smc.is_valid()) {
    printf("SMC cap 'smc' invalid. Pass it via .cfg: caps = { smc = L4.Env.arm_smc }\n");
    return 1;
  }
  else 
  {
    printf("cap presents ok\n");
  }
  psci_version();
  /*std::cout<<"Start to call smc commands" << std::endl;
  for (l4_umword_t func_id = 0xc2000000; func_id <= 0xc20000FF; func_id++) {
      smc_command_check(func_id);
  }*/
  perform_smc64_example(smc);
  get_chipid();
  get_pm_version();
  //shutdown(0); // shutdown
  shutdown(1); // reboot
  
  return 0;
}

/*
change in fiasco/src/kern/arm/smc_user.cpp
    // only allow calls in configured service call range
    //if (   (r0 & 0x3F000000) < CONFIG_ARM_SMC_USER_MIN
     //   || (r0 & 0x3F000000) > CONFIG_ARM_SMC_USER_MAX)
     to

    if (   (r0  < CONFIG_ARM_SMC_USER_MIN )
        || (r0 > CONFIG_ARM_SMC_USER_MAX))

    in fiasco config, set
      CONFIG_ARM_SMC_USER_MIN=0x80000000
      CONFIG_ARM_SMC_USER_MAX=0xC200FFFF
*/


void get_chipid()
{
  l4_umword_t out[4] = {~0ul,0,0,0};
  unsigned long flags = 0; // 0 => SMC

  // ---- PM_GET_CHIPID (id = 24), no args
  l4_umword_t func = 0xc2000018ul; // PM_GET_CHIPID (Fast call, 64-bit)
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


void get_pm_version()
{
  l4_umword_t out[4] = {~0ul, 0, 0, 0};
  unsigned long flags = 0; // 0 = SMC (use HVC if your setup needs it)

  // ---- PM_GET_API_VERSION (id = 1), no args
  l4_umword_t func = 0xc2000001ul; // PM_GET_API_VERSION (Fast call, 64-bit)
  std::cout << __FUNCTION__ <<": func id is " <<std::hex<< func << std::endl;

  auto tag = smc->call(func, 0, 0, 0, 0, 0, 0,
                       &out[0], &out[1], &out[2], &out[3], flags);
  if (l4_ipc_error(tag, l4_utcb())) {
    printf("SMC IPC error (label=%lu words=%u)\n",
           (unsigned long)tag.label(), tag.words());
    return ;
  }
  std::cout << __FUNCTION__ << "output is " <<std::hex<< out[0] << " " << out[1] << " " << out[2] << " " << out[3] << std::endl;
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
      printf("%s PSCI_VERSION success: version = %d.%d (0x%llx)\n", __FUNCTION__ , major, minor, status);
    }
  } else {
        printf("SMC IPC error (label=%lu words=%u)\n",
           (unsigned long)tag.label(), tag.words());
  }
}


void 
shutdown(l4_umword_t restart) // 0 = shutdown , 1 = reboot
{
    l4_umword_t out[4] = {~0ul,0,0,0};
unsigned long flags = 0; // 0 => SMC

// PM_SYSTEM_SHUTDOWN API id = 12
l4_umword_t func = 0xc200000cul;
std::cout << __FUNCTION__  << "shutdown func id is " <<std::hex<< func << std::endl;

auto tag = smc->call(func, restart, 1, 0, 0, 0, 0,
                       &out[0], &out[1], &out[2], &out[3], flags);
  if (l4_ipc_error(tag, l4_utcb())) {
    printf("SMC IPC error on SYSTEM_SHUTDOWN (shutdown)\n");
    std::cout << "SMC IPC error on SYSTEM_SHUTDOWN (shutdown)\n";
  } else {
    printf("%s shutdown returned status=%lu\n", __FUNCTION__ , (unsigned long)out[0]);
    
  }
}


void smc_command(l4_umword_t func)  
{
    l4_umword_t out[4] = {~0ul,0,0,0};
    std::cout<<"func id is " <<std::hex<< func << std::endl;
    unsigned long flags = 0; // 0 => SMC
    auto tag = smc->call(func, 0, 0, 0, 0, 0, 0,
                       &out[0], &out[1], &out[2], &out[3], flags);
    if (l4_ipc_error(tag, l4_utcb())) {
        printf("SMC IPC error on SMC command\n");
    } else {
        std::cout << __FUNCTION__ << "output is " <<std::hex<< out[0] << " " << out[1] << " " << out[2] << " " << out[3] << std::endl;
    }
}

l4_umword_t smc_command_check(l4_umword_t func)  
{
    l4_umword_t out[4] = {~0ul,0,0,0};
    unsigned long flags = 0; // 0 => SMC
    auto tag = smc->call( 0x80000001,func, 0, 0, 0, 0, 0,
                       &out[0], &out[1], &out[2], &out[3], flags);
    if (l4_ipc_error(tag, l4_utcb())) {
        printf("SMC IPC error on SMC command:\n");
        return -1;
    } else {
        if (out[0] == 0) {
            std::cout << "func id " <<std::hex<< func << " is supported" << std::endl;
        } else {
            std::cout << "func id " <<std::hex<< func << " is NOT supported: 0x" <<out[0] << std::endl;
        }
        return out[0];
    }
}

#define XILINX_PM_GET_API_VERSION 0xC2000001

int perform_smc64_example(L4::Cap<L4::Arm_smccc> smc_cap) 
{
    l4_umword_t out0, out1, out2, out3;
    int ret_val;

    // Perform the SMC64 call
    // For PM_GET_API_VERSION, inputs in0-in5 are typically zero.
    l4_msgtag_t tag = smc_cap->call(XILINX_PM_GET_API_VERSION,
                                    0, 0, 0, 0, 0, 0, // in0 through in5
                                    &out0, &out1, &out2, &out3,
                                    0); // client_id

    ret_val = l4_error(tag);
    if (ret_val != 0) {
        // Handle L4 IPC error
        return ret_val;
    }

    std::cout << "SMC64 call returned: " << std::hex
              << " out0=0x" << out0
              << " out1=0x" << out1
              << " out2=0x" << out2
              << " out3=0x" << out3
              << std::dec << std::endl;

    // Success! out0 now contains the PM API version.
    // You can check out1, out2, out3 for other function-specific returns.
    return 0;
}