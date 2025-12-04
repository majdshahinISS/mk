#include <stdint.h>


#ifndef L4RE_CPPWRAP_NET_DEV_SRC_ASSERT_H
#define L4RE_CPPWRAP_NET_DEV_SRC_ASSERT_H
#define XIL_ASSERT_NONE     0U
#define XIL_ASSERT_OCCURRED 1U
#define XNULL NULL


extern uint32_t Xil_AssertStatus;
extern int32_t Xil_AssertWait;

void Xil_Assert(const char *File, int32_t Line);


#ifndef Xil_AssertVoid
#define Xil_AssertVoid(Expression)                \
{                                                  \
    if (Expression) {                              \
        Xil_AssertStatus = XIL_ASSERT_NONE;       \
    } else {                                       \
        Xil_Assert(__FILE__, __LINE__);            \
        Xil_AssertStatus = XIL_ASSERT_OCCURRED;   \
        return;                                    \
    }                                              \
}
#endif
#ifndef Xil_AssertNonvoid
#define Xil_AssertNonvoid(Expression)             \
{                                                  \
    if (Expression) {                              \
        Xil_AssertStatus = XIL_ASSERT_NONE;       \
    } else {                                       \
        Xil_Assert(__FILE__, __LINE__);            \
        Xil_AssertStatus = XIL_ASSERT_OCCURRED;   \
        return 0;                                  \
    }                                              \
}
#endif
#endif // L4RE_CPPWRAP_NET_DEV_SRC_ASSERT_H