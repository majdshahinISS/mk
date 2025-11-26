#ifndef __INTERRUPTPL__
#define __INTERRUPTPL__
#ifdef __cplusplus
extern "C" {
#endif
#include <l4/re/util/object_registry>

// the interrupt is already disabled before calling interrupt_handler
// then it will be cleared and enabled after calling interrupt_handler
bool Interrupt_add(void *server, void * obj, int irqnum, void(*interrupt_handler)(void*));
bool Interrupt_remove(int irqnum);

bool Interrupt_check(int irqnum);
#ifdef __cplusplus
}
#endif
#endif // __INTERRUPTPL__