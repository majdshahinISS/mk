#ifndef XEMACPSIF_WRAPPER_H
#define XEMACPSIF_WRAPPER_H
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


bool xemacpsif_new(void **obj);
bool xemacpsif_delete(void **obj);

// Basic configuration / control
bool xemacpsif_init(void *obj, unsigned char *macaddr);
bool xemacpsif_set_phy_loopback(void *obj);
bool xemacpsif_init_normal(void *obj);
bool xemacpsif_start_device(void *obj);
bool xemacpsif_stop_device(void *obj);

// RX path
// Returns true if a frame was available; false if none.
// On success, *frame_out is a non-null pointer that must later be passed
// to xemacpsif_release_frame() after smoltcp has consumed the data.
bool xemacpsif_receive(void *obj, void **frame_out);

// Must be called after smoltcp / Rust finishes processing a received frame.
bool xemacpsif_release_frame(void *obj, void *frame);

// TX path
// Returns true on success, false on error.
bool xemacpsif_send(void *obj, const uint8_t *data, uint32_t len);

// Observer callbacks (usually NOT called from Rust; exposed for completeness)
void xemacpsif_notify_send(void *obj);// TODO remove : it must be called from interrupt handler DevEmacPs::handle_irq() automaticaly
void xemacpsif_notify_receive(void *obj);// TODO remove : it must be called from interrupt handler DevEmacPs::handle_irq() automaticaly
// TODO remove : it must be called from interrupt handler DevEmacPs::handle_irq() automaticaly
void xemacpsif_notify_error(void *obj, uint8_t direction, uint32_t errorword);

#ifdef __cplusplus
}
#endif
#endif // XEMACPSIF_WRAPPER_H