/**
 * @file tlog.h 
 * @author majd shahin (majd.shahin@iss-ag.com)
 * @brief 
 * @version 0.1
 * @date 2026-01-13
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#ifndef TLOG_H
#define TLOG_H

#ifdef __cplusplus
#include <cstdio>
#include <cstdarg>
extern "C" {
#else
#include <stdio.h>
#include <stdarg.h>
#endif

// ============================================================================
// TAG DEFINITIONS - Enable/disable tags here or via Makefile
// ============================================================================

// Define tags you want to enable:
//#define LOG_ENABLE_ALL


#define LOG                     1
#define LOG_EVBuffer_insert     1
#define LOG_EVBuffer_remove     1

#define LOG_Xemacpsif           1
#define LOG_Xemacpsif_Error     1
#define LOG_Xemacpsif_Warning   1
#define LOG_Xemacpsif_Receive   1
#define LOG_Xemacpsif_Send      1
#define LOG_Xemacpsif_Wrapper   1
#define LOG_Xemacpsif_INTER     1
// ============================================================================
// LOGGING IMPLEMENTATION
// ============================================================================


#ifdef LOG_ENABLE_ALL
static inline void tlog_print(const char* tag_name, const char* func_name, const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("[%s][%s] ", tag_name, func_name);
    vprintf(format, args);
    va_end(args);
    printf("\n");
    fflush(stdout);
}


#define tlog(tag, fmt, ...) \
    do { \
        /* Compile-time check - array size trick */ \
        char _compile_check[tag == 1 ? 1 : 0]; \
        (void)_compile_check; \
        if (tag == 1) { \
            tlog_print(#tag, __func__, fmt, ##__VA_ARGS__); \
        } \
        \
    } while(0)


#else
#define tlog(tag, tmt, ...) (void)0
#endif

#ifdef __cplusplus
}
#endif

#endif // TLOG_H

