/******************************************************************************
*
* Copyright (C) 2010 - 2019 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*
*
*
******************************************************************************/
/****************************************************************************/
/**
*
* @file xemacps_example.h
*
* Defines common data types, prototypes, and includes the proper headers
* for use with the EMACPS example code residing in this directory.
*
* This file along with xemacps_example_util.c are utilized with the specific
* example code in the other source code files provided.
* These examples are designed to be compiled and utilized within the SDK
* standalone BSP development environment.
*
*****************************************************************************/
#ifndef XEMACPS_EXAMPLE_H
#define XEMACPS_EXAMPLE_H


/***************************** Include Files ********************************/

#include <stdio.h>
#include <stdlib.h>
#include "xil_types.h"
#include "xil_assert.h"
// #include "nx_packet.h"
#include "ethernet.h"

/************************** Constant Definitions ****************************/

#define EMACPS_LOOPBACK_SPEED    100	/* 100Mbps */
#define EMACPS_LOOPBACK_SPEED_1G 1000	/* 1000Mbps */
#define EMACPS_PHY_DELAY_SEC     4	/* Amount of time to delay waiting on
					   PHY to reset */
#define EMACPS_SLCR_DIV_MASK	0xFC0FC0FF

#define CSU_VERSION		0xFFCA0044
#define PLATFORM_MASK		0xF000
#define PLATFORM_SILICON	0x0000
#define VERSAL_VERSION		0xF11A0004
#define PLATFORM_MASK_VERSAL	0xF000000
#define PLATFORM_VERSALEMU	0x1000000
#define PLATFORM_VERSALSIL	0x0000000

/* aus xilinx emacps.h */
/* The next few constants help upper layers determine the size of memory
 * pools used for Ethernet buffers and descriptor lists.
 */
#ifndef XEMACPS_MAC_ADDR_SIZE
#define XEMACPS_MAC_ADDR_SIZE   6U      /* size of Ethernet header */
#endif
#define XEMACPS_MTU             1500U   /* max MTU size of Ethernet frame */
#define XEMACPS_MTU_JUMBO       10240U  /* max MTU size of jumbo frame */
#ifndef XEMACPS_HDR_SIZE
#define XEMACPS_HDR_SIZE        14U     /* size of Ethernet header */
#endif
#define XEMACPS_HDR_VLAN_SIZE   18U     /* size of Ethernet header with VLAN */
#define XEMACPS_TRL_SIZE        4U      /* size of Ethernet trailer (FCS) */
#define XEMACPS_MAX_FRAME_SIZE       (XEMACPS_MTU + XEMACPS_HDR_SIZE + \
        XEMACPS_TRL_SIZE)
#define XEMACPS_MAX_VLAN_FRAME_SIZE  (XEMACPS_MTU + XEMACPS_HDR_SIZE + \
        XEMACPS_HDR_VLAN_SIZE + XEMACPS_TRL_SIZE)
#define XEMACPS_MAX_VLAN_FRAME_SIZE_JUMBO  (XEMACPS_MTU_JUMBO + XEMACPS_HDR_SIZE + \
        XEMACPS_HDR_VLAN_SIZE + XEMACPS_TRL_SIZE)

/***************** Macros (Inline Functions) Definitions ********************/


/**************************** Type Definitions ******************************/


/************************** Function Prototypes *****************************/

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Utility functions implemented in xemacps_example_util.c
 */
/* TODO
void EmacPsUtilFrameHdrFormatMACandType(NX_PACKET *packet, const char *DestAddr,uint16_t FrameType);
void EmacPsUtilFrameSetPayloadData(NX_PACKET *packet, uint32_t PayloadSize);
LONG EmacPsUtilFrameVerify(NX_PACKET * Check, NX_PACKET * Actual);
void EmacPsUtilMemClear(NX_PACKET *packet);
void EmacPsUtilstrncpy(char *Destination, const char *Source, uint32_t n);
void EmacPsUtilErrorTrap(const char *Message);
void EmacpsDelay(u32 delay);
*/

/************************** Variable Definitions ****************************/
/* defined in xemacsps_example_util.c */
extern char EmacPsMAC[];	/* Local MAC address */

#ifdef __cplusplus
} 
#endif
#endif /* XEMACPS_EXAMPLE_H */
