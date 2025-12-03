#ifndef DUMPUTILITY
#define DUMPUTILITY
#include <sys/types.h>
#include<ostream>

typedef unsigned char octet_t;

namespace Dumputility
{
    void dump_hex(std::ostream &str,const octet_t *start,size_t anz,off_t startoff=0);
    void dump_ascii(std::ostream &str,const octet_t *start,size_t anz,off_t startoff=0);
};
#endif
