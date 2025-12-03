#include"dumputility.h"

void 
Dumputility::dump_hex(std::ostream &str,const octet_t *start,size_t anz,off_t startoff)
{
    const octet_t *p=start;
    if((startoff & 0xf)!=0)
    {
        str.width(8);str.fill('0');
    	str << std::hex << (startoff & ~(off_t)0xf) << ':';
        str.width(0);
    	for(off_t i=0;i<(startoff & 0xf);i++)
       	{
	        str << "   ";
	    }
    }
    while(p<start+anz)
    {
        off_t off=startoff+(p-start);
        if((off & 0xf)==0)
	    {
	        str << std::endl;
	        str.width(8);str.fill('0');
	        str << std::hex << off << ':';
	        str.width(0);
	    }
	    str << ' ';
	    str.width(2);
     	str <<  (int)*p;
	    str.width(0);
	    p++;
    }
    str << std::dec << std::endl;
    str.width(0);
}

void 
Dumputility::dump_ascii(std::ostream &str,const octet_t *start,size_t anz,off_t startoff)
{
   const octet_t *p=start;
    if((startoff & 0xf)!=0)
    {
	str.width(8);str.fill('0');
	str << std::hex << (startoff & ~(off_t)0xf) << ": ";
	str.width(0);
	for(off_t i=0;i<(startoff & 0xf);i++)
	{
	    str << "   ";
	}
    }
    while(p<start+anz)
    {
	off_t off=startoff+(p-start);
	if((off & 0xf)==0)
	{
	    str << std::endl;
	    str.width(8);str.fill('0');
	    str << std::hex << off << ": ";
	    str.width(0);
	}
	if(::isalnum(*p)||*p==' ')
	{
	    str << *p;
	}
	else switch(*p)
	{
	    case '\t': str << "\\t"; break;
	    case '\r': str << "\\r"; break;
	    case '\n': str << "\\n"; break;
	    default:
		if(isgraph(*p))
		{
		    str << *p; 
		}
		else
		{
		    str << '\\' << std::oct << (int)*p;
		}
	}
	p++;
    }
    str << std::dec << std::endl;
    str.width(0); 
}
