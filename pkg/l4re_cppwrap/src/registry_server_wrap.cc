#include "../include/registry_server_wrap.h"


static L4Re::Util::Registry_server<> server;

// C-compatible accessor
extern "C" void *Registry_Server_get()
{
    return static_cast<void*>(&server);
}

extern "C" void Registry_Server_loop()
{
    // TODO , check if in main thread
    server.loop();   // or whatever method you need
}

