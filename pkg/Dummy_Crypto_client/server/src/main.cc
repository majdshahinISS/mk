#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>

// #include <pthread-l4.h>

#include "DataspaceEndpoint.hpp"

static L4Re::Util::Registry_server<> server;
const char *CTS_ipc_name = "CTS_ipc"; // name must be 11 characters long maximum
const char *STC_ipc_name = "STC_ipc"; // name must be 11 characters long maximum
 

class Client_ : DataspaceEndpoint
{

};

int
main()
{
  std::printf("client\n");
  DataspaceEndpoint obj = DataspaceEndpoint(
    &server,
    CTS_ipc_name,
    STC_ipc_name,
    2*1024,
    1000 
  );
  
  server.loop();
  //pthread_join(&thread, nullptr);
  l4_sleep_forever();
  return 0;
}