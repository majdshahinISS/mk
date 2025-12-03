#include "../include/xemacpsif_wrapper.h"
#include "../net_dev_src/xemacpsif.h"


#include <iostream>

static inline bool check_xemacpsif_object(void *obj, char const* func_name, bool additional_fail_condition = false)
{
    if (additional_fail_condition) {
        return false;
    }
    if (!obj) {
        std::cerr << func_name << ": Xemacpsif object pointer is null" << std::endl;
        return false;
    }

    Xemacpsif *instance = static_cast<Xemacpsif *>(obj);
    if (instance->magic_ != 0x5137434D) {
        std::cerr << "Invalid Xemacpsif object" << std::endl;
        return false;
    }

    return true;
}

static bool xemacpsif_new_(void **obj)
{
    if (obj == nullptr) {
        return false;
    }

    Xemacpsif *instance = nullptr;
    try {
        instance = new Xemacpsif();
    } catch (...) {
        return false;
    }

    if (!instance) {
        return false;
    }

    *obj = static_cast<void *>(instance);
    std::cout << "Xemacpsif instance created " << instance << std::endl;
    return true;
}

static bool xemacpsif_delete_(void **obj)
{
    if (!check_xemacpsif_object(*obj, __FUNCTION__, obj == nullptr)) {
        return false;
    }
    Xemacpsif *instance = static_cast<Xemacpsif *>(*obj);

    std::cout << "Deleting Xemacpsif instance " << instance << std::endl;

    delete instance;
    *obj = nullptr;
    return true;
}

static bool xemacpsif_init_(void *obj, unsigned char *macaddr)
{
    if (!check_xemacpsif_object(obj, __FUNCTION__, macaddr == nullptr)) {
        return false;
    }

    Xemacpsif *instance = static_cast<Xemacpsif *>(obj);

    try {
        instance->init(macaddr);
    } catch (...) {
        return false;
    }

    return true;
}

static bool xemacpsif_set_phy_loopback_(void *obj)
{
    if (!check_xemacpsif_object(obj, __FUNCTION__)) {
        return false;
    }

    Xemacpsif *instance = static_cast<Xemacpsif *>(obj);
    try {
        instance->setPhyLoopback();
    } catch (...) {
        return false;
    }
    return true;
}

static bool xemacpsif_init_normal_(void *obj)
{
    if (!check_xemacpsif_object(obj, __FUNCTION__)) {
        return false;
    }

    Xemacpsif *instance = static_cast<Xemacpsif *>(obj);

    try {
        instance->initNormal();
    } catch (...) {
        return false;
    }
    return true;
}

static bool xemacpsif_start_device_(void *obj)
{
    if (!check_xemacpsif_object(obj, __FUNCTION__)) {
        return false;
    }

    Xemacpsif *instance = static_cast<Xemacpsif *>(obj);

    try {
        instance->startDevice();
    } catch (...) {
        return false;
    }
    return true;
}

static bool xemacpsif_stop_device_(void *obj)
{
    if (!check_xemacpsif_object(obj, __FUNCTION__)) {
        return false;
    }

    Xemacpsif *instance = static_cast<Xemacpsif *>(obj);

    try {
        instance->stopDevice();
    } catch (...) {
        return false;
    }
    return true;
}

static bool xemacpsif_receive_(void *obj, void **frame_out)
{
    if (!check_xemacpsif_object(obj, __FUNCTION__, frame_out == nullptr)) {
        return false;
    }

    Xemacpsif *instance = static_cast<Xemacpsif *>(obj);

    EmacRawFrame *frame = nullptr;
    try {
        frame = instance->receive();
    } catch (...) {
        return false;
    }

    if (!frame) {
        *frame_out = nullptr;
        return false; // no frame available
    }

    *frame_out = static_cast<void *>(frame);
    return true;
}

static bool xemacpsif_release_frame_(void *obj, void *frame)
{
    if (!check_xemacpsif_object(obj, __FUNCTION__, frame == nullptr)) {
        return false;
    }

    Xemacpsif *instance = static_cast<Xemacpsif *>(obj);

    EmacRawFrame *f = static_cast<EmacRawFrame *>(frame);

    try {
        instance->releaseReceivedFrame(f);
    } catch (...) {
        return false;
    }

    return true;
}

static bool xemacpsif_send_(void *obj, const uint8_t *data, uint32_t len)
{
    if (!check_xemacpsif_object(obj, __FUNCTION__, data == nullptr || len == 0)) {
        return false;
    }

    Xemacpsif *instance = static_cast<Xemacpsif *>(obj);

    int ret = 0;
    try {
        ret = instance->send(data, len);
    } catch (...) {
        return false;
    }

    return (ret == XST_SUCCESS);
}

static void xemacpsif_notify_send_(void *obj)
{
    if (!check_xemacpsif_object(obj, __FUNCTION__)) {
        return;
    }
    Xemacpsif *instance = static_cast<Xemacpsif *>(obj);
    instance->notifySend();
}

static void xemacpsif_notify_receive_(void *obj)
{
    if (!check_xemacpsif_object(obj, __FUNCTION__)) {
        return;
    }
    Xemacpsif *instance = static_cast<Xemacpsif *>(obj);
    instance->notifyReceive();
}

static void xemacpsif_notify_error_(void *obj, uint8_t direction, uint32_t errorword)
{
    if (!check_xemacpsif_object(obj, __FUNCTION__)) {
        return;
    }

    Xemacpsif *instance = static_cast<Xemacpsif *>(obj);
    instance->notifyError(direction, errorword);
}


// =======================
//  C ABI exports
// =======================

#ifdef __cplusplus
extern "C" {
#endif

bool xemacpsif_new(void **obj)
{
    return xemacpsif_new_(obj);
}

bool xemacpsif_delete(void **obj)
{
    return xemacpsif_delete_(obj);
}

bool xemacpsif_init(void *obj, unsigned char *macaddr)
{
    return xemacpsif_init_(obj, macaddr);
}

bool xemacpsif_set_phy_loopback(void *obj)
{
    return xemacpsif_set_phy_loopback_(obj);
}

bool xemacpsif_init_normal(void *obj)
{
    return xemacpsif_init_normal_(obj);
}

bool xemacpsif_start_device(void *obj)
{
    return xemacpsif_start_device_(obj);
}

bool xemacpsif_stop_device(void *obj)
{
    return xemacpsif_stop_device_(obj);
}

bool xemacpsif_receive(void *obj, void **frame_out)
{
    return xemacpsif_receive_(obj, frame_out);
}

bool xemacpsif_release_frame(void *obj, void *frame)
{
    return xemacpsif_release_frame_(obj, frame);
}

bool xemacpsif_send(void *obj, const uint8_t *data, uint32_t len)
{
    return xemacpsif_send_(obj, data, len);
}

void xemacpsif_notify_send(void *obj)
{
    xemacpsif_notify_send_(obj);
}

void xemacpsif_notify_receive(void *obj)
{
    xemacpsif_notify_receive_(obj);
}

void xemacpsif_notify_error(void *obj, uint8_t direction, uint32_t errorword)
{
    xemacpsif_notify_error_(obj, direction, errorword);
}

#ifdef __cplusplus
}
#endif