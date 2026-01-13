#include <iostream>
#include <stdio.h>
#include <string.h>
#include <l4/re/error_helper>
#include <l4/re/util/object_registry>
#include <l4/re/util/br_manager>
#include <l4/re/env>
#include <l4/re/dataspace>
#include <l4/re/dma_space>
#include <l4/re/protocols.h>
#include <l4/re/error_helper>
#include <l4/re/rm>
#include <l4/re/util/unique_cap>
#include <l4/re/util/cap_alloc>
#include <l4/sys/err.h>
#include <l4/sys/factory>
#include <l4/sys/cache.h>
#include <l4/io/io.h>
#include <pthread.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <cassert>
#include "xil_assert.h"
#include "xemacps_example.h"
#include "dumputility.h"
#include "dmamem.h"
#include "xemacpsif.h"
#include "../include/registry_server_wrap.h"
#include "Xil_Assert.h"

void EmacPsUtilErrorTrap(const char *Message);
void EmacPsUtilErrorTrap(const char *Message)
{
	static uint32_t Count = 0;

	Count++;

	printf("%s\r\n", Message);

}

// #include "nx_packet_pool.h"
// NX_PACKET_POOL packetpool; // REVIEW
static constexpr size_t RxBufSize = XEMACPSIF_MAX_FRAME_SIZE * RXBD_CNT;
uint8_t *RxDataBufVirt = nullptr;
uint8_t *RxDataBufPhys = nullptr;

// TX DMA buffers (one per BD)
static constexpr size_t TxBufSize = XEMACPSIF_MAX_FRAME_SIZE;
uint8_t *TxDataBufVirt = nullptr;
uint8_t *TxDataBufPhys = nullptr;

uint32_t GemVersion = 0;
/*
 * Counters to be incremented by callbacks
 */
volatile int32_t DeviceErrors;	/* Number of errors detected in the device */

const uint32_t TXBD_CNT = 32;	/* Number of TxBDs to use */
/*
 * Buffer descriptors are allocated in uncached memory. The memory is made
 * uncached by setting the attributes appropriately in the MMU table.
 */
#define RXBD_SPACE_BYTES XEmacPs_BdRingMemCalc(XEMACPS_BD_ALIGNMENT, RXBD_CNT)
#define TXBD_SPACE_BYTES XEmacPs_BdRingMemCalc(XEMACPS_BD_ALIGNMENT, TXBD_CNT)
#ifndef __aarch64__
#error __aarch64__ is not defined
#endif


Xemacpsif::~Xemacpsif() {
    tlog(LOG_Xemacpsif, " Destructor called");
    magic_ = 0;
    // REVIEW : cleanup if needed 
}

Xemacpsif::Xemacpsif()
 :eth_link_status_(ETH_LINK_DOWN)
{
    tlog(LOG_Xemacpsif, " Constructor called");
    etherdev.registerObserver(this);
    pthread_mutex_init(&mutex_,NULL);
    pthread_cond_init(&signalrecv_,NULL);
    pthread_cond_init(&signalsendfinished_,NULL);

    for(uint32_t i=0; i< RXBD_CNT; i++)
    {
        rx_frames_[i].len = 0; // REVIEW
    }
    tlog(LOG_Xemacpsif, " Constructor finished");
}

void Xemacpsif::init(UCHAR *macaddr)
{
    tlog(LOG_Xemacpsif, " Initialization started");
    allocResourcesFromIo();
    allocDMASpaces();
    initializeDevice();
    setMacAddr(macaddr);
    createRXRing();
    createTXRing();
    setPriorityBuffers();
    prepareReceive();
    tlog(LOG_Xemacpsif, " Initialization finished");
}

int Xemacpsif::allocResourcesFromIo()
{
    tlog(LOG_Xemacpsif, " Allocating resources from L4IO");
    l4io_device_handle_t dh = l4io_get_root_device();
    l4io_device_t dev;
    l4io_resource_handle_t reshandle;
    l4io_resource_t res;

    uint16_t irqnum = 0;
    tlog(LOG_Xemacpsif, "Device scan:");
    long ret = 0;

    while ((ret = l4io_iterate_devices(&dh, &dev, &reshandle)) == 0)
    {
    tlog(LOG_Xemacpsif, "dev.name=%s", dev.name);
      if (strcmp(dev.name, "ethernet3") == 0)
      {
        if (dev.num_resources == 0)
        {
	       std::clog << "No resource entries for ethernet3- terminating "
	                 << std::endl;
	       return -1;
	    }
	    l4io_resource_handle_t reshandle_save = reshandle;
	    ret = l4io_lookup_resource(dh, L4IO_RESOURCE_MEM, &reshandle, &res);
	    if (ret != 0)
	    {
	        std::clog << "l4io_lookup_resource(L4IO_RESOURCE_MEM) returns "
	                  << ret << std::endl;
	       return -1;
	    }
        etherdev.initmem(res.start,res.end);
	
	    // Query Interrupt at io
	    reshandle =  reshandle_save;
	    ret = l4io_lookup_resource(dh, L4IO_RESOURCE_IRQ, &reshandle, &res);
	    if (ret != 0)
	    {
	      std::clog << "l4io_lookup_resource(L4IO_RESOURCE_IRQ) returns "
	                << ret << std::endl;
	      return -1;
	    }
	    irqnum = (uint16_t) (res.start);
        tlog(LOG_Xemacpsif, " IRQ: %u", irqnum);

	    L4::Cap < L4::Irq > l4irq;
        L4Re::Util::Registry_server<> *server = (L4Re::Util::Registry_server<> *) Registry_Server_get();
	    L4Re::chkcap(l4irq =  server->registry()->register_irq_obj(&etherdev));
        L4_irq_mode irq_mode = L4_irq_mode(res.flags);
	    etherdev.set_irq(irqnum, l4irq,irq_mode);
      }
      else if (strcmp(dev.name, "CRL_ARB") == 0)
      {
	    long retr = l4io_lookup_resource(dh, L4IO_RESOURCE_MEM, &reshandle, &res);
       	if (retr != 0)
	    {
	       std::clog << "l4io_lookup_resource(L4IO_RESOURCE_MEM) returns "
	                 << retr << std::endl;
	       return -1;
	    }
	    crl_arb.initmem(res.start, res.end);
      }
    }
    tlog(LOG_Xemacpsif, " Resource allocation from L4IO completed");
    return 0;
}

int Xemacpsif::allocDMASpaces()// REVIEW
{
    tlog(LOG_Xemacpsif, " Allocating DMA spaces");
    void *dmavirt_rxbdspace=0;
    void *dmavirt_txbaspace=0;
    void *dmavirt_BdRxTerminate=0;
    void *dmavirt_BdTxTerminate=0;
    void *dmavirt_rxdata = 0;
    void *dmavirt_txdata = 0;

    L4Re::Dma_space::Dma_addr dmaphys_rxbdspace=0;
    L4Re::Dma_space::Dma_addr dmaphys_txbaspace=0;
    L4Re::Dma_space::Dma_addr dmaphys_BdRxTerminate=0;
    L4Re::Dma_space::Dma_addr dmaphys_BdTxTerminate=0;
    L4Re::Dma_space::Dma_addr dmaphys_rxdata = 0;
    L4Re::Dma_space::Dma_addr dmaphys_txdata = 0;

    int r;
    auto dmaspace =
      L4Re::chkcap(L4Re::Util::make_unique_cap < L4Re::Dma_space > ());
    if ((r = l4_error(L4Re::Env::env()->user_factory()->create(dmaspace.get()))) != 0)
    {
    tlog(LOG_Xemacpsif_Error, " Creation of DMA-Space failed");
      return r;
    }
    /* Allocate memory: 16k Bytes (usually) */
    // war 2 + L4_PAGESHIFT,// 16 KByte aligned
    if (allocate_dmamem(0x100000, 0, 2+L4_PAGESHIFT, 
			&dmavirt_rxbdspace, dmaspace, &dmaphys_rxbdspace))
    {
    tlog(LOG_Xemacpsif_Error, " The memory allocation failed(1)");
      return -1;
    }

    tlog(LOG_Xemacpsif, " Allocated DMA memory RxBdSpace, virtual: %p phys %llx", dmavirt_rxbdspace,dmaphys_rxbdspace);
    RxBdSpacePtrVirt = (uint8_t *) dmavirt_rxbdspace;
    RxBdSpacePtrPhys = (uint8_t *) dmaphys_rxbdspace;

    /* Allocate memory: 16k Bytes (usually) */
    // war 2 + L4_PAGESHIFT,	// 16 KByte aligned
    if (allocate_dmamem(0x100000, 0, 2 + L4_PAGESHIFT,
			&dmavirt_txbaspace, dmaspace, &dmaphys_txbaspace))
    {
    tlog(LOG_Xemacpsif_Error, " The memory allocation failed(2)");
      return -1;
    }

    tlog(LOG_Xemacpsif, " Allocated DMA memory TxBdSpace, virtual: %p phys %llx", dmavirt_txbaspace,dmaphys_txbaspace);
    /* Allocate Tx BD space each */
    TxBdSpacePtrVirt = (uint8_t *) dmavirt_txbaspace;
    TxBdSpacePtrPhys = (uint8_t *) dmaphys_txbaspace;

    /* Allocate memory: 4k Bytes */
    if (allocate_dmamem(0x4000, 0, 2 + L4_PAGESHIFT,	// 16 KByte aligned
			&dmavirt_BdRxTerminate, dmaspace, &dmaphys_BdRxTerminate))
    {
    tlog(LOG_Xemacpsif_Error, " The memory allocation failed(3)");
      return -1;
    }
    tlog(LOG_Xemacpsif, " Allocated DMA memory BdRxTerminate, virtual: %p phys %llx", dmavirt_BdRxTerminate,dmaphys_BdRxTerminate);
    BdRxTerminatePtrVirt = (uint8_t *) dmavirt_BdRxTerminate;
    BdRxTerminatePtrPhys = (uint8_t *) dmaphys_BdRxTerminate;


    /* Allocate memory: 4k Bytes */
    if (allocate_dmamem(0x4000, 0, 2 + L4_PAGESHIFT,	// 16 KByte aligned
			&dmavirt_BdTxTerminate, dmaspace, &dmaphys_BdTxTerminate))
    {
    tlog(LOG_Xemacpsif_Error, " The memory allocation failed(4)");
      return -1;
    }
    tlog(LOG_Xemacpsif, " Allocated DMA memory BdTxTerminate, virtual: %p phys %llx", dmavirt_BdTxTerminate,dmaphys_BdTxTerminate);

    BdTxTerminatePtrVirt = (uint8_t *) dmavirt_BdTxTerminate;
    BdTxTerminatePtrPhys = (uint8_t *) dmaphys_BdTxTerminate;
    
    // REVIEW
    // init_packet_pool(&packetpool,"IPpackets",dmaspace,JUMBO_FRAME_SIZE,64);
    size_t rx_buf_total = RXBD_CNT * XEMACPSIF_MAX_FRAME_SIZE;

    if (allocate_dmamem(rx_buf_total, 0, 2 + L4_PAGESHIFT,
                        &dmavirt_rxdata, dmaspace, &dmaphys_rxdata))
    {
        tlog(LOG_Xemacpsif_Error, " The memory allocation failed(5) — RX data buffers");
        return -1;
    }

    tlog(LOG_Xemacpsif, " Allocated RX data buffer, virt=%p phys=%llx, size=%zu",
        dmavirt_rxdata, dmaphys_rxdata, rx_buf_total);

    RxDataBufVirt = (uint8_t *)dmavirt_rxdata;
    RxDataBufPhys = (uint8_t *)dmaphys_rxdata;


    size_t tx_buf_total = TXBD_CNT * XEMACPSIF_MAX_FRAME_SIZE;

    if (allocate_dmamem(tx_buf_total, 0, 2 + L4_PAGESHIFT,
                        &dmavirt_txdata, dmaspace, &dmaphys_txdata))
    {
        tlog(LOG_Xemacpsif_Error, " The memory allocation failed(6) — TX data buffers");
        return -1;
    }

        tlog(LOG_Xemacpsif, " Allocated TX data buffer, virt=%p phys=%llx, size=%zu",
            dmavirt_txdata, dmaphys_txdata, tx_buf_total);

    TxDataBufVirt = (uint8_t *)dmavirt_txdata;
    TxDataBufPhys = (uint8_t *)dmaphys_txdata;
    
    return 0;
}

void Xemacpsif::initializeDevice()
{
    etherdev.ResetDevice();
    tlog(LOG_Xemacpsif, "Device initialized");
    GemVersion = etherdev.GemVersion();
    tlog(LOG_Xemacpsif, "GemVersion = %x", GemVersion);

    if (GemVersion > 2)
    {
        etherdev.SetOptions(XEMACPS_JUMBO_ENABLE_OPTION);
    }
    crl_arb.configure_GEM3_1G_clock();	//XEmacPsClkSetup
}

int Xemacpsif::setMacAddr(UCHAR *macaddr)
{
    tlog(LOG_Xemacpsif, "Set the MAC address ");
    #ifdef LOG_Xemacpsif
    for(int i=0;i<6;i++)
    {
        printf("%2x",macaddr[i]);
    }
    printf("\r\n");
    #endif
    LONG Status = etherdev.SetMacAddress(macaddr, 1);
    if (Status != XST_SUCCESS)
    {
      tlog(LOG_Xemacpsif_Error, "Error setting MAC address");
      return -1;
    }
    tlog(LOG_Xemacpsif, "OK - MAC address set");
    return 0;
}

LONG Xemacpsif::createRXRing()
{
    tlog(LOG_Xemacpsif, "Creating RX Ring");
    LONG Status=XST_SUCCESS;
    XEmacPs_Bd BdTemplate;
    /*
     * Setup RxBD space.
     *
     * Setup a BD template for the Rx channel. This template will be
     * copied to every RxBD. We will not have to explicitly set these
     * again.
     */
    XEmacPs_BdClear(&BdTemplate);
    /*
     * Create the RxBD ring
     */
    Status = XEmacPs_BdRingCreate(&(etherdev.RxBdRing),
				  (UINTPTR) RxBdSpacePtrPhys,
				  (UINTPTR) RxBdSpacePtrVirt,
				  XEMACPS_BD_ALIGNMENT, RXBD_CNT);
    if (Status != XST_SUCCESS)
    {
      tlog(LOG_Xemacpsif_Error, " Error setting up RxBD space, BdRingCreate");
      EmacPsUtilErrorTrap("Error setting up RxBD space, BdRingCreate");
      return XST_FAILURE;
    }

    Status = XEmacPs_BdRingClone(&(etherdev.RxBdRing), &BdTemplate, XEMACPS_RECV);
    if (Status != XST_SUCCESS)
    {
      tlog(LOG_Xemacpsif_Error, " Error setting up RxBD space, BdRingClone");
      EmacPsUtilErrorTrap("Error setting up RxBD space, BdRingClone");
      return XST_FAILURE;
    }
    tlog(LOG_Xemacpsif, "RX Ring created successfully");
    return XST_SUCCESS;
}

LONG Xemacpsif::createTXRing()
{
    tlog(LOG_Xemacpsif, "Creating TX Ring");
    LONG Status=XST_SUCCESS;
    XEmacPs_Bd BdTemplate;
    /*
     * Setup TxBD space.
     *
     * Like RxBD space, we have already defined a properly aligned area
     * of memory to use.
     *
     * Also like the RxBD space, we create a template. Notice we don't
     * set the "last" attribute. The example will be overriding this
     * attribute so it does no good to set it up here.
     */
    XEmacPs_BdClear(&BdTemplate);
    XEmacPs_BdSetStatus(&BdTemplate, XEMACPS_TXBUF_USED_MASK);

    /*
     * Create the TxBD ring
     */
    Status = XEmacPs_BdRingCreate(&(etherdev.TxBdRing), (UINTPTR) TxBdSpacePtrPhys, (UINTPTR) TxBdSpacePtrVirt,
				  XEMACPS_BD_ALIGNMENT, TXBD_CNT);
    if (Status != XST_SUCCESS)
    {
        tlog(LOG_Xemacpsif_Error, " Error setting up TxBD space, BdRingCreate");
      EmacPsUtilErrorTrap("Error setting up TxBD space, BdRingCreate");
      return XST_FAILURE;
    }
    Status = XEmacPs_BdRingClone(&(etherdev.TxBdRing), &BdTemplate, XEMACPS_SEND);
    if (Status != XST_SUCCESS)
    {
        tlog(LOG_Xemacpsif_Error, " Error setting up TxBD space, BdRingClone");
      EmacPsUtilErrorTrap("Error setting up TxBD space, BdRingClone");
      return XST_FAILURE;
    }
    tlog(LOG_Xemacpsif, "TX Ring created successfully");
    return XST_SUCCESS;
}

void Xemacpsif::setPriorityBuffers()
{
    /*
     * GemVersion > 2:
     * This version of GEM supports priority queuing and the current
     * driver is using tx priority queue 1 and normal rx queue for
     * packet transmit and receive. The below code ensure that the
     * other queue pointers are parked to known state for avoiding
     * the controller to malfunction by fetching the descriptors
     * from these queues.
     */
    tlog(LOG_Xemacpsif, "set priority buffers ...");
    XEmacPs_BdClear((XEmacPs_Bd *) BdRxTerminatePtrVirt);
    XEmacPs_BdSetAddressRx(BdRxTerminatePtrVirt, (XEMACPS_RXBUF_NEW_MASK |
						  XEMACPS_RXBUF_WRAP_MASK));
    tlog(LOG_Xemacpsif, "\tSetRXQ1Base BdRxTerminatePtrPhys=%p",BdRxTerminatePtrPhys);
    etherdev.SetRXQ1Base((UINTPTR) BdRxTerminatePtrPhys);

    XEmacPs_BdClear((XEmacPs_Bd *) BdTxTerminatePtrVirt);
    XEmacPs_BdSetStatus(BdTxTerminatePtrVirt, (XEMACPS_TXBUF_USED_MASK |
					       XEMACPS_TXBUF_WRAP_MASK));
    tlog(LOG_Xemacpsif, "\tSetTXQbase BdTxTerminatePtrPhys=%p",BdTxTerminatePtrPhys);
    etherdev.SetTXQbase((UINTPTR) BdTxTerminatePtrPhys);

    // Flush BD (terminate descriptor) so hardware sees it (uses virtual address)
    l4_cache_flush_data((unsigned long)BdTxTerminatePtrVirt, (unsigned long)BdTxTerminatePtrVirt+sizeof(XEmacPs_Bd));
    tlog(LOG_Xemacpsif, "end priority buffers set");
}

void Xemacpsif::setPhyLoopback()
{
    /*
     * Set emacps to phy loopback
     */
    printf("set to phy loopback ...");
    /* gemversion =7 platform=0x513 */
    etherdev.SetMdioDivisor(MDC_DIV_224);
    etherdev.EnterLoopback(EMACPS_LOOPBACK_SPEED_1G); //+
    etherdev.SetOperatingSpeed(EMACPS_LOOPBACK_SPEED_1G);
}

void Xemacpsif::initNormal()
{
    etherdev.SetOptions(XEMACPS_JUMBO_ENABLE_OPTION); // xemacps_hw.cc:83
    etherdev.SetOptions(XEMACPS_MULTICAST_OPTION);    // xemacps_hw.cc:87
    etherdev.SetMdioDivisor(MDC_DIV_224);             // xemacps_hw.cc:103
    uint32_t link_speed = this->detect_phy();
    tlog(LOG_Xemacpsif, "link_speed=%u",link_speed);
    etherdev.SetOperatingSpeed(link_speed);           // xemacs_hw.cc:152
    /* Setting the operating speed of the MAC needs a delay. */
	{
		//volatile uint32_t wait;
		//for (wait=0; wait < 20000; wait++);// TODO
		usleep(20000);
	}
}

void Xemacpsif::startDevice()
{
  printf("Starting the device ...\r\n");
  /*
   * Set the Queue pointers
   */
  tlog(LOG_Xemacpsif, "Set the Queue pointers");
  tlog(LOG_Xemacpsif, "etherdev.RxBdRing.PhysBaseAddr=%p",(void *)(etherdev.RxBdRing.PhysBaseAddr));
  etherdev.SetQueuePtr(etherdev.RxBdRing.PhysBaseAddr, 0, DevEmacPs::Recv);
  tlog(LOG_Xemacpsif, "etherdev.TxBdRing.PhysBaseAddr=%p",(void *)(etherdev.TxBdRing.PhysBaseAddr));
  etherdev.SetQueuePtr(etherdev.TxBdRing.PhysBaseAddr, 1, DevEmacPs::Send);

    etherdev.unmask();		// sonst kommt kein Interrupt

    etherdev.StartDevice();
    tlog(LOG_Xemacpsif, "Device started");
}


void Xemacpsif::stopDevice()
{
    tlog(LOG_Xemacpsif, "Stopping device ...");
    etherdev.StopDevice();
}

/**
 * fills up the ringbuffer and rx_buffer_correspond with 
 * allocated packet from the pool
 * xemacpsif_dma.c:422 setup_rx_bds
 */
int Xemacpsif::prepareReceive() // REVIEW !!!
{
    tlog(LOG_Xemacpsif_Receive, "prepareReceive: start");
    if (!RxDataBufPhys)
    {
        tlog(LOG_Xemacpsif_Error, "prepareReceive ERROR: RxDataBufPhys is NULL — did you call allocDMASpaces()?");
        return -1;
    }
    XEmacPs_Bd *rxbd = NULL;
    uint32_t freebds = XEmacPs_BdRingGetFreeCnt(&(etherdev.RxBdRing));

    tlog(LOG_Xemacpsif_Receive, "prepareReceive: freebds=%u", freebds);

    while (freebds > 0)
    {
        freebds--;

        // Allocate one BD from the ring
        LONG status = XEmacPs_BdRingAlloc(&(etherdev.RxBdRing), 1, &rxbd);
        if (status != XST_SUCCESS)
        {
            tlog(LOG_Xemacpsif_Error, "Error allocating RxBD");
            return -1;
        }

        // Determine BD index in ring
        uint32_t bdindex = XEMACPS_BD_TO_INDEX(&(etherdev.RxBdRing), rxbd);

        // Compute virtual and physical RX buffer addresses
        // NOTE: You must ensure RxDataBufVirt and RxDataBufPhys are allocated earlier!
        uint8_t *virt_buf = RxDataBufVirt + bdindex * XEMACPSIF_MAX_FRAME_SIZE;
        UINTPTR phys_buf = (UINTPTR)(RxDataBufPhys + 
                                     bdindex * XEMACPSIF_MAX_FRAME_SIZE);

        tlog(LOG_Xemacpsif_Receive, "BD[%u] -> virt RX buffer @ %p, phys @ %p",
               bdindex, (void *)virt_buf, (void *)phys_buf);

        // Associate BD with this buffer (hardware uses physical address)
        XEmacPs_BdSetAddressRx(rxbd, phys_buf);

        tlog(LOG_Xemacpsif_Receive, "XEmacPs_BdSetAddressRx(rxbd=%p, Physaddr=%p)",
               (void *)rxbd, (void *)phys_buf);
        
        // BD is empty -> clear status
        XEmacPs_BdClearRxNew(rxbd);
        tlog(LOG_Xemacpsif_Receive, "XEmacPs_BdClearRxNew(rxbd=%p)", (void *)rxbd);

        XEmacPs_BdSetStatus(rxbd, 0);
        tlog(LOG_Xemacpsif_Receive, "XEmacPs_BdSetStatus(rxbd=%p, 0)", (void *)rxbd);

        // Mark our software frame structure as empty
        rx_frames_[bdindex].len = 0;

        // Wrap bit for the last BD
        if (bdindex == RXBD_CNT - 1)
        {
            XEmacPs_BdSetRxWrap(rxbd);
            tlog(LOG_Xemacpsif_Receive, "XEmacPs_BdSetRxWrap(rxbd=%p)", (void *)rxbd);
        }
        // Cache flush for the RX DMA buffer
        // IMPORTANT: l4_cache_flush_data() requires VIRTUAL address (CPU perspective)
        // Only the physical address is used when programming the BD
        //printf("Cache flush for RX buffer: virt_buf %p (size %zu)\r\n", (void *)virt_buf, XEMACPSIF_MAX_FRAME_SIZE);
        l4_cache_flush_data((unsigned long)virt_buf,    
                            (unsigned long)virt_buf + XEMACPSIF_MAX_FRAME_SIZE);
        tlog(LOG_Xemacpsif_Receive, "l4_cache_flush_data(virt_buf=%p, virt_buf+%zu)",
               (void *)virt_buf, XEMACPSIF_MAX_FRAME_SIZE);
        
        // Cache flush for BD descriptor itself (also uses virtual address)
        l4_cache_flush_data((unsigned long)rxbd,
                            (unsigned long)rxbd + sizeof(XEmacPs_Bd));
        tlog(LOG_Xemacpsif_Receive, "l4_cache_flush_data(BD virt=%p, size=%zu)", 
               (void *)rxbd, sizeof(XEmacPs_Bd));

        // Commit BD to HW
        status = XEmacPs_BdRingToHw(&(etherdev.RxBdRing), 1, rxbd);
        tlog(LOG_Xemacpsif_Receive, "XEmacPs_BdRingToHw status=%ld", status);

        if (status != XST_SUCCESS)
        {
            //EmacPsUtilErrorTrap("Error committing RxBD to HW");
            XEmacPs_BdRingUnAlloc(&(etherdev.RxBdRing), 1, rxbd);
            EmacPsUtilErrorTrap("Error committing RxBD to HW");
            return XST_FAILURE;
        }
    }
    tlog(LOG_Xemacpsif_Receive, "prepareReceive: done");
    return 0;
}


EmacRawFrame *Xemacpsif::receive()// REVIEW
{
    tlog(LOG_Xemacpsif_Receive, "Calling recvqueue_.remove()");
    
    EmacRawFrame *frame = recvqueue_.remove();
    
    if (frame) {
        tlog(LOG_Xemacpsif_Receive, "Frame received: ptr=%p, len=%u", (void*)frame, frame->len);
        /*
        #if defined(LOG_Xemacpsif_Receive)
        std::cout << "Frame Dump = ";
        Dumputility::dump_hex(std::cout,
                            (octet_t *)frame,
                            frame->len,
                            0);
        std::cout << std::endl;
        #endif
        */
    } else {
        tlog(LOG_Xemacpsif_Receive, "No frame available in queue");
    }

    return frame;
}

// int Xemacpsif::send(NX_PACKET *ts)// REVIEW 
int Xemacpsif::send(const uint8_t *data, uint32_t len)
{
    // Basic sanity checks
    if (!TxDataBufVirt || !TxDataBufPhys)
    {
        tlog(LOG_Xemacpsif_Error, "Xemacpsif::send ERROR: TxDataBufVirt/Phys is NULL — allocDMASpaces() missing?");
        return XST_FAILURE;
    }

    if (!data || len == 0 || len > XEMACPSIF_MAX_FRAME_SIZE)
    {
        tlog(LOG_Xemacpsif_Error, "Xemacpsif::send ERROR: invalid frame length %u", len);
        return XST_FAILURE;
    }

    XEmacPs_Bd *Bd1Ptr = nullptr;
    pthread_mutex_lock(&mutex_);
  /*
   * Allocate, setup, and enqueue 1 TxBDs. The first BD will
   * describe the first 32 bytes of TxFrame and the rest of BDs
   * will describe the rest of the frame.
   *
   * The function below will allocate 1 adjacent BDs with Bd1Ptr
   * being set as the lead BD.
   */
    LONG Status = XEmacPs_BdRingAlloc(&(etherdev.TxBdRing), 1, &Bd1Ptr);
    if (Status != XST_SUCCESS)
    {
        tlog(LOG_Xemacpsif_Error, "Error allocating TxBD");
        pthread_mutex_unlock(&mutex_);// REVIEW
        return XST_FAILURE;
    }

    tlog(LOG_Xemacpsif_Send, "Xemacpsif::send (len=%u)", len);

    // Determine BD index in ring so we can pick a matching TX buffer slice
    uint32_t bdindex = XEMACPS_BD_TO_INDEX(&(etherdev.TxBdRing), Bd1Ptr);

    // Compute virtual + physical TX buffer address for this BD
    uint8_t *virt_buf = TxDataBufVirt + bdindex * XEMACPSIF_MAX_FRAME_SIZE;
    UINTPTR phys_buf  = (UINTPTR)(TxDataBufPhys + bdindex * XEMACPSIF_MAX_FRAME_SIZE);

    // Copy frame into DMA buffer
    memcpy(virt_buf, data, len);

    // Flush cache for TX buffer so DMA sees correct data (uses virtual address)
    // This ensures the DMA engine sees our newly written frame data
    l4_cache_flush_data((unsigned long)virt_buf,
                        (unsigned long)virt_buf + len);

    /*
     * Setup TxBD
     */
    tlog(LOG_Xemacpsif_Send, "TxFrameLength=%u", len);
    tlog(LOG_Xemacpsif_Send, "XEmacPs_BdSetAddressTx(Bd1Ptr=%p, Physaddr=%p)",
           (void *)Bd1Ptr, (void *)phys_buf);

    XEmacPs_BdSetAddressTx(Bd1Ptr, phys_buf);
    XEmacPs_BdSetLength(Bd1Ptr, len);
    XEmacPs_BdClearTxUsed(Bd1Ptr);
    XEmacPs_BdSetLast(Bd1Ptr);

    #if defined(LOG_Xemacpsif_Send)
    std::cout << "Bd1Ptr before ToHw = ";
    Dumputility::dump_hex(std::cout,
                          (octet_t *)Bd1Ptr,
                          sizeof(XEmacPs_Bd),
                          0);
    std::cout << std::endl;
    #endif
    // Flush BD itself (also uses virtual address)
    // This ensures the DMA engine sees our updated descriptor
    l4_cache_flush_data((unsigned long)Bd1Ptr,
                        (unsigned long)Bd1Ptr + sizeof(XEmacPs_Bd));

    /*
     * Enqueue to HW
     */
    Status = XEmacPs_BdRingToHw(&(etherdev.TxBdRing), 1, Bd1Ptr);
    if (Status != XST_SUCCESS)
    {
        tlog(LOG_Xemacpsif_Error, "Error committing TxBD to HW");
        pthread_mutex_unlock(&mutex_);// REVIEW
        return XST_FAILURE;
    }

    tlog(LOG_Xemacpsif_Send, "Start transmit");
    etherdev.Transmit();

    /*
     * Wait for transmission to complete
     * (notifySend() should signal 'signalsendfinished_')// TODO
     */
    pthread_cond_wait(&signalsendfinished_, &mutex_);

    #if defined(LOG_Xemacpsif_Send)
    std::cout << "Bd1Ptr after transmit = ";
    Dumputility::dump_hex(std::cout,
                          (octet_t *)Bd1Ptr,
                          sizeof(XEmacPs_Bd),
                          0);
    std::cout << std::endl;
    #endif
    /*
     * Now that the frame has been sent, post process our TxBDs.
     * Since we have only submitted 1 to hardware, there should
     * be only 1 ready for post processing.
     */
    if (XEmacPs_BdRingFromHwTx(&(etherdev.TxBdRing), 1, &Bd1Ptr) == 0)
    {
        tlog(LOG_Xemacpsif_Error, "TxBDs were not ready for post processing");
        pthread_mutex_unlock(&mutex_);
        return XST_FAILURE;
    }

    #if defined(LOG_Xemacpsif_Send)
    std::cout << "Bd1Ptr after FromHw = ";
    Dumputility::dump_hex(std::cout,
                          (octet_t *)Bd1Ptr,
                          sizeof(XEmacPs_Bd),
                          0);
    std::cout << std::endl;
    #endif
   /*
    * Examine the TxBDs.
    *
    * There isn't much to do. The only thing to check would be DMA
    * exception bits. But this would also be caught in the error
    * handler. So we just return these BDs to the free list.
    */
    Status = XEmacPs_BdRingFree(&(etherdev.TxBdRing), 1, Bd1Ptr);
    if (Status != XST_SUCCESS)
    {
        tlog(LOG_Xemacpsif_Error, "Error freeing up TxBDs");
        pthread_mutex_unlock(&mutex_);
        return XST_FAILURE;
    }

    pthread_mutex_unlock(&mutex_);
    tlog(LOG_Xemacpsif_Send, "Frame sent successfully");
    return XST_SUCCESS;
}

void Xemacpsif::notifySend()
{
  /*
   * signal senderthread
   */
  tlog(LOG_Xemacpsif_Send, "notifySend(): TX completed");
  pthread_mutex_lock(&mutex_);
  pthread_cond_signal(&signalsendfinished_);
  pthread_mutex_unlock(&mutex_);
  tlog(LOG_Xemacpsif_Send, "notifySend(): signal sent");
}

void Xemacpsif::notifyReceive() // REVIEW
{
    XEmacPs_BdRing *rxring = &(etherdev.RxBdRing);
    XEmacPs_Bd *rxbdset = NULL;
    XEmacPs_Bd *curbdptr = NULL;
    volatile int32_t bd_processed = 0;
    size_t RxFrLen = 0;
    tlog(LOG_Xemacpsif_Receive, "start");
    while ((bd_processed = XEmacPs_BdRingFromHwRx(rxring, RXBD_CNT, &rxbdset)) > 0)
    {
        tlog(LOG_Xemacpsif_Receive, "bd_processed=%d", bd_processed);
        // REVIEW : k , curbdptr
        for (int32_t k = 0; k < bd_processed; k++)
        {
            curbdptr = (k == 0 ? rxbdset
                               : XEmacPs_BdRingNext(rxring, curbdptr));

            uint32_t bdindex = XEMACPS_BD_TO_INDEX(rxring, curbdptr);

            // Compute buffer addresses for this BD
            uint8_t *virt_buf = RxDataBufVirt + bdindex * XEMACPSIF_MAX_FRAME_SIZE;
            // UINTPTR phys_buf = (UINTPTR)(RxDataBufPhys + bdindex * XEMACPSIF_MAX_FRAME_SIZE);
            // (phys not needed here except for debugging)

            // Invalidate cache BEFORE reading DMA buffer (uses virtual address)
            // This ensures we read fresh data from main memory that the DMA wrote
            l4_cache_flush_data((unsigned long)virt_buf,
                                     (unsigned long)virt_buf + XEMACPSIF_MAX_FRAME_SIZE);

            // Get frame length (depends on GEM version)
            if (GemVersion > 2)
                RxFrLen = etherdev.GetRxFrameSize(curbdptr);
            else
                RxFrLen = XEmacPs_BdGetLength(curbdptr);

            tlog(LOG_Xemacpsif_Receive, "Received index=%u length=%lu", bdindex, RxFrLen);

            // Fill software raw frame structure
            EmacRawFrame *f = &rx_frames_[bdindex];
            f->len = RxFrLen;
            memcpy(f->data, virt_buf, RxFrLen);

            /* enqueue */
            recvqueue_.insert(f);
        }

        /* free up the BD's */
        XEmacPs_BdRingFree(rxring, bd_processed, rxbdset);

        /* orginalcode macht setup_rx_bds = prepare_receive */
        prepareReceive();
    }
    tlog(LOG_Xemacpsif_Receive, "notifyReceive: done");
}

// REVIEW
void Xemacpsif::releaseReceivedFrame(EmacRawFrame *frame)
{
    // Nothing complicated needed:
    // We simply mark this software frame struct as empty.
    frame->len = 0;
    memset(frame->data, 0, XEMACPSIF_MAX_FRAME_SIZE);// REVIEW : not nissary
    tlog(LOG_Xemacpsif_Receive, "releaseReceivedFrame: frame %p released", (void*)frame);
}


void Xemacpsif::notifyError(uint8_t, uint32_t)
{
  /*
   * Increment the counter so that main thread knows something
   * happened. Reset the device and reallocate resources ...
   */
  DeviceErrors++;
  tlog(LOG_Xemacpsif_Error, "notifyError: Device error occurred, total errors=%d", DeviceErrors);
}

#define PHY_DETECT_REG  						1
#define PHY_IDENTIFIER_1_REG					2
#define PHY_IDENTIFIER_2_REG					3
#define PHY_DETECT_MASK 					0x1808
#define PHY_MARVELL_IDENTIFIER				0x0141
#define PHY_TI_IDENTIFIER					0x2000
#define PHY_ADI_IDENTIFIER				0x0283
#define PHY_REALTEK_IDENTIFIER				0x001c
#define PHY_XILINX_PCS_PMA_ID1			0x0174
#define PHY_XILINX_PCS_PMA_ID2			0x0C00


uint32_t Xemacpsif::detect_phy()
{
    tlog(LOG_Xemacpsif, "detect_phy: start");
    uint32_t phyfoundforemac0 = FALSE;
	uint32_t phyfoundforemac1 = FALSE;
    uint32_t phyaddrforemac=0;
    uint32_t link_speed = XST_FAILURE;
    uint32_t phy_addr = 0;
	uint32_t emacnum = 0;
    for(phy_addr=0; phy_addr < 32; phy_addr++)
    {
       phymapemac0[phy_addr]=0;
       phymapemac0[phy_addr]=0;
    }

	if (etherdev.getPhysaddr() == (uint8_t *)XPAR_XEMACPS_0_BASEADDR)
    {
        emacnum = 0;
    }
	else
    {
		emacnum = 1;
    }
	tlog(LOG_Xemacpsif, "detect_phy: emacnum=%d",emacnum);

	for (phy_addr = 31; phy_addr > 0; phy_addr--)
    {
       phy_identify(phy_addr, emacnum);
	}

	for (uint32_t i = 31; i > 0; i--)
    {
		if (etherdev.getPhysaddr() == (uint8_t *)XPAR_XEMACPS_0_BASEADDR)
        {
			if (phymapemac0[i] == TRUE)
            {
				link_speed = phy_setup_emacps(i);
				phyfoundforemac0 = TRUE;
				phyaddrforemac = i;
			}
		}
		else
        {
			if (phymapemac1[i] == TRUE)
            {
				link_speed = phy_setup_emacps(i);
				phyfoundforemac1 = TRUE;
				phyaddrforemac = i;
			}
		}
	}
	/* If no PHY was detected, use broadcast PHY address of 0 */
	if (etherdev.getPhysaddr() == (uint8_t *)XPAR_XEMACPS_0_BASEADDR)
    {
		if (phyfoundforemac0 == FALSE)
        {
			link_speed = phy_setup_emacps(0);
        }
	}
	else
    {
		if (phyfoundforemac1 == FALSE)
        {
			link_speed = phy_setup_emacps(0);
        }
	}


	if (link_speed == XST_FAILURE)
    {
		eth_link_status_ = ETH_LINK_DOWN;
		tlog(LOG_Xemacpsif_Error, "Phy setup failure %s",__func__);
	}
	else
    {
		eth_link_status_ = ETH_LINK_UP;
	}
  	(void) phyaddrforemac;
    tlog(LOG_Xemacpsif, "detect_phy: end, link_speed=%u",link_speed);
    return link_speed;
}

void Xemacpsif::phy_identify(uint32_t phy_addr, uint32_t emacnum)
{
	uint16_t phy_reg=0;
	uint16_t phy_id =0;
    tlog(LOG_Xemacpsif, "phy_identify: start, phy_addr=%u, emacnum=%u", phy_addr, emacnum);
	etherdev.PhyRead(phy_addr, PHY_DETECT_REG,	&phy_reg);
	etherdev.PhyRead(phy_addr, PHY_IDENTIFIER_1_REG,&phy_id);

	if (((phy_reg != 0xFFFF) &&
	     ((phy_reg & PHY_DETECT_MASK) == PHY_DETECT_MASK)) ||
	    (phy_id == PHY_XILINX_PCS_PMA_ID1))
    {
		/* Found a valid PHY address */
		printf("XEmacPs detect_phy: PHY detected at address %d.\r\n", phy_addr);
		if (emacnum == 0)
        {
			phymapemac0[phy_addr] = TRUE;
		}
		else
        {
			phymapemac1[phy_addr] = TRUE;
		}

		etherdev.PhyRead(phy_addr, PHY_IDENTIFIER_1_REG,&phy_reg);
		if ((phy_reg != PHY_MARVELL_IDENTIFIER) &&
		    (phy_reg != PHY_TI_IDENTIFIER) &&
		    (phy_reg != PHY_REALTEK_IDENTIFIER) &&
		    (phy_reg != PHY_ADI_IDENTIFIER)) {
            
			tlog(LOG_Xemacpsif_Warning, "WARNING: Not a Marvell or TI or Realtek or Xilinx PCS PMA Ethernet PHY or ADI Ethernet PHY. Please verify the initialization sequence");
		}
	}
}

#define XEMACPS_GMII2RGMII_SPEED1000_FD		0x140
#define XEMACPS_GMII2RGMII_SPEED100_FD		0x2100
#define XEMACPS_GMII2RGMII_SPEED10_FD		0x100
#define XEMACPS_GMII2RGMII_REG_NUM			0x10


uint32_t Xemacpsif::phy_setup_emacps(uint32_t phy_addr)
{
    tlog(LOG_Xemacpsif, "phy_setup_emacps: start, phy_addr=%u", phy_addr);
	uint32_t link_speed;
	uint32_t conv_present = 0;
	uint32_t convspeeddupsetting = 0;
	uint32_t convphyaddr = 0;

    link_speed = etherdev.get_IEEE_phy_speed(phy_addr);
	if (link_speed == 1000)
    {
		crl_arb.SetUpSLCRDivisors(1000);
		convspeeddupsetting = XEMACPS_GMII2RGMII_SPEED1000_FD;
	}
	else if (link_speed == 100)
    {
		crl_arb.SetUpSLCRDivisors(100);
		convspeeddupsetting = XEMACPS_GMII2RGMII_SPEED100_FD;
	}
	else if (link_speed != XST_FAILURE)
    {
		crl_arb.SetUpSLCRDivisors(10);
		convspeeddupsetting = XEMACPS_GMII2RGMII_SPEED10_FD;
	}
	else
    {
		tlog(LOG_Xemacpsif_Error, "Phy setup error ");
		return XST_FAILURE;
	}

	if (conv_present)
    {
		etherdev.PhyWrite(convphyaddr,	XEMACPS_GMII2RGMII_REG_NUM, convspeeddupsetting);
	}
	tlog(LOG_Xemacpsif, "link speed for phy address %d: %d", phy_addr, link_speed);
	return link_speed;
}


