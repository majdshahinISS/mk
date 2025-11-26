#include <l4/sys/types.h>    // For time_t, suseconds_t
#include <sys/stat.h>        // For stat64
#include <sys/time.h>        // For suseconds_t
#include <sys/statvfs.h>     // For fsblkcnt64_t, fsfilcnt64_t
#include <cwchar>            // For wchar_t
#include <iostream>

int main()
{
    std::cout << "Size of types on this system:\n";
    
    // Previous types
    std::cout << "blkcnt_t: " << sizeof(blkcnt_t) << " bytes\n";
    std::cout << "blksize_t: " << sizeof(blksize_t) << " bytes\n";
    std::cout << "clock_t: " << sizeof(clock_t) << " bytes\n";
    std::cout << "fsblkcnt_t: " << sizeof(fsblkcnt_t) << " bytes\n";
    std::cout << "fsfilcnt_t: " << sizeof(fsfilcnt_t) << " bytes\n";
    // std::cout << "fsword_t: " << sizeof(__fsword_t) << " bytes\n";
    std::cout << "ino_t: " << sizeof(ino_t) << " bytes\n";
    std::cout << "nlink_t: " << sizeof(nlink_t) << " bytes\n";
    std::cout << "off_t: " << sizeof(off_t) << " bytes\n";
    
    // New types
    std::cout << "stat64: " << sizeof(struct stat64) << " bytes\n";
    std::cout << "suseconds_t: " << sizeof(suseconds_t) << " bytes\n";
    std::cout << "time_t: " << sizeof(time_t) << " bytes\n";
    std::cout << "wchar_t: " << sizeof(wchar_t) << " bytes\n";
    std::cout << "fsblkcnt64_t: " << sizeof(fsblkcnt64_t) << " bytes\n";
    std::cout << "fsfilcnt64_t: " << sizeof(fsfilcnt64_t) << " bytes\n";
    std::cout << "__WORDSIZE: " << __WORDSIZE<< "\n";
    return 0;
}