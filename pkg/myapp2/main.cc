#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
// #include <l4/sys/capability>  // for capability management
// #include <l4/re/console>
// #include <l4/re/env>
// #include <l4/re/log>
// #include <l4/re/dataspace>
//#include <l4/re/vcon>
// #include <l4/re/util/cap_alloc>

#include "Test.h"
#include "Tests.h"


Test mytests[] = {
    // Test("log_test", test_log),
    // Test("thread_test", test_pthread),
    // Test("l4_thread_test", test_l4_thread_ipc_1),
    // Test("dummy_test", test_dummy),
    Test("memory_allocation_free_test", test_memory_allocation_free),
};


int main()
{
    std::cout << "myapp2: Hello World! , using main function" << std::endl;
    
    // Run the tests
    for (const auto& test : mytests) {
        test.run();
        std::cout << "______________________________"<<std::endl;
    }
    // Print the results
    std::cout << "All tests completed." << std::endl;
    std::cout << "______________________________"<<std::endl;
    return 0;
}
