#ifndef BSPCONFIG_H
#define BSPCONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Minimal BSP configuration for L4Re ARM64 Xilinx environment */

/* Define basic platform parameters if not already defined */
#ifndef XPAR_MICROBLAZE_ADDR_SIZE
#define XPAR_MICROBLAZE_ADDR_SIZE 0
#endif

#ifndef XPAR_CPU_ID
#define XPAR_CPU_ID 0
#endif

/* ARM64 architecture definitions */
#define __aarch64__ 1
#define __arch64__ 1

#ifdef __cplusplus
}
#endif

#endif /* BSPCONFIG_H */
