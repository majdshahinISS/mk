#ifndef XPARAMETERS_H
#define XPARAMETERS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Minimal Xilinx parameters for L4Re ARM64 environment */

/* Processor definitions */
#ifndef XPAR_CPU_ID
#define XPAR_CPU_ID 0
#endif

#ifndef XPAR_MICROBLAZE_ADDR_SIZE
#define XPAR_MICROBLAZE_ADDR_SIZE 0
#endif

/* Architecture definitions */
#define __aarch64__ 1
#define __arch64__ 1

#ifdef __cplusplus
}
#endif

#endif /* XPARAMETERS_H */
