/*
 * CRLARB.cpp
 *
 *  Created on: Apr 14, 2022
 *      Author: weber
 */

#include "CRLARB.h"

CRL_ARB::CRL_ARB():IORegion("CRLARB")
{

}

CRL_ARB::~CRL_ARB()
{

}

const uint32_t CR_GEM3_REF_CRNTL = 0x5c;
/* high level operations */
// vgl xemacps_example_intr_dema.c:1543ff ANDERS
void CRL_ARB::configure_GEM3_1G_clock()
{
    printf("configure_GEM3_1G_clock\r\n");
    const uint32_t CRL_GEM_DIV_MASK = 0x003F3F00;
    const uint32_t CRL_GEM_1G_DIV0 = 0x00000C00;
    const uint32_t CRL_GEM_1G_DIV1 = 0x00010000;

    uint32_t ClkCntrl = In32(CR_GEM3_REF_CRNTL);
    ClkCntrl &= ~CRL_GEM_DIV_MASK;
    ClkCntrl |= CRL_GEM_1G_DIV1;
    ClkCntrl |= CRL_GEM_1G_DIV0;
    Out32(CR_GEM3_REF_CRNTL, ClkCntrl);
}

// aus xparameters
/* Definitions for peripheral PSU_ETHERNET_3 */
#define XPAR_PSU_ETHERNET_3_DEVICE_ID 0
#define XPAR_PSU_ETHERNET_3_BASEADDR 0xFF0E0000
#define XPAR_PSU_ETHERNET_3_HIGHADDR 0xFF0EFFFF
#define XPAR_PSU_ETHERNET_3_ENET_CLK_FREQ_HZ 125000000
#define XPAR_PSU_ETHERNET_3_ENET_SLCR_1000MBPS_DIV0 12
#define XPAR_PSU_ETHERNET_3_ENET_SLCR_1000MBPS_DIV1 1
#define XPAR_PSU_ETHERNET_3_ENET_SLCR_100MBPS_DIV0 60
#define XPAR_PSU_ETHERNET_3_ENET_SLCR_100MBPS_DIV1 1
#define XPAR_PSU_ETHERNET_3_ENET_SLCR_10MBPS_DIV0 60
#define XPAR_PSU_ETHERNET_3_ENET_SLCR_10MBPS_DIV1 10
#define XPAR_PSU_ETHERNET_3_ENET_TSU_CLK_FREQ_HZ 250000000

//aus xemacpsif.h
#define CRL_APB_GEM_DIV0_MASK	0x00003F00
#define CRL_APB_GEM_DIV0_SHIFT	8
#define CRL_APB_GEM_DIV1_MASK	0x003F0000
#define CRL_APB_GEM_DIV1_SHIFT	16

// vgl xemacps_pyspeed.c
// für gigeversion == GEM_VERSION_ZYNQMP == 7 und ZYNQMP_EMACPS_3
void CRL_ARB::SetUpSLCRDivisors(uint32_t speed)
{
	uint32_t CrlApbDiv0 = 0;
	uint32_t CrlApbDiv1 = 0;
	uint32_t CrlApbGemCtrl;

    /* Setup divisors in CRL_APB for Zynq Ultrascale+ MPSoC */
	if (speed == 1000)
    {
		CrlApbDiv0 = XPAR_PSU_ETHERNET_3_ENET_SLCR_1000MBPS_DIV0;
		CrlApbDiv1 = XPAR_PSU_ETHERNET_3_ENET_SLCR_1000MBPS_DIV1;
	}
	else if (speed == 100)
    {
		CrlApbDiv0 = XPAR_PSU_ETHERNET_3_ENET_SLCR_100MBPS_DIV0;
		CrlApbDiv1 = XPAR_PSU_ETHERNET_3_ENET_SLCR_100MBPS_DIV1;
	}
	else
    {
		CrlApbDiv0 = XPAR_PSU_ETHERNET_3_ENET_SLCR_10MBPS_DIV0;
		CrlApbDiv1 = XPAR_PSU_ETHERNET_3_ENET_SLCR_10MBPS_DIV1;
	}

	if (CrlApbDiv0 != 0 && CrlApbDiv1 != 0)
    {
        CrlApbGemCtrl = In32(CR_GEM3_REF_CRNTL);
        printf("CRLARB: IN32(%p)=%x\r\n",(void *)(physaddr_+CR_GEM3_REF_CRNTL),CrlApbGemCtrl);
		CrlApbGemCtrl &= ~CRL_APB_GEM_DIV0_MASK;
		CrlApbGemCtrl |= CrlApbDiv0 << CRL_APB_GEM_DIV0_SHIFT;
		CrlApbGemCtrl &= ~CRL_APB_GEM_DIV1_MASK;
        CrlApbGemCtrl |= CrlApbDiv1 << CRL_APB_GEM_DIV1_SHIFT;

        Out32(CR_GEM3_REF_CRNTL, CrlApbGemCtrl);  
        printf("CRLARB: OUT32(%p)=%x\r\n",(void *)(physaddr_+CR_GEM3_REF_CRNTL),CrlApbGemCtrl);

    }
    else
    {
        printf("Clock Divisors incorrect - Please check\r\n");
    }
	return;
}

