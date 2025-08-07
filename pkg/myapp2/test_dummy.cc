#include <l4/re/dataspace>
#include <l4/re/util/cap_alloc>
#include <l4/re/env>
#include <l4/sys/err.h>
#include <l4/sys/consts.h>

#include <l4/re/dma_space>
#include <l4/re/util/unique_cap>
#include <l4/sys/factory>
#include <l4/sys/capability>
#include <l4/re/error_helper>
#include <l4/sys/l4int.h>


#include <iostream>
#include <cstring> // for strncpy

#include "Tests.h"

static void test_dataspace() {
    using namespace L4Re;
    using namespace L4;

    // Allocate a 4KB dataspace
    const unsigned long size = 4096; // Use unsigned long instead of l4_size_t
    Cap<Dataspace> ds = Util::cap_alloc.alloc<Dataspace>();
    
    if (!ds.is_valid()) {
        std::cerr << "Capability allocation failed\n";
        return;
    }
    else
        std::cout<< "Capability allocated ok\n";

    // Allocate memory for the dataspace
    if (int ret = Env::env()->mem_alloc()->alloc(size, ds); ret < 0) {
        std::cerr << "Dataspace allocation failed: " << ret << "\n";
        return;
    }
    else
        std::cout<< "Dataspace allocated ok!\n";

    // Map the dataspace
    void *addr = nullptr;
    L4Re::Rm::Flags tflags;
    tflags.raw = 0 ;
    tflags |= L4Re::Rm::F::Search_addr; // Search for a free address
    tflags |= L4Re::Rm::F::Eager_map;
    tflags |= L4Re::Rm::F::RWX; // Read, Write, Execute permissions
    tflags |= L4Re::Rm::F::Cache_normal; // Normal cacheable memory
    tflags |= L4Re::Rm::F::Detach_free; // Free the portion of the data space after detach
    tflags |= L4Re::Rm::F::Cache_normal; // Normal cacheable memory
    tflags |= L4Re::Rm::F::Detach_free; // Free the portion of the data space after detach


    if (int ret = Env::env()->rm()->attach(&addr, size, 
        tflags, ds, 0, 21); ret < 0) {
        std::cerr << "Dataspace attach failed: " << ret << "\n";
        return;
    }
    else
        std::cout<< "Dataspace attach ok!\n";

    // Use the dataspace
    char *data = static_cast<char*>(addr);
    const char *message = "L4Re Dataspace test";
    strncpy(data, message, strlen(message) + 1);
    std::cout << "Dataspace content: " << data << "\n";

    // Cleanup
    Env::env()->rm()->detach(addr, nullptr);
    // Capability will be automatically freed when 'ds' goes out of scope
    std::cout << "Dataspace test completed\n";
}

static void dummy()
{
    // L4Re::Dma_space dma = L4.Env.dma_space,
    
    std::cout << "dummy function" << std::endl;

    int r;
    auto x = L4Re::chkcap(L4Re::Util::make_unique_cap < L4Re::Dma_space > ());
      r = l4_error(L4Re::Env::env()->user_factory()->create(x.get()));
    if (r != 0)
    {
        std::cerr << "Error creating DMA space: " << std::endl;
        return;
    }
    else
    {
        std::cout << "DMA space created successfully." << std::endl;
    }
}

#include <l4/re/util/cap_alloc>
#include <l4/re/dataspace>
#include <l4/re/env>

void use_configured_dataspace() {
    // Get the capability from the environment
    L4::Cap<L4Re::Dataspace> ds = L4Re::Env::env()->get_cap<L4Re::Dataspace>("static_ds");
    
    if (!ds.is_valid()) {
        std::cerr << "Dataspace capability not valid\n";
        // Handle error - capability not available
        return;
    }
    else
        std::cout<< "Dataspace capability ok!\n";

    // Use the dataspace
    void *addr = nullptr;
    L4Re::Rm::Flags tflags;
    tflags.raw = 0 ;
    tflags |= L4Re::Rm::F::Search_addr; // Search for a free address
    tflags |= L4Re::Rm::F::Eager_map;
    tflags |= L4Re::Rm::F::RWX; // Read, Write, Execute permissions
    tflags |= L4Re::Rm::F::Cache_normal; // Normal cacheable memory
    tflags |= L4Re::Rm::F::Detach_free; // Free the portion of the data space after detach
    tflags |= L4Re::Rm::F::Cache_normal; // Normal cacheable memory
    tflags |= L4Re::Rm::F::Detach_free; // Free the portion of the data space after detach
    int ret = L4Re::Env::env()->rm()->attach(&addr, ds->size(), tflags, ds);
    if (ret < 0) {
        // Handle attachment error
        std::cerr << "Error attaching dataspace: " << ret << std::endl;
        return;
    }
    else
        std::cout<< "Dataspace attach ok!\n";

    // Now you can use the memory...
}




#include <l4/re/env>
#include <l4/re/dataspace>
// #include <l4/re/utcb_cap>
#include <l4/re/mem_alloc>
#include <l4/sys/types.h>

// #include <l4/sys/kmem.h>
#include <cstring>
#include <cstdio>

#include <iostream>
#include <l4/sys/capability> 
#include <l4/re/log>
#include <l4/re/env>
#include <l4/sys/consts.h>


#include <l4/re/rm>
#include <l4/re/util/cap_alloc>
#include <l4/sys/err.h>
int data_space_test1()
{
    
    // Get the environment interface
    const L4Re::Env *env = L4Re::Env::env();
    if (!env) {
        std::cerr << "Failed to get environment" << std::endl;
        return 1;
    }
    std::cout << "Environment obtained successfully." << std::endl;
    
    // Get memory allocator
    L4::Cap<L4Re::Mem_alloc> mem_alloc = env->mem_alloc();
    
    // Allocate a dataspace of 4096 bytes (1 page)
    L4::Cap<L4Re::Dataspace> ds = L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();
    if (!ds.is_valid()) {
        std::cerr << "Failed to allocate dataspace capability\n";
        return 1;
    }
    else
        std::cout<< "Dataspace capability ok!\n";
    // Allocate memory for the dataspace
    if (long ret = mem_alloc->alloc(4096, ds); ret < 0) {
        std::cerr << "Failed to allocate dataspace: " << ret << "\n";
        return 1;
    }
    else
        std::cout<< "Dataspace allocated ok!\n";

    
    // Attach the dataspace to current address space
    void *addr = nullptr;
    long r = env->rm()->attach(&addr, 4096,                                           
        L4Re::Rm::F::Search_addr | L4Re::Rm::F::RW,
        L4::Ipc::make_cap_rw(ds), 0,
        0 & L4Re::Mem_alloc::Super_pages   ? L4_SUPERPAGESHIFT : L4_PAGESHIFT
        );
    if (r < 0) {
        printf("Failed to attach dataspace: %ld\n", r);
        return 1;
    }
    else
        printf("Dataspace attached at address %p\n", addr);
    
    // Write data to the dataspace
    const char *msg = "Hello from Dataspace!";
    std::strcpy(static_cast<char *>(addr), msg);
    
    // Read it back
    printf("Dataspace content: %s\n", static_cast<char *>(addr));
    
    // Detach (optional, normally OS will clean up on exit)
    env->rm()->detach(addr, nullptr);
    
    return 0;
      
}


void test_dummy()
{
   
    data_space_test1();
    //dummy();

}