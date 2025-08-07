#include "Tests.h"

#include <iostream>
#include <l4/sys/capability> 
#include <l4/re/log>
#include <l4/re/env>

void test_log()
{
    L4::Cap<L4Re::Log> log = L4Re::Env::env()->log();
    if(!log.is_valid())
    {
        std::cerr << "log_test: Failed to get log capability" << std::endl;
        return ;
    }
    else
        log->print("log_test: log capability is valid\n");

    
    l4_icu_info_t *info = new l4_icu_info_t;
    log->info(info);
    if (info->features & L4_ICU_FLAG_MSI)
        log->print("log_test: ICU supports MSI\n");
    else
        log->print("log_test: ICU does not support MSI\n");
    printf("log_test: ICU has %u IRQ lines\n", info->nr_irqs);
    printf("log_test: ICU has %u MSI lines\n", info->nr_msis);
    delete info;
}