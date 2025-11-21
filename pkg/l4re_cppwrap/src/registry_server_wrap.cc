#include "../include/registry_server_wrap.h"


static L4Re::Util::Registry_server<> server;

// C-compatible accessor
extern "C" void *l4re_get_registry_server()
{
    return static_cast<void*>(&server);
}

extern "C" void registry_server_loop()
{
    server.loop();   // or whatever method you need
}

