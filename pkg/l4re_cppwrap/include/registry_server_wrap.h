// registry_server_wrap.h
#pragma once
#include <l4/re/util/object_registry>
#ifdef __cplusplus
extern "C" {
#endif

 //struct RegistryServerOpaque;   // forward declare if needed

void *l4re_get_registry_server();

void registry_server_loop();


#ifdef __cplusplus
}
#endif
