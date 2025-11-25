// registry_server_wrap.h
#pragma once
#include <l4/re/util/object_registry>
#ifdef __cplusplus
extern "C" {
#endif

 //struct RegistryServerOpaque;   // forward declare if needed

void *Registry_Server_get();

void Registry_Server_loop();


#ifdef __cplusplus
}
#endif
