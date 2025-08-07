#include "Tests.h"

#include <l4/re/mem_alloc>
#include <l4/re/rm>
#include <l4/re/env>
#include <l4/re/dataspace>
#include <l4/re/util/cap_alloc>
#include <l4/sys/err.h>
#include <cstdio>
#include <cstring>
 
static int allocate_mem(unsigned long size_in_bytes, unsigned long flags,
                        void **virt_addr)
{
    int r;
    L4::Cap<L4Re::Dataspace> d;

    /* Allocate a free capability index for our data space */
    d = L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();
    if (!d.is_valid())
    {
        printf("Failed to allocate dataspace capability\n");
        return -L4_ENOMEM;
    }
    else
        printf("Dataspace capability ok!\n");

    size_in_bytes = l4_trunc_page(size_in_bytes);

    /* Allocate memory via a dataspace */
    if ((r = L4Re::Env::env()->mem_alloc()->alloc(size_in_bytes, d, flags)))
    {
        printf("Failed to allocate dataspace: %d\n", r);
        return r;
    }
    else
        printf("Dataspace allocated ok!\n");
    /* Make the dataspace visible in our address space */
    *virt_addr = 0;
    if ((r = L4Re::Env::env()->rm()->attach(virt_addr, size_in_bytes,
                                            L4Re::Rm::F::Search_addr | L4Re::Rm::F::RW,
                                            L4::Ipc::make_cap_rw(d), 0,
                                            flags & L4Re::Mem_alloc::Super_pages
                                                ? L4_SUPERPAGESHIFT : L4_PAGESHIFT)))
    {
        printf("Failed to attach dataspace: %d\n", r);
        return r;
    }
    else
        printf("Dataspace attached ok!\n");

    /* Done, virtual address is in virt_addr */
    return 0;
}
 
static int free_mem(void *virt_addr)
{
    int r;
    L4::Cap<L4Re::Dataspace> ds;
    
    /* Detach memory from our address space */
    if ((r = L4Re::Env::env()->rm()->detach(virt_addr, &ds)))
    {
        printf("Failed to detach dataspace: %d\n", r);
        return r;
    }
    else
        printf("Dataspace detached ok!\n");
 
  /* Release and return capability slot to allocator */
  L4Re::Util::cap_alloc.free(ds, L4Re::Env::env()->task().cap());
 
  /* All went ok */
  return 0;
}

void test_memory_allocation_free()
{
    void *virt_addr;
    unsigned long size_in_bytes = 4096; // 4KB
    unsigned long flags = L4Re::Mem_alloc::Super_pages | L4Re::Mem_alloc::Continuous;

    // Allocate memory
    if (allocate_mem(size_in_bytes, flags, &virt_addr) != 0)
    {
        printf("Memory allocation failed\n");
        return;
    }
    else
        printf("Memory allocated at %p\n", virt_addr);


    // Use the allocated memory (for demonstration purposes)
    char *data = static_cast<char *>(virt_addr);
    const char *message = "data1: Hello from L4Re!";
    strncpy(data, message, strlen(message) + 1);
    printf("Memory content: %s\n", data);
    

    // Write data to the dataspace
    const char *msg = "data2: Hello from Memory L4Re!";
    void *msg_addr = static_cast<char *>(virt_addr) + strlen(data)+1;
    std::strcpy(static_cast<char *>(msg_addr), msg);
    
    // Read it back
    printf("Memory content: %s\n", static_cast<char *>(msg_addr));

    // Free memory
    if (free_mem(virt_addr) != 0)
    {
        printf("Memory free failed\n");
        return;
    }
    else
        printf("Memory freed successfully\n");
}