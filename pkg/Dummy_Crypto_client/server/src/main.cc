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
 


int new_data_callback_handler(DataspaceEndpoint * ds,u_int64_t id, u_int8_t type, u_int8_t * read_addr , l4_size_t size)
{
  // run in a thread ! 
  std::printf("@MS Client , read from address: %p:%s\n",read_addr, read_addr);
  sleep(1);
  std::printf("request peer to free his data\n");
  int res = ds->free_peer_req(id,type, read_addr, size);
  return res;
}

int
main()
{
  std::printf("client\n");
  DataspaceEndpoint obj = DataspaceEndpoint(
    &server,
    CTS_ipc_name,
    STC_ipc_name,
    2*1024,
    1000 ,
    new_data_callback_handler
  );
  
  server.loop();
  //pthread_join(&thread, nullptr);
  l4_sleep_forever();
  return 0;
}