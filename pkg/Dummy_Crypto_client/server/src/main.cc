#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <l4/sys/ipc.h>
#include <l4/sys/utcb.h>


#include <l4/sys/err.h>
#include <l4/sys/types.h>
#include <l4/re/env>
#include <l4/re/util/cap_alloc>
 
#include <stdio.h>

#include <l4/util/util.h>
#include "/home/iss/L4Re_d/l4/pkg/user_shared/crypto_shared.h"

static L4Re::Util::Registry_server<> server;
const char *CTS_ipc_name = "CTS_ipc"; // name must be 11 characters long maximum
const char *STC_ipc_name = "STC_ipc"; // name must be 11 characters long maximum
 
//#include <l4/irq/irq.h>
#include <l4/util/util.h>
#include <stdio.h>
#include <pthread-l4.h>



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