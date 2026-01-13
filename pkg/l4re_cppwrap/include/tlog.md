
# tlog: Tagged Logging System

`tlog` is a lightweight, compile-time logging system for C and C++ that enables or disables log messages based on user-defined tags. It is designed for embedded and performance-critical environments, providing zero-overhead for disabled logs.

## Features
- **Tag-based filtering:** Only logs with enabled tags (LOG_*) are compiled and executed.
- **Master switch:** `LOG_ENABLE_ALL` enables or disables all logs at once.
- **Compile-time control:** Tags can be enabled/disabled via header or build system (Makefile, compiler flags).
- **Works in C and C++:** Fully compatible with both languages.
- **Function context:** Each log prints the function name where it is called.
- **Immediate output:** All logs are flushed to stdout immediately.

## Usage

### 1. Define Tags
Tags are simple macros with the LOG_ prefix. You can define them in `tlog.h` or via your build system:

```c
#define LOG_ENABLE_ALL      // Enable all logs
#define LOG                 1
#define LOG_Xemacpsif       1
#define LOG_Xemacpsif_Error 1
// etc.
```
Or in your Makefile:
```makefile
CFLAGS += -DLOG_ENABLE_ALL -DLOG=1 -DLOG_Xemacpsif=1
```

### 2. Add tlog to Your Code
Include the header:
```c
#include "tlog.h"
```
Use the macro:
```c
tlog(LOG_Xemacpsif, "Init done, value=%d", value);
tlog(LOG_Xemacpsif_Error, "Error: code=%d", err);
```

### 3. Master Switch: LOG_ENABLE_ALL
- If `LOG_ENABLE_ALL` is defined, all logs are enabled (if their tag is set to 1).
- If `LOG_ENABLE_ALL` is not defined, all tlog calls are disabled at compile time (no code, no cost).

### 4. Output Format
Each log prints:
```
[LOG_TAG][function_name] message
```
Example:
```
[LOG_Xemacpsif][my_init_func] Init done, value=42
[LOG_Xemacpsif_Error][handle_error] Error: code=3
```

### 5. How It Works
- If a tag (e.g., LOG_Xemacpsif) is defined as 1 and LOG_ENABLE_ALL is defined, the log is compiled and executed.
- If a tag is not defined or set to 0, or LOG_ENABLE_ALL is not defined, the log is removed at compile time.
- The macro automatically adds the function name using `__func__`.

### 6. Adding New Tags
Just add a new macro with the LOG_ prefix in `tlog.h` or via your build system, and use it in your code.

## Example
```c
#define LOG_ENABLE_ALL
#define LOG_DEBUG 1
#include "tlog.h"

void foo() {
    tlog(LOG_DEBUG, "Debug info: x=%d", 123);
    tlog(LOG_Xemacpsif_Error, "Should not print unless LOG_Xemacpsif_Error is defined");
}
```

## Best Practices
- Use descriptive tag names (e.g., `LOG_NET`, `LOG_ERROR`, `LOG_INIT`).
- Enable/disable tags as needed for debugging or production.
- Use `LOG_ENABLE_ALL` to globally enable/disable all logs.
- Avoid runtime string concatenation in log messages for performance.

## License
This system is provided as-is, suitable for open-source and proprietary projects.
