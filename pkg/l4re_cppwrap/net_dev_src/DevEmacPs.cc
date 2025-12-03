/*
 * DevEmacPs.cpp
 *
 *  Created on: Apr 14, 2022
 *      Author: weber
 */

#include "DevEmacPs.h"
#include <l4/io/io.h>
#include <l4/sys/cache.h>
#include <iostream>
#include <unistd.h>
#include "/media/iss/Arbeit/Vitis_install/2025.1/data/embeddedsw/ThirdParty/sw_services/lwip220_v1_2/src/lwip-2.2.0/contrib/ports/xilinx/netif/xemac_ieee_reg.h"

DevEmacPs::DevEmacPs():IORegion("EmacPs"),
isStarted_(0), isReady_(0),
    _icu(l4io_request_icu()),// ICU bekommen (virtueller interrupt controller) 
    observer_(0)
{
}

DevEmacPs::~DevEmacPs()
{
}

void DevEmacPs::initmem(l4_addr_t phyaddr_start, l4_addr_t phyaddr_end)
{
  IORegion::initmem(phyaddr_start,phyaddr_end);
  isReady_ = XIL_COMPONENT_IS_READY;
}


void DevEmacPs::SetQueuePtr(UINTPTR QPtr, uint8_t QueueNum, Direction_t Direction)
{
  const uint32_t TXQBASE_OFFSET = 0x0000001CU; /**< TX Q Base address reg */
  const uint32_t RXQBASE_OFFSET = 0x00000018U; /**< RX Q Base address reg */
  const uint32_t TXQ1BASE_OFFSET = 0x00000440U;	/**< TX Q1 Base address reg */
  const uint32_t MSBBUF_TXQBASE_OFFSET = 0x000004C8U; /**< MSB Buffer TX Q Base reg */
  const uint32_t MSBBUF_RXQBASE_OFFSET = 0x000004D4U; /**< MSB Buffer RX Q Base reg */
  //Xil_AssertVoid(InstancePtr->IsReady == (uint32_t)XIL_COMPONENT_IS_READY);

  /* If already started, then there is nothing to do */
  if (isStarted_ == (uint32_t) XIL_COMPONENT_IS_STARTED)
  {
    printf("DevEmacPs::SetQueuePtr: COMPONENT_IS_STARTED\r\n");
    return;
  }
  
  printf("DevEmacPs::SetQueuePtr(QPtr=%p,QueueNum=%x,%s)\r\n",(void *)QPtr,(unsigned int)QueueNum,
            (Direction == Send ? "Send" : "Receive"));
  if (QueueNum == 0x00U)
  {
    if (Direction == Send)
    {
      Out32(TXQBASE_OFFSET, ((uint64_t) QPtr & ULONG64_LO_MASK));
    }
    else
    {
      Out32(RXQBASE_OFFSET, ((uint64_t) QPtr & ULONG64_LO_MASK));
    }
  }
  else
  {
    Out32(TXQ1BASE_OFFSET, ((uint64_t) QPtr & ULONG64_LO_MASK));
  }
#ifdef __aarch64__
  if (Direction == Send)
  {
    /* Set the MSB of TX Queue start address */
    Out32(MSBBUF_TXQBASE_OFFSET, (uint32_t) (((uint64_t) QPtr & ULONG64_HI_MASK) >> 32U));
  }
  else
  {
    /* Set the MSB of RX Queue start address */
    Out32(MSBBUF_RXQBASE_OFFSET, (uint32_t) (((uint64_t) QPtr & ULONG64_HI_MASK) >> 32U));
  }
#endif
}

/**
* Start the Ethernet controller as follows:
*   - Enable transmitter if XTE_TRANSMIT_ENABLE_OPTION is set
*   - Enable receiver if XTE_RECEIVER_ENABLE_OPTION is set
*   - Start the SG DMA send and receive channels and enable the device
*     interrupt
*
* @param InstancePtr is a pointer to the instance to be worked on.
*
* @return N/A
*
* @note
* Hardware is configured with scatter-gather DMA, the driver expects to start
* the scatter-gather channels and expects that the user has previously set up
* the buffer descriptor lists.
*
* This function makes use of internal resources that are shared between the
* Start, Stop, and Set/ClearOptions functions. So if one task might be setting
* device options while another is trying to start the device, the user is
* required to provide protection of this shared data (typically using a
* semaphore).
*
* This function must not be preempted by an interrupt that may service the
* device.
*
******************************************************************************/
void DevEmacPs::StartDevice()
{
  uint32_t Reg;

  /* Assert bad arguments and conditions */
  //Xil_AssertVoid(IsReady_ == (uint32_t)XIL_COMPONENT_IS_READY);
  printf("StartDevice\n\r");
  /* Start DMA */
  /* When starting the DMA channels, both transmit and receive sides
   * need an initialized BD list.
   */
  if (Version == 2) // da version==7 wird es nicht ausgeführt
  {
    //Xil_AssertVoid(InstancePtr->RxBdRing.BaseBdAddr != 0);
    //Xil_AssertVoid(InstancePtr->TxBdRing.BaseBdAddr != 0);
    Out32(XEMACPS_RXQBASE_OFFSET, RxBdRing.PhysBaseAddr);
    Out32(XEMACPS_TXQBASE_OFFSET, TxBdRing.PhysBaseAddr);
  }

  /* clear any existed int status */
  Out32(XEMACPS_ISR_OFFSET, XEMACPS_IXR_ALL_MASK);

  /* Enable transmitter if not already enabled */
  if ((options_ & (uint32_t) XEMACPS_TRANSMITTER_ENABLE_OPTION)!=0x00000000U)
  {
    Reg = In32(XEMACPS_NWCTRL_OFFSET);
    if ((!(Reg & XEMACPS_NWCTRL_TXEN_MASK)) == TRUE)
    {
      Out32(XEMACPS_NWCTRL_OFFSET, Reg | (uint32_t) XEMACPS_NWCTRL_TXEN_MASK);
    }
  }

  /* Enable receiver if not already enabled */
  if ((options_ & XEMACPS_RECEIVER_ENABLE_OPTION) != 0x00000000U)
  {
    Reg = In32(XEMACPS_NWCTRL_OFFSET);
    if ((!(Reg & XEMACPS_NWCTRL_RXEN_MASK)) == TRUE)
    {
      Out32(XEMACPS_NWCTRL_OFFSET, Reg | (uint32_t) XEMACPS_NWCTRL_RXEN_MASK);
    }
  }

  /* Enable TX and RX interrupts */
  intenable((XEMACPS_IXR_TX_ERR_MASK | XEMACPS_IXR_RX_ERR_MASK | (uint32_t) XEMACPS_IXR_FRAMERX_MASK | (uint32_t) XEMACPS_IXR_TXCOMPL_MASK));

  /* Enable TX Q1 Interrupts */
  if (Version > 2)
  {
    intQ1Enable(XEMACPS_INTQ1_IXR_ALL_MASK);
  }
  /* Mark as started */
  isStarted_ = XIL_COMPONENT_IS_STARTED;

  return;
}


/*****************************************************************************/
/**
* Gracefully stop the Ethernet MAC as follows:
*   - Disable all interrupts from this device
*   - Stop DMA channels
*   - Disable the transmitter and receiver
*
* Device options currently in effect are not changed.
*
* This function will disable all interrupts. Default interrupts settings that
* had been enabled will be restored when XEmacPs_Start() is called.
*
* @param InstancePtr is a pointer to the instance to be worked on.
*
* @note
* This function makes use of internal resources that are shared between the
* Start, Stop, SetOptions, and ClearOptions functions. So if one task might be
* setting device options while another is trying to start the device, the user
* is required to provide protection of this shared data (typically using a
* semaphore).
*
* Stopping the DMA channels causes this function to block until the DMA
* operation is complete.
*
* Source: xemacps.c:242
******************************************************************************/
void DevEmacPs::StopDevice()
{
  const uint32_t NWCTRL_OFFSET = 0x00000000U;  /**< Network Control reg */
  const uint32_t NWCTRL_RXEN_MASK = 0x00000004U;/**< Enable receive */
  const uint32_t NWCTRL_TXEN_MASK = 0x00000008U;/**< Enable transmit */
  const uint32_t IDR_OFFSET = 0x0000002CU;     /**< Interrupt Status reg */
  const uint32_t IXR_ALL_MASK = 0x00007FFFU;

  uint32_t Reg = 0;
  printf("StopDevice\n\r");
  /* Disable all interrupts */
  Out32(IDR_OFFSET, IXR_ALL_MASK);

  /* Disable the receiver & transmitter */
  Reg = In32(NWCTRL_OFFSET);
  Reg &= (uint32_t) (~NWCTRL_RXEN_MASK);
  Reg &= (uint32_t) (~NWCTRL_TXEN_MASK);
  Out32(NWCTRL_OFFSET, Reg);

  /* Mark as stopped */
  isStarted_ = 0U;
}

void DevEmacPs::ResetDevice()
{
  uint32_t Reg;
  uint8_t i;
  int8_t EmacPs_zero_MAC[6] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
  printf("Reset Device\n\r"); 
  printf("Set NWCFG to reset value\r\n");
  Out32(XEMACPS_NWCFG_OFFSET,0x280000);
  Out32(XEMACPS_NWCTRL_OFFSET,0x0);
  Out32(XEMACPS_LADDR1L_OFFSET,0);
  Out32(XEMACPS_LADDR1H_OFFSET,0);
  /* Stop the device and reset hardware */
  StopDevice();

  options_ = XEMACPS_DEFAULT_OPTIONS;
  Version = GemVersion();
  MaxMtuSize = XEMACPS_MTU;
  MaxFrameSize = XEMACPS_MTU + XEMACPS_HDR_SIZE + XEMACPS_TRL_SIZE;
  MaxVlanFrameSize = MaxFrameSize + XEMACPS_HDR_VLAN_SIZE;
  RxBufMask = XEMACPS_RXBUF_LEN_MASK;

  /* Setup hardware with default values */
  const uint32_t NWCTRL_OFFSET = 0x00000000U;	    /**< Network Control reg */
  const uint32_t NWCTRL_STATCLR_MASK = 0x00000020U;	   /**< Clear statistic registers */
  const uint32_t NWCTRL_MDEN_MASK = 0x00000010U;	   /**< Enable MDIO port */
  const uint32_t NWCTRL_LOOPEN_MASK = 0x00000002U;	   /**< local loopback */
  Out32(NWCTRL_OFFSET,(NWCTRL_STATCLR_MASK | NWCTRL_MDEN_MASK) & (uint32_t) (~NWCTRL_LOOPEN_MASK));

  const uint32_t NWCFG_OFFSET = 0x00000004U;		/**< Network Config reg */
  const uint32_t NWCFG_MDCCLKDIV_MASK = 0x001C0000U;	   /**< MDC Mask PCLK divisor */
  const uint32_t NWCFG_100_MASK = 0x00000001U;		   /**< 100 Mbps */
  const uint32_t NWCFG_FDEN_MASK = 0x00000002U;		   /**< full duplex */
  const uint32_t NWCFG_UCASTHASHEN_MASK = 0x00000080U;	     /**< Receive unicast hash frames */

  Reg = In32(NWCFG_OFFSET);
  Reg &= NWCFG_MDCCLKDIV_MASK;
  Reg = Reg | (uint32_t) NWCFG_100_MASK | (uint32_t) NWCFG_FDEN_MASK | (uint32_t) NWCFG_UCASTHASHEN_MASK;
  Out32(NWCFG_OFFSET, Reg);
  //if(GemVersion() > 2)
  {
    const uint32_t NWCFG_DWIDTH_64_MASK = 0x00200000U;	      /**< 64 bit Data bus width */
    Out32(NWCFG_OFFSET, In32(NWCFG_OFFSET) | NWCFG_DWIDTH_64_MASK);
  }
  const uint32_t DMACR_OFFSET = 0x00000010U;   /**< DMA Control reg */
  const uint32_t RX_BUF_SIZE = 1536U;	 /**< Specify the receive buffer size in
                                       bytes, 64, 128, ... 10240 */
  const uint32_t RX_BUF_UNIT = 64U;    /**< Number of receive buffer bytes as a
                                       unit, this is HW setup */
  const uint32_t DMACR_RXBUF_SHIFT = 16U;	/**< Shift bit for RX buffer size */
  const uint32_t DMACR_RXBUF_MASK = 0x00FF0000U;    /**< Mask bit for RX buffer size */
  const uint32_t DMACR_RXSIZE_MASK = 0x00000300U;   /**< RX buffer memory size */
  const uint32_t DMACR_TXSIZE_MASK = 0x00000400U;   /**< TX buffer memory size */
  Out32(DMACR_OFFSET,
	((((RX_BUF_SIZE / RX_BUF_UNIT) +
	   ((((RX_BUF_SIZE %
	       RX_BUF_UNIT)) != (uint32_t) 0) ? 1U : 0U)) << DMACR_RXBUF_SHIFT) &
	 DMACR_RXBUF_MASK) | DMACR_RXSIZE_MASK | DMACR_TXSIZE_MASK);

  if (Version > 2)
  {
    const uint32_t DMACR_ADDR_WIDTH_64 = 0x40000000U;	  /**< 64 bit address bus */
    const uint32_t DMACR_INCR16_AHB_BURST = 0x00000010U;     /**< 16 bytes AHB bursts */
    Out32(DMACR_OFFSET, (In32(DMACR_OFFSET) |
#if defined(__aarch64__) || defined(__arch64__)
			 DMACR_ADDR_WIDTH_64 |
#endif
			 DMACR_INCR16_AHB_BURST));
  }
  const uint32_t TXSR_OFFSET = 0x00000014U;   /**< TX Status reg */
  const uint32_t SR_ALL_MASK = 0xFFFFFFFFU;   /**< Mask for full register */
  Out32(TXSR_OFFSET, SR_ALL_MASK);

  SetQueuePtr(0, 0x00U, Send);
  if (Version > 2)
  {
    SetQueuePtr(0, 0x01U, Send);
  }
  SetQueuePtr(0, 0x00U, Recv);
  const uint32_t RXSR_OFFSET = 0x00000020U; /**< RX Status reg */
  Out32(RXSR_OFFSET, SR_ALL_MASK);

  const uint32_t IDR_OFFSET = 0x0000002CU; /**< Interrupt Disable reg */
  const uint32_t IXR_ALL_MASK = 0x00007FFFU;	/**< Everything! */

  Out32(IDR_OFFSET, IXR_ALL_MASK);

  const uint32_t ISR_OFFSET = 0x00000024U;	/**< Interrupt Status reg */
  Reg = In32(ISR_OFFSET);
  Out32(ISR_OFFSET, Reg);

  ClearHash();

  for (i = 1U; i < 5U; i++)
  {
    (void) SetMacAddress(EmacPs_zero_MAC, i);
    (void) SetTypeIdCheck(0x00000000U, i);
  }

  /* clear all counters */
  const uint32_t OCTTXL_OFFSET = 0x00000100U;	  /**< Octects transmitted Low reg */
  const uint32_t LAST_OFFSET = 0x000001B4U;	  /**< Last statistic counter offset, for clearing */
  for (i = 0U; i < (uint8_t) ((LAST_OFFSET - OCTTXL_OFFSET) / 4U); i++)
  {
    In32(OCTTXL_OFFSET + (uint32_t) (((uint32_t) i) * ((uint32_t) 4)));
  }

  /* Disable the receiver */
  const uint32_t NWCTRL_RXEN_MASK = 0x00000004U;     /**< Enable receive */
  Reg = In32(NWCTRL_OFFSET);
  Reg &= (uint32_t) (~NWCTRL_RXEN_MASK);
  Out32(NWCTRL_OFFSET, Reg);

  /* Sync default options with hardware but leave receiver and
   * transmitter disabled. They get enabled with XEmacPs_Start() if
   * XEMACPS_TRANSMITTER_ENABLE_OPTION and
   * XEMACPS_RECEIVER_ENABLE_OPTION are set.
   */
  (void) SetOptions(options_ &
		    ~((uint32_t) XEMACPS_TRANSMITTER_ENABLE_OPTION |
		      (uint32_t) XEMACPS_RECEIVER_ENABLE_OPTION));

  (void) ClearOptions(~options_);
}

/*****************************************************************************/
/**
 * Clear the Hash registers for the mac address pointed by AddressPtr.
 *
 * @param InstancePtr is a pointer to the instance to be worked on.
 *
 *****************************************************************************/
void DevEmacPs::ClearHash()
{
  //Xil_AssertVoid(InstancePtr->IsReady == (uint32_t)XIL_COMPONENT_IS_READY);
  const uint32_t HASHL_OFFSET = 0x00000080U;	/*< Hash Low address reg */
  const uint32_t HASHH_OFFSET = 0x00000084U; /**< Hash High address reg */
  Out32(HASHL_OFFSET, 0x0U);

  /* write bits [63:32] in TOP */
  Out32(HASHH_OFFSET, 0x0U);
}

/*****************************************************************************/
/**
 * Set the MAC address for this driver/device.  The address is a 48-bit value.
 * The device must be stopped before calling this function.
 *
 * @param InstancePtr is a pointer to the instance to be worked on.
 * @param AddressPtr is a pointer to a 6-byte MAC address.
 * @param Index is a index to which MAC (1-4) address.
 *
 * @return
 * - XST_SUCCESS if the MAC address was set successfully
 * - XST_DEVICE_IS_STARTED if the device has not yet been stopped
 *
 *****************************************************************************/
long DevEmacPs::SetMacAddress(void *AddressPtr, uint8_t Index)
{
  uint32_t MacAddr;
  uint8_t *Aptr = (uint8_t *) AddressPtr;
  uint8_t IndexLoc = Index;
  long Status;

  //Xil_AssertNonvoid(Aptr != NULL);
  //Xil_AssertNonvoid(InstancePtr->IsReady == (uint32_t)XIL_COMPONENT_IS_READY);
  //Xil_AssertNonvoid((IndexLoc <= (uint8_t)XEMACPS_MAX_MAC_ADDR) && (IndexLoc > 0x00U));

  /* Be sure device has been stopped */
  if (isStarted_ == (uint32_t) XIL_COMPONENT_IS_STARTED)
  {
    Status = (long) (XST_DEVICE_IS_STARTED);
  }
  else
  {
    /* Index ranges 1 to 4, for offset calculation is 0 to 3. */
    IndexLoc--;
    const uint32_t LADDR1L_OFFSET = 0x00000088U;   /**< Specific1 addr low reg */
    const uint32_t LADDR1H_OFFSET = 0x0000008CU;   /**< Specific1 addr high reg */
    const uint32_t LADDR_MACH_MASK = 0x0000FFFFU;   /**< Address bits[47:32]  bit[31:0] are in BOTTOM */
    /* Set the MAC bits [31:0] in BOT */
    MacAddr = *(Aptr);
    MacAddr |= ((uint32_t) (*(Aptr + 1)) << 8U);
    MacAddr |= ((uint32_t) (*(Aptr + 2)) << 16U);
    MacAddr |= ((uint32_t) (*(Aptr + 3)) << 24U);
    Out32((LADDR1L_OFFSET + ((uint32_t) IndexLoc * (uint32_t) 8)), MacAddr);

    /* There are reserved bits in TOP so don't affect them */
    MacAddr = In32(LADDR1H_OFFSET + ((uint32_t) IndexLoc * (uint32_t) 8));

    MacAddr &= ~LADDR_MACH_MASK;

    /* Set MAC bits [47:32] in TOP */
    MacAddr |= (uint32_t) (*(Aptr + 4));
    MacAddr |= (uint32_t) (*(Aptr + 5)) << 8U;

    Out32((LADDR1H_OFFSET + ((uint32_t) IndexLoc * (uint32_t) 8)), MacAddr);

    Status = (LONG) (XST_SUCCESS);
  }
  return Status;
}

/*****************************************************************************/
/**
 * Get the MAC address for this driver/device.
 *
 * @param InstancePtr is a pointer to the instance to be worked on.
 * @param AddressPtr is an output parameter, and is a pointer to a buffer into
 *        which the current MAC address will be copied.
 * @param Index is a index to which MAC (1-4) address.
 *
 *****************************************************************************/
void DevEmacPs::GetMacAddress(void *AddressPtr, uint8_t Index)
{
  const uint32_t LADDR1L_OFFSET = 0x00000088U;	   /**< Specific1 addr low reg */
  const uint32_t LADDR1H_OFFSET = 0x0000008CU;	   /**< Specific1 addr high reg */
  uint32_t MacAddr;
  uint8_t *Aptr = (uint8_t *) (void *) AddressPtr;
  uint8_t IndexLoc = Index;

  //Xil_AssertVoid(Aptr != NULL);
  //Xil_AssertVoid(InstancePtr->IsReady == (uint32_t)XIL_COMPONENT_IS_READY);
  //Xil_AssertVoid((IndexLoc <= (uint8_t)XEMACPS_MAX_MAC_ADDR) && (IndexLoc > 0x00U));

  /* Index ranges 1 to 4, for offset calculation is 0 to 3. */
  IndexLoc--;

  MacAddr = In32(LADDR1L_OFFSET + ((uint32_t) IndexLoc * (uint32_t) 8));
  *Aptr = (uint8_t) MacAddr;
  *(Aptr + 1) = (uint8_t) (MacAddr >> 8U);
  *(Aptr + 2) = (uint8_t) (MacAddr >> 16U);
  *(Aptr + 3) = (uint8_t) (MacAddr >> 24U);

  /* Read MAC bits [47:32] in TOP */
  MacAddr = In32(LADDR1H_OFFSET + ((uint32_t) IndexLoc * (uint32_t) 8));
  *(Aptr + 4) = (uint8_t) MacAddr;
  *(Aptr + 5) = (uint8_t) (MacAddr >> 8U);
}

/*****************************************************************************/
/**
 * Set the Type ID match for this driver/device.  The register is a 32-bit
 * value. The device must be stopped before calling this function.
 *
 * @param InstancePtr is a pointer to the instance to be worked on.
 * @param Id_Check is type ID to be configured.
 * @param Index is a index to which Type ID (1-4).
 *
 * @return
 * - XST_SUCCESS if the MAC address was set successfully
 * - XST_DEVICE_IS_STARTED if the device has not yet been stopped
 *
 *****************************************************************************/
long DevEmacPs::SetTypeIdCheck(uint32_t Id_Check, uint8_t Index)
{
  const uint32_t MATCH1_OFFSET = 0x000000A8U;	/**< Type ID1 Match reg */
  uint8_t IndexLoc = Index;
  long Status;
  //Xil_AssertNonvoid(InstancePtr->IsReady == (uint32_t)XIL_COMPONENT_IS_READY);
  //Xil_AssertNonvoid((IndexLoc <= (uint8_t)XEMACPS_MAX_TYPE_ID) && (IndexLoc > 0x00U));

  /* Be sure device has been stopped */
  if (isStarted_ == (uint32_t) XIL_COMPONENT_IS_STARTED)
  {
    Status = (long) (XST_DEVICE_IS_STARTED);
  }
  else
  {
    /* Index ranges 1 to 4, for offset calculation is 0 to 3. */
    IndexLoc--;

    /* Set the ID bits in MATCHx register */
    Out32((MATCH1_OFFSET + ((uint32_t) IndexLoc * (uint32_t) 4)), Id_Check);

    Status = (long) (XST_SUCCESS);
  }
  return Status;
}

/*****************************************************************************/
/**
 * Set options for the driver/device. The driver should be stopped with
 * XEmacPs_Stop() before changing options.
 *
 * @param InstancePtr is a pointer to the instance to be worked on.
 * @param Options are the options to set. Multiple options can be set by OR'ing
 *        XTE_*_OPTIONS constants together. Options not specified are not
 *        affected.
 *
 * @return
 * - XST_SUCCESS if the options were set successfully
 * - XST_DEVICE_IS_STARTED if the device has not yet been stopped
 *
 * @note
 * See xemacps.h for a description of the available options.
 *
 *****************************************************************************/
long DevEmacPs::SetOptions(uint32_t Options)
{
  uint32_t Reg;			/* Generic register contents */
  uint32_t RegNetCfg;		/* Reflects original contents of NET_CONFIG */
  uint32_t RegNewNetCfg;	/* Reflects new contents of NET_CONFIG */
  long Status;
  //Xil_AssertNonvoid(InstancePtr->IsReady == (uint32_t)XIL_COMPONENT_IS_READY);

  printf("SetOptions(%x)\r\n",Options);
  /* Be sure device has been stopped */
  if (isStarted_ == (uint32_t) XIL_COMPONENT_IS_STARTED)
  {
    Status = (LONG) (XST_DEVICE_IS_STARTED);
  }
  else
  {
    const uint32_t NWCFG_OFFSET = 0x00000004U;		   /**< Network Config reg */
    /* Many of these options will change the NET_CONFIG registers.
     * To reduce the amount of IO to the device, group these options here
     * and change them all at once.
     */

    /* Grab current register contents */
    RegNetCfg = In32(NWCFG_OFFSET);
    RegNewNetCfg = RegNetCfg;

    /*
     * It is configured to max 1536.
     */
    if ((Options & XEMACPS_FRAME1536_OPTION) != 0x00000000U)
    {
      const uint32_t NWCFG_1536RXEN_MASK = 0x00000100U;		       /**< Enable 1536 byte
                                                                  frames reception */
      RegNewNetCfg |= NWCFG_1536RXEN_MASK;
    }

    /* Turn on VLAN packet only, only VLAN tagged will be accepted */
    if ((Options & XEMACPS_VLAN_OPTION) != 0x00000000U)
    {
      const uint32_t NWCFG_NVLANDISC_MASK = 0x00000004U;	     /**< Receive only VLAN
                                                        frames */
      RegNewNetCfg |= NWCFG_NVLANDISC_MASK;
    }

    /* Turn on FCS stripping on receive packets */
    if ((Options & XEMACPS_FCS_STRIP_OPTION) != 0x00000000U)
    {
      const uint32_t NWCFG_FCSREM_MASK = 0x00020000U;		 /**< Discard FCS from
                                                           received frames */
      RegNewNetCfg |= NWCFG_FCSREM_MASK;
    }

    /* Turn on length/type field checking on receive packets */
    if ((Options & XEMACPS_LENTYPE_ERR_OPTION) != 0x00000000U)
    {
      const uint32_t NWCFG_LENERRDSCRD_MASK = 0x00010000U;		     /**< RX length error discard */
      RegNewNetCfg |= NWCFG_LENERRDSCRD_MASK;
    }

    /* Turn on flow control */
    if ((Options & XEMACPS_FLOW_CONTROL_OPTION) != 0x00000000U)
    {
      const uint32_t NWCFG_PAUSEEN_MASK = 0x00002000U;		     /**< Enable pause RX */
      RegNewNetCfg |= NWCFG_PAUSEEN_MASK;
    }

    /* Turn on promiscuous frame filtering (all frames are received) */
    if ((Options & XEMACPS_PROMISC_OPTION) != 0x00000000U)
    {
      const uint32_t NWCFG_COPYALLEN_MASK = 0x00000010U;	      /**< Copy all frames */
      RegNewNetCfg |= NWCFG_COPYALLEN_MASK;
    }

    /* Allow broadcast address reception */
    if ((Options & XEMACPS_BROADCAST_OPTION) != 0x00000000U)
    {
      const uint32_t NWCFG_BCASTDI_MASK = 0x00000020U;		       /**< Do not receive  broadcast frames */
      RegNewNetCfg &= (uint32_t) (~NWCFG_BCASTDI_MASK);
    }

    /* Allow multicast address filtering */
    if ((Options & XEMACPS_MULTICAST_OPTION) != 0x00000000U)
    {
      const uint32_t NWCFG_MCASTHASHEN_MASK = 0x00000040U;	      /**< Receive multicast hash
                                                        frames */
      RegNewNetCfg |= NWCFG_MCASTHASHEN_MASK;
    }

    /* enable RX checksum offload */
    if ((Options & XEMACPS_RX_CHKSUM_ENABLE_OPTION) != 0x00000000U)
    {
      const uint32_t NWCFG_RXCHKSUMEN_MASK = 0x01000000U;		  /**< enable RX checksum offload */
      RegNewNetCfg |= NWCFG_RXCHKSUMEN_MASK;
    }

    /* Enable jumbo frames */
    if (((Options & XEMACPS_JUMBO_ENABLE_OPTION) != 0x00000000U) /* && GemVersion() > 2 */ )
    {
      const uint32_t NWCFG_JUMBO_MASK = 0x00000008U;	       /**< Jumbo frames */
      const uint32_t JUMBOMAXLEN_OFFSET = 0x00000048U;	       /**< Jumbo max length reg */
      const uint32_t RX_BUF_SIZE_JUMBO = 10240U;
      const uint32_t RX_BUF_UNIT = 64U;		/**< Number of receive buffer bytes as a
                                                 unit, this is HW setup */
      const uint32_t DMACR_RXBUF_SHIFT = 16U;		/**< Shift bit for RX buffer size */
      RegNewNetCfg |= NWCFG_JUMBO_MASK;
      Out32(JUMBOMAXLEN_OFFSET, RX_BUF_SIZE_JUMBO);

      const uint32_t DMACR_OFFSET = 0x00000010U;	     /**< DMA Control reg */
      const uint32_t DMACR_RXBUF_MASK = 0x00FF0000U;		  /**< Mask bit for RX buffer
													size */
      Reg = In32(DMACR_OFFSET);
      Reg &= ~DMACR_RXBUF_MASK;
      Reg |= ((((RX_BUF_SIZE_JUMBO / RX_BUF_UNIT) +
		((((RX_BUF_SIZE_JUMBO % RX_BUF_UNIT)) != (uint32_t) 0) ? 1U : 0U)) << DMACR_RXBUF_SHIFT) &
	      DMACR_RXBUF_MASK);
      Out32(DMACR_OFFSET, Reg);

      MaxMtuSize = XEMACPS_MTU_JUMBO;
      MaxFrameSize = XEMACPS_MTU_JUMBO + XEMACPS_HDR_SIZE + XEMACPS_TRL_SIZE;
      MaxVlanFrameSize = MaxFrameSize + XEMACPS_HDR_VLAN_SIZE;
      RxBufMask = XEMACPS_RXBUF_LEN_JUMBO_MASK;
    }

    if (((Options & XEMACPS_SGMII_ENABLE_OPTION) != 0x00000000U) /* && GemVersion() > 2 */ )
    {
      const uint32_t NWCFG_SGMIIEN_MASK = 0x08000000U;	       /**< SGMII Enable */
      const uint32_t NWCFG_PCSSEL_MASK = 0x00000800U;	      /**< PCS Select */
      RegNewNetCfg |= (NWCFG_SGMIIEN_MASK | NWCFG_PCSSEL_MASK);
    }

    /* Officially change the NET_CONFIG registers if it needs to be
     * modified.
     */
    if (RegNetCfg != RegNewNetCfg)
    {
      Out32(NWCFG_OFFSET, RegNewNetCfg);
    }

    /* Enable TX checksum offload */
    if ((Options & XEMACPS_TX_CHKSUM_ENABLE_OPTION) != 0x00000000U)
    {
      const uint32_t DMACR_OFFSET = 0x00000010U;		 /**< DMA Control reg */
      const uint32_t DMACR_TCPCKSUM_MASK = 0x00000800U;			/**< enable/disable TX checksum offload */
      Reg = In32(DMACR_OFFSET);
      Reg |= DMACR_TCPCKSUM_MASK;
      Out32(DMACR_OFFSET, Reg);
    }

    const uint32_t NWCTRL_OFFSET = 0x00000000U;		    /**< Network Control reg */

    /* Enable transmitter */
    if ((Options & XEMACPS_TRANSMITTER_ENABLE_OPTION) != 0x00000000U)
    {
      const uint32_t NWCTRL_TXEN_MASK = 0x00000008U;		 /**< Enable transmit */
      Reg = In32(NWCTRL_OFFSET);
      Reg |= NWCTRL_TXEN_MASK;
      Out32(NWCTRL_OFFSET, Reg);
    }

    /* Enable receiver */
    if ((Options & XEMACPS_RECEIVER_ENABLE_OPTION) != 0x00000000U)
    {
      const uint32_t NWCTRL_RXEN_MASK = 0x00000004U;		/**< Enable receive */
      Reg = In32(NWCTRL_OFFSET);
      Reg |= NWCTRL_RXEN_MASK;
      Out32(NWCTRL_OFFSET, Reg);
    }

    /* The remaining options not handled here are managed elsewhere in the
     * driver. No register modifications are needed at this time. Reflecting
     * the option in InstancePtr->Options is good enough for now.
     */

    /* Set options word to its new value */
    options_ |= Options;

    Status = (long) (XST_SUCCESS);
  }
  return Status;
}


/*****************************************************************************/
/**
 * Clear options for the driver/device
 *
 * @param InstancePtr is a pointer to the instance to be worked on.
 * @param Options are the options to clear. Multiple options can be cleared by
 *        OR'ing XEMACPS_*_OPTIONS constants together. Options not specified
 *        are not affected.
 *
 * @return
 * - XST_SUCCESS if the options were set successfully
 * - XST_DEVICE_IS_STARTED if the device has not yet been stopped
 *
 * @note
 * See xemacps.h for a description of the available options.
 *
 *****************************************************************************/
long DevEmacPs::ClearOptions(uint32_t Options)
{
  uint32_t Reg;			/* Generic */
  uint32_t RegNetCfg;		/* Reflects original contents of NET_CONFIG */
  uint32_t RegNewNetCfg;	/* Reflects new contents of NET_CONFIG */
  long Status;

  printf("ClearOptions(%x)\r\n",Options);
  //Xil_AssertNonvoid(isReady == (uint32_t)XIL_COMPONENT_IS_READY);

  /* Be sure device has been stopped */
  if (isStarted_ == (uint32_t) XIL_COMPONENT_IS_STARTED)
  {
    Status = (long) (XST_DEVICE_IS_STARTED);
  }
  else
  {
    /* Many of these options will change the NET_CONFIG registers.
     * To reduce the amount of IO to the device, group these options here
     * and change them all at once.
     */
    const uint32_t NWCFG_OFFSET = 0x00000004U;	   /**< Network Config reg */

    /* Grab current register contents */
    RegNetCfg = In32(NWCFG_OFFSET);
    RegNewNetCfg = RegNetCfg;

    /* There is only RX configuration!?
     * It is configured in two different length, up to 1536 and 10240 bytes
     */
    if ((Options & XEMACPS_FRAME1536_OPTION) != 0x00000000U)
    {
      const uint32_t NWCFG_1536RXEN_MASK = 0x00000100U;		      /**< Enable 1536 byte
                                                                  frames reception */
      RegNewNetCfg &= ~NWCFG_1536RXEN_MASK;
    }

    /* Turn off VLAN packet only */
    if ((Options & XEMACPS_VLAN_OPTION) != 0x00000000U)
    {
      const uint32_t NWCFG_NVLANDISC_MASK = 0x00000004U;	     /**< Receive only VLAN frames */
      RegNewNetCfg &= ~NWCFG_NVLANDISC_MASK;
    }

    /* Turn off FCS stripping on receive packets */
    if ((Options & XEMACPS_FCS_STRIP_OPTION) != 0x00000000U)
    {
      const uint32_t NWCFG_FCSREM_MASK = 0x00020000U;		/**< Discard FCS from
		                                                             received frames */
      RegNewNetCfg &= ~NWCFG_FCSREM_MASK;
    }

    /* Turn off length/type field checking on receive packets */
    if ((Options & XEMACPS_LENTYPE_ERR_OPTION) != 0x00000000U)
    {
      const uint32_t NWCFG_LENERRDSCRD_MASK = 0x00010000U;	       /**< RX length error discard */
      RegNewNetCfg &= ~NWCFG_LENERRDSCRD_MASK;
    }

    /* Turn off flow control */
    if ((Options & XEMACPS_FLOW_CONTROL_OPTION) != 0x00000000U)
    {
      const uint32_t NWCFG_PAUSEEN_MASK = 0x00002000U;		   /**< Enable pause RX */
      RegNewNetCfg &= ~NWCFG_PAUSEEN_MASK;
    }

    /* Turn off promiscuous frame filtering (all frames are received) */
    if ((Options & XEMACPS_PROMISC_OPTION) != 0x00000000U)
    {
      const uint32_t NWCFG_COPYALLEN_MASK = 0x00000010U;	   /**< Copy all frames */
      RegNewNetCfg &= ~NWCFG_COPYALLEN_MASK;
    }

    /* Disallow broadcast address filtering => broadcast reception */
    if ((Options & XEMACPS_BROADCAST_OPTION) != 0x00000000U)
    {
      const uint32_t NWCFG_BCASTDI_MASK = 0x00000020U;		 /**< Do not receive  broadcast frames */
      RegNewNetCfg |= NWCFG_BCASTDI_MASK;
    }

    /* Disallow multicast address filtering */
    if ((Options & XEMACPS_MULTICAST_OPTION) != 0x00000000U)
    {
      const uint32_t NWCFG_MCASTHASHEN_MASK = 0x00000040U;	     /**< Receive multicast hash
		                                                          frames */
      RegNewNetCfg &= ~NWCFG_MCASTHASHEN_MASK;
    }

    /* Disable RX checksum offload */
    if ((Options & XEMACPS_RX_CHKSUM_ENABLE_OPTION) != 0x00000000U)
    {
      const uint32_t NWCFG_RXCHKSUMEN_MASK = 0x01000000U;	    /**< enable RX checksum offload */
      RegNewNetCfg &= ~NWCFG_RXCHKSUMEN_MASK;
    }

    /* Disable jumbo frames */
    if (((Options & XEMACPS_JUMBO_ENABLE_OPTION) != 0x00000000U) /* && GemVersion() > 2 */ )
    {
      //const uint32_t DMACR_OFFSET=0x00000010U; /**< DMA Control reg */
      const uint32_t NWCFG_JUMBO_MASK = 0x00000008U;		 /**< Jumbo frames */
      const uint32_t RX_BUF_SIZE = 1536U;
      const uint32_t RX_BUF_UNIT = 64U;		  /**< Number of receive buffer bytes as a
		                                                   unit, this is HW setup */
      const uint32_t DMACR_RXBUF_MASK = 0x00FF0000U;		/**< Mask bit for RX buffer
		  													size */
      const uint32_t DMACR_RXBUF_SHIFT = 16U;		/**< Shift bit for RX buffer size */
      const uint32_t DMACR_OFFSET = 0x00000010U;	   /**< DMA Control reg */
      RegNewNetCfg &= ~NWCFG_JUMBO_MASK;
      Reg = In32(DMACR_OFFSET);
      Reg &= ~DMACR_RXBUF_MASK;
      Reg |= (((RX_BUF_SIZE / RX_BUF_UNIT) +
	       ((((RX_BUF_SIZE % RX_BUF_UNIT) != (uint32_t) 0) ? 1U : 0U)) <<
	       DMACR_RXBUF_SHIFT) & DMACR_RXBUF_MASK);
      Out32(DMACR_OFFSET, Reg);
      MaxMtuSize = XEMACPS_MTU;
      MaxFrameSize = XEMACPS_MTU + XEMACPS_HDR_SIZE + XEMACPS_TRL_SIZE;
      MaxVlanFrameSize = MaxFrameSize + XEMACPS_HDR_VLAN_SIZE;
      RxBufMask = XEMACPS_RXBUF_LEN_MASK;
    }

    if (((Options & XEMACPS_SGMII_ENABLE_OPTION) != 0x00000000U) /* && GemVersion() > 2 */ )
    {
      const uint32_t NWCFG_SGMIIEN_MASK = 0x08000000U;		/**< SGMII Enable */
      const uint32_t NWCFG_PCSSEL_MASK = 0x00000800U;	       /**< PCS Select */
      RegNewNetCfg &= ~(NWCFG_SGMIIEN_MASK | NWCFG_PCSSEL_MASK);
    }

    /* Officially change the NET_CONFIG registers if it needs to be
     * modified.
     */
    if (RegNetCfg != RegNewNetCfg)
    {
      Out32(NWCFG_OFFSET, RegNewNetCfg);
    }

    /* Disable TX checksum offload */
    if ((Options & XEMACPS_TX_CHKSUM_ENABLE_OPTION) != 0x00000000U)
    {
      const uint32_t DMACR_OFFSET = 0x00000010U;	   /**< DMA Control reg */
      const uint32_t DMACR_TCPCKSUM_MASK = 0x00000800U;		  /**< enable/disable TX checksum offload */
      Reg = In32(DMACR_OFFSET);
      Reg &= ~DMACR_TCPCKSUM_MASK;
      Out32(DMACR_OFFSET, Reg);
    }

    const uint32_t NWCTRL_OFFSET = 0x00000000U;	      /**< Network Control reg */
    /* Disable transmitter */
    if ((Options & XEMACPS_TRANSMITTER_ENABLE_OPTION) != 0x00000000U)
    {
      const uint32_t NWCTRL_TXEN_MASK = 0x00000008U;		/**< Enable transmit */
      Reg = In32(NWCTRL_OFFSET);
      Reg &= ~NWCTRL_TXEN_MASK;
      Out32(NWCTRL_OFFSET, Reg);
    }

    /* Disable receiver */
    if ((Options & XEMACPS_RECEIVER_ENABLE_OPTION) != 0x00000000U)
    {
      const uint32_t NWCTRL_RXEN_MASK = 0x00000004U;	     /**< Enable receive */
      Reg = In32(NWCTRL_OFFSET);
      Reg &= ~NWCTRL_RXEN_MASK;
      Out32(NWCTRL_OFFSET, Reg);
    }

    /* The remaining options not handled here are managed elsewhere in the
     * driver. No register modifications are needed at this time. Reflecting
     * option in InstancePtr->Options is good enough for now.
     */

    /* Set options word to its new value */
    options_ &= ~Options;
    Status = (long) (XST_SUCCESS);
  }
  return Status;
}

/*****************************************************************************/
/**
 * Set the MDIO clock divisor.
 *
 * Calculating the divisor:
 *
 * <pre>
 *              f[HOSTCLK]
 *   f[MDC] = -----------------
 *            (1 + Divisor) * 2
 * </pre>
 *
 * where f[HOSTCLK] is the bus clock frequency in MHz, and f[MDC] is the
 * MDIO clock frequency in MHz to the PHY. Typically, f[MDC] should not
 * exceed 2.5 MHz. Some PHYs can tolerate faster speeds which means faster
 * access. Here is the table to show values to generate MDC,
 *
 * <pre>
 * 000 : divide pclk by   8 (pclk up to  20 MHz)
 * 001 : divide pclk by  16 (pclk up to  40 MHz)
 * 010 : divide pclk by  32 (pclk up to  80 MHz)
 * 011 : divide pclk by  48 (pclk up to 120 MHz)
 * 100 : divide pclk by  64 (pclk up to 160 MHz)
 * 101 : divide pclk by  96 (pclk up to 240 MHz)
 * 110 : divide pclk by 128 (pclk up to 320 MHz)
 * 111 : divide pclk by 224 (pclk up to 540 MHz)
 * </pre>
 *
 * @param InstancePtr is a pointer to the instance to be worked on.
 * @param Divisor is the divisor to set. Range is 0b000 to 0b111.
 *
 *****************************************************************************/
void DevEmacPs::SetMdioDivisor(XEmacPs_MdcDiv Divisor)
{
  uint32_t Reg;
  //Xil_AssertVoid(InstancePtr->IsReady == (uint32_t)XIL_COMPONENT_IS_READY);
  //Xil_AssertVoid(Divisor <= (XEmacPs_MdcDiv)0x7); /* only last three bits are valid */

  Reg = In32(XEMACPS_NWCFG_OFFSET);
  /* clear these three bits, could be done with mask */
  Reg &= (uint32_t) (~XEMACPS_NWCFG_MDCCLKDIV_MASK);

  Reg |= ((uint32_t) Divisor << XEMACPS_NWCFG_MDC_SHIFT_MASK);

  Out32(XEMACPS_NWCFG_OFFSET, Reg);
}

/*****************************************************************************/
/**
 * XEmacPs_SetOperatingSpeed sets the current operating link speed. For any
 * traffic to be passed, this speed must match the current MII/GMII/SGMII/RGMII
 * link speed.
 *
 * @param InstancePtr references the TEMAC channel on which to operate.
 * @param Speed is the speed to set in units of Mbps. Valid values are 10, 100,
 *        or 1000. XEmacPs_SetOperatingSpeed ignores invalid values.
 *
 * @note
 *
 *****************************************************************************/
void DevEmacPs::SetOperatingSpeed(uint16_t Speed)
{
  uint32_t Reg;
  //Xil_AssertVoid(InstancePtr->IsReady == (uint32_t)XIL_COMPONENT_IS_READY);
  //Xil_AssertVoid((Speed == (uint16_t)10) || (Speed == (uint16_t)100) || (Speed == (uint16_t)1000));

  Reg = In32(XEMACPS_NWCFG_OFFSET);
  Reg &= (uint32_t) (~(XEMACPS_NWCFG_1000_MASK | XEMACPS_NWCFG_100_MASK));

  switch (Speed)
  {
  case (uint16_t) 10:
    break;

  case (uint16_t) 100:
    Reg |= XEMACPS_NWCFG_100_MASK;
    break;

  case (uint16_t) 1000:
    Reg |= XEMACPS_NWCFG_1000_MASK;
    break;
  }
  /* Set register and return */
  Out32(XEMACPS_NWCFG_OFFSET, Reg);
}

/*****************************************************************************/
/**
* Read the current value of the PHY register indicated by the PhyAddress and
* the RegisterNum parameters. The MAC provides the driver with the ability to
* talk to a PHY that adheres to the Media Independent Interface (MII) as
* defined in the IEEE 802.3 standard.
*
* Prior to PHY access with this function, the user should have setup the MDIO
* clock with XEmacPs_SetMdioDivisor().
*
* @param InstancePtr is a pointer to the XEmacPs instance to be worked on.
* @param PhyAddress is the address of the PHY to be read (supports multiple
*        PHYs)
* @param RegisterNum is the register number, 0-31, of the specific PHY register
*        to read
* @param PhyDataPtr is an output parameter, and points to a 16-bit buffer into
*        which the current value of the register will be copied.
*
* @return
*
* - XST_SUCCESS if the PHY was read from successfully
* - XST_EMAC_MII_BUSY if there is another PHY operation in progress
*
* @note
*
* This function is not thread-safe. The user must provide mutually exclusive
* access to this function if there are to be multiple threads that can call it.
*
* There is the possibility that this function will not return if the hardware
* is broken (i.e., it never sets the status bit indicating that the read is
* done). If this is of concern to the user, the user should provide a mechanism
* suitable to their needs for recovery.
*
* For the duration of this function, all host interface reads and writes are
* blocked to the current XEmacPs instance.
*
******************************************************************************/
long DevEmacPs::PhyRead(uint32_t PhyAddress, uint32_t RegisterNum, uint16_t * PhyDataPtr)
{
  uint32_t Mgtcr;
  volatile uint32_t Ipisr;
  uint32_t IpReadTemp;
  long Status;

#ifdef DEBUG_ETHERNET
  printf("PhyRead(PhyAddress=%x,RegisterNum=%x)\r\n", PhyAddress,RegisterNum);
#endif
  /* Make sure no other PHY operation is currently in progress */
  if ((!(In32(XEMACPS_NWSR_OFFSET) & XEMACPS_NWSR_MDIOIDLE_MASK)) == TRUE)
  {
    Status = (long) (XST_EMAC_MII_BUSY);
  }
  else
  {
    /* Construct Mgtcr mask for the operation */
    Mgtcr = XEMACPS_PHYMNTNC_OP_MASK | XEMACPS_PHYMNTNC_OP_R_MASK |
      (PhyAddress << XEMACPS_PHYMNTNC_PHAD_SHFT_MSK) |
      (RegisterNum << XEMACPS_PHYMNTNC_PREG_SHFT_MSK);

    /* Write Mgtcr and wait for completion */
    Out32(XEMACPS_PHYMNTNC_OFFSET, Mgtcr);

    do
    {
      Ipisr = In32(XEMACPS_NWSR_OFFSET);
      IpReadTemp = Ipisr;
    } while ((IpReadTemp & XEMACPS_NWSR_MDIOIDLE_MASK) == 0x00000000U);

    /* Read data */
    *PhyDataPtr = (uint16_t) In32(XEMACPS_PHYMNTNC_OFFSET);
    Status = (long) (XST_SUCCESS);
  }
#ifdef DEBUG_ETHERNET
  printf("end PhyRead(PhyAddress=%x,RegisterNum=%x) PhyDataPtr=%x Status=%ld\r\n", PhyAddress,RegisterNum,(uint32_t)*PhyDataPtr,Status);
#endif
  return Status;
}


/*****************************************************************************/
/**
* Write data to the specified PHY register. The Ethernet driver does not
* require the device to be stopped before writing to the PHY.  Although it is
* probably a good idea to stop the device, it is the responsibility of the
* application to deem this necessary. The MAC provides the driver with the
* ability to talk to a PHY that adheres to the Media Independent Interface
* (MII) as defined in the IEEE 802.3 standard.
*
* Prior to PHY access with this function, the user should have setup the MDIO
* clock with XEmacPs_SetMdioDivisor().
*
* @param InstancePtr is a pointer to the XEmacPs instance to be worked on.
* @param PhyAddress is the address of the PHY to be written (supports multiple
*        PHYs)
* @param RegisterNum is the register number, 0-31, of the specific PHY register
*        to write
* @param PhyData is the 16-bit value that will be written to the register
*
* @return
*
* - XST_SUCCESS if the PHY was written to successfully. Since there is no error
*   status from the MAC on a write, the user should read the PHY to verify the
*   write was successful.
* - XST_EMAC_MII_BUSY if there is another PHY operation in progress
*
* @note
*
* This function is not thread-safe. The user must provide mutually exclusive
* access to this function if there are to be multiple threads that can call it.
*
* There is the possibility that this function will not return if the hardware
* is broken (i.e., it never sets the status bit indicating that the write is
* done). If this is of concern to the user, the user should provide a mechanism
* suitable to their needs for recovery.
*
* For the duration of this function, all host interface reads and writes are
* blocked to the current XEmacPs instance.
*
******************************************************************************/
long DevEmacPs::PhyWrite(uint32_t PhyAddress, uint32_t RegisterNum, uint16_t PhyData)
{
  uint32_t Mgtcr;
  volatile uint32_t Ipisr;
  uint32_t IpWriteTemp;
  long Status;

#ifdef DEBUG_ETHERNET
  printf("PhyWrite(PhyAddress=%x,RegisterNum=%x,PhyData=%x\r\n",PhyAddress,RegisterNum,(uint32_t) PhyData);
#endif
  /* Make sure no other PHY operation is currently in progress */
  if ((!(In32(XEMACPS_NWSR_OFFSET) & XEMACPS_NWSR_MDIOIDLE_MASK)) == TRUE)
  {
    Status = (long) (XST_EMAC_MII_BUSY);
  }
  else
  {
    /* Construct Mgtcr mask for the operation */
    Mgtcr = XEMACPS_PHYMNTNC_OP_MASK | XEMACPS_PHYMNTNC_OP_W_MASK |
      (PhyAddress << XEMACPS_PHYMNTNC_PHAD_SHFT_MSK) |
      (RegisterNum << XEMACPS_PHYMNTNC_PREG_SHFT_MSK) | (uint32_t) PhyData;

    /* Write Mgtcr and wait for completion */
    Out32(XEMACPS_PHYMNTNC_OFFSET, Mgtcr);

    do
    {
      Ipisr = In32(XEMACPS_NWSR_OFFSET);
      IpWriteTemp = Ipisr;
    } while ((IpWriteTemp & XEMACPS_NWSR_MDIOIDLE_MASK) == 0x00000000U);

    Status = (long) (XST_SUCCESS);
  }
#ifdef DEBUG_ETHERNET
  printf("end PhyWrite(PhyAddress=%x,RegisterNum=%x,PhyData=%x) Status=%ld\r\n",PhyAddress,RegisterNum,(uint32_t)PhyData,Status);
#endif
  return Status;
}

/****************************************************************************/
/**
*
* This function detects the PHY address by looking for successful MII status
* register contents.
*
* @param    The XEMACPS driver instance
*
* @return   The address of the PHY (defaults to 32 if none detected)
*
* @note     None.
*
*****************************************************************************/
#define PHY_DETECT_REG1 2
#define PHY_DETECT_REG2 3

#define PHY_ID_MARVELL	0x141
#define PHY_ID_TI		0x2000

uint32_t DevEmacPs::DetectPHY()
{
  uint32_t PhyAddr;
  uint32_t Status;
  uint16_t PhyReg1;
  uint16_t PhyReg2;

  for (PhyAddr = 0; PhyAddr <= 31; PhyAddr++)
  {
    Status = PhyRead(PhyAddr, PHY_DETECT_REG1, &PhyReg1);

    Status |= PhyRead(PhyAddr, PHY_DETECT_REG2, &PhyReg2);

    if ((Status == XST_SUCCESS) &&
	(PhyReg1 > 0x0000) && (PhyReg1 < 0xffff) &&
	(PhyReg2 > 0x0000) && (PhyReg2 < 0xffff))
    {
      /* Found a valid PHY address */
      return PhyAddr;
    }
  }

  return PhyAddr;		/* default to 32(max of iteration) */
}

long DevEmacPs::EnterLoopback(uint32_t Speed)
{
  long Status=0;
  uint16_t PhyIdentity;
  uint32_t PhyAddr;
  
  printf("EnterLoopback(Speed=%x)\r\n",Speed);
  /*
   * Detect the PHY address
   */
  PhyAddr = DetectPHY();

  if (PhyAddr >= 32)
  {
    printf("%s\r\n", "Error detect phy");
    return XST_FAILURE;
  }

  PhyRead(PhyAddr, PHY_DETECT_REG1, &PhyIdentity);

  if (PhyIdentity == PHY_ID_MARVELL)
  {
    Status = MarvellPhyLoopback(Speed, PhyAddr);
  }
  else if (PhyIdentity == PHY_ID_TI)
  {
    Status = TiPhyLoopback(Speed, PhyAddr);
  }

  if (Status != XST_SUCCESS)
  {
    printf("%s\r\n", "Error setup phy loopback");
    return XST_FAILURE;
  }

  printf("-- End EnterLoopback\r\n");
  return XST_SUCCESS;
}

/****************************************************************************/
/**
*
* This function sets the PHY to loopback mode.
*
* @param    The XEMACPS driver instance
* @param    Speed is the loopback speed 10/100 Mbit.
*
* @return   XST_SUCCESS if successful, else XST_FAILURE.
*
* @note     None.
*
*****************************************************************************/
#define PHY_REG0_RESET    0x8000
#define PHY_REG0_LOOPBACK 0x4000
#define PHY_REG0_10       0x0100
#define PHY_REG0_100      0x2100
#define PHY_REG0_1000     0x0140
#define PHY_REG21_10      0x0030
#define PHY_REG21_100     0x2030
#define PHY_REG21_1000    0x0070
#define PHY_LOOPCR		0xFE
#define PHY_REGCR		0x0D
#define PHY_ADDAR		0x0E
#define PHY_RGMIIDCTL	0x86
#define PHY_RGMIICTL	0x32

#define PHY_REGCR_ADDR	0x001F
#define PHY_REGCR_DATA	0x401F

/* RGMII RX and TX tuning values */
#define PHY_TI_RGMII_ZCU102	0xA8
#define PHY_TI_RGMII_VERSALEMU	0xAB

long DevEmacPs::MarvellPhyLoopback(uint32_t Speed, uint32_t PhyAddr)
{
  long Status;
  uint16_t PhyReg0 = 0;
  uint16_t PhyReg21 = 0;
  uint16_t PhyReg22 = 0;

  printf("MarvellPhyLoopback(Speed=%x,PhyAddr=%x\r\n",Speed,PhyAddr);   
  /*
   * Setup speed and duplex
   */
  switch (Speed)
  {
  case 10:
    PhyReg0 |= PHY_REG0_10;
    PhyReg21 |= PHY_REG21_10;
    break;
  case 100:
    PhyReg0 |= PHY_REG0_100;
    PhyReg21 |= PHY_REG21_100;
    break;
  case 1000:
    PhyReg0 |= PHY_REG0_1000;
    PhyReg21 |= PHY_REG21_1000;
    break;
  default:
    printf("%s\r\n", "Error: speed not recognized ");
    return XST_FAILURE;
  }

  Status = PhyWrite(PhyAddr, 0, PhyReg0);
  /*
   * Make sure new configuration is in effect
   */
  Status = PhyRead(PhyAddr, 0, &PhyReg0);
  if (Status != XST_SUCCESS)
  {
    printf("%s\r\n", "Error setup phy speed");
    return XST_FAILURE;
  }

  /*
   * Switching to PAGE2
   */
  PhyReg22 = 0x2;
  Status = PhyWrite(PhyAddr, 22, PhyReg22);

  /*
   * Adding Tx and Rx delay. Configuring loopback speed.
   */
  Status = PhyWrite(PhyAddr, 21, PhyReg21);
  /*
   * Make sure new configuration is in effect
   */
  Status = PhyRead(PhyAddr, 21, &PhyReg21);
  if (Status != XST_SUCCESS)
  {
    printf("%s\r\n", "Error setting Reg 21 in Page 2");
    return XST_FAILURE;
  }
  /*
   * Switching to PAGE0
   */
  PhyReg22 = 0x0;
  Status = PhyWrite(PhyAddr, 22, PhyReg22);

  /*
   * Issue a reset to phy
   */
  Status = PhyRead(PhyAddr, 0, &PhyReg0);
  PhyReg0 |= PHY_REG0_RESET;
  Status = PhyWrite(PhyAddr, 0, PhyReg0);

  Status = PhyRead(PhyAddr, 0, &PhyReg0);
  if (Status != XST_SUCCESS)
  {
    printf("%s\r\n", "Error reset phy");
    return XST_FAILURE;
  }

  /*
   * Enable loopback
   */
  PhyReg0 |= PHY_REG0_LOOPBACK;
  Status = PhyWrite(PhyAddr, 0, PhyReg0);

  Status = PhyRead(PhyAddr, 0, &PhyReg0);
  if (Status != XST_SUCCESS)
  {
    printf("%s\r\n", "Error setup phy loopback");
    return XST_FAILURE;
  }

  sleep(1);

  printf("end MarvellPhyLoopback(Speed=%x,PhyAddr=%x\r\n",Speed,PhyAddr);   
  return XST_SUCCESS;
}

long DevEmacPs::TiPhyLoopback(uint32_t Speed, uint32_t PhyAddr)
{
  long Status;
  uint16_t PhyReg0 = 0, LoopbackSpeed = 0;
  uint16_t RgmiiTuning = PHY_TI_RGMII_ZCU102;

  printf("TiPhyLoopback(Speed=%x,PhyAddr=%x)\r\n",Speed,PhyAddr);
  /*
   * Setup speed and duplex
   */
  switch (Speed)
  {
  case 10:
    PhyReg0 |= PHY_REG0_10;
    break;
  case 100:
    PhyReg0 |= PHY_REG0_100;
    break;
  case 1000:
    PhyReg0 |= PHY_REG0_1000;
    break;
  default:
    printf("%s\r\n", "Error: speed not recognized ");
    return XST_FAILURE;
  }
  LoopbackSpeed = PhyReg0;

  Status = PhyWrite(PhyAddr, 0, PhyReg0);
  /*
   * Make sure new configuration is in effect
   */
  Status = PhyRead(PhyAddr, 0, &PhyReg0);
  if (Status != XST_SUCCESS)
  {
    printf("%s\r\n", "Error setup phy speed");
    return XST_FAILURE;
  }

  Status = PhyRead(PhyAddr, 1, &PhyReg0);
  if (Status != XST_SUCCESS)
  {
    printf("%s\r\n", "Error setup phy speed");
    return XST_FAILURE;
  }

  /* Write PHY_LOOPCR */
  Status = PhyWrite(PhyAddr, PHY_REGCR, PHY_REGCR_ADDR);
  Status = PhyWrite(PhyAddr, PHY_ADDAR, PHY_LOOPCR);
  Status = PhyWrite(PhyAddr, PHY_REGCR, PHY_REGCR_DATA);
  Status = PhyWrite(PhyAddr, PHY_ADDAR, 0xEF20);
  if (Status != XST_SUCCESS)
  {
    printf("%s\r\n", "Error setup phy speed");
    return XST_FAILURE;
  }

  /* Read PHY_LOOPCR */
  Status = PhyWrite(PhyAddr, PHY_REGCR, PHY_REGCR_ADDR);
  Status = PhyWrite(PhyAddr, PHY_ADDAR, PHY_LOOPCR);
  Status = PhyWrite(PhyAddr, PHY_REGCR, PHY_REGCR_DATA);
  Status = PhyRead(PhyAddr, PHY_ADDAR, &PhyReg0);
  if (Status != XST_SUCCESS)
  {
    printf("%s\r\n", "Error setup phy speed");
    return XST_FAILURE;
  }

  /* SW reset */
  Status = PhyWrite(PhyAddr, 0x1F, 0x4000);
  Status = PhyRead(PhyAddr, 0, &PhyReg0);
  if (Status != XST_SUCCESS)
  {
    printf("%s\r\n", "Error setup phy speed");
    return XST_FAILURE;
  }

  /* issue a reset to phy */
  Status = PhyRead(PhyAddr, 0, &PhyReg0);
  PhyReg0 |= PHY_REG0_RESET;
  Status = PhyWrite(PhyAddr, 0, PhyReg0);

  Status = PhyRead(PhyAddr, 0, &PhyReg0);
  if (Status != XST_SUCCESS)
  {
    printf("%s\r\n", "Error reset phy");
    return XST_FAILURE;
  }

  sleep(1);

  /* enable loopback */
  PhyReg0 = LoopbackSpeed | PHY_REG0_LOOPBACK;
  Status = PhyWrite(PhyAddr, 0, PhyReg0);
  Status = PhyRead(PhyAddr, 0, &PhyReg0);
  if (Status != XST_SUCCESS)
  {
    printf("%s\r\n", "Error setup phy loopback");
    return XST_FAILURE;
  }

  Status = PhyWrite(PhyAddr, 0x10, 0x5048);
  Status = PhyRead(PhyAddr, 0x10, &PhyReg0);
  if (Status != XST_SUCCESS)
  {
    printf("%s\r\n", "Error setup phy loopback");
    return XST_FAILURE;
  }

  /* Write to PHY_RGMIIDCTL */
  PhyWrite(PhyAddr, PHY_REGCR, PHY_REGCR_ADDR);
  PhyWrite(PhyAddr, PHY_ADDAR, PHY_RGMIIDCTL);
  PhyWrite(PhyAddr, PHY_REGCR, PHY_REGCR_DATA);
  //if ((Platform & PLATFORM_MASK_VERSAL) == PLATFORM_VERSALEMU)
  //{
  //      RgmiiTuning = PHY_TI_RGMII_VERSALEMU;
  //}
  Status = PhyWrite(PhyAddr, PHY_ADDAR, RgmiiTuning);
  if (Status != XST_SUCCESS)
  {
    printf("%s\r\n", "Error in tuning");
    return XST_FAILURE;
  }

  /* Read PHY_RGMIIDCTL */
  PhyWrite(PhyAddr, PHY_REGCR, PHY_REGCR_ADDR);
  PhyWrite(PhyAddr, PHY_ADDAR, PHY_RGMIIDCTL);
  PhyWrite(PhyAddr, PHY_REGCR, PHY_REGCR_DATA);
  Status = PhyRead(PhyAddr, PHY_ADDAR, &PhyReg0);
  if (Status != XST_SUCCESS)
  {
    printf("%s\r\n", "Error in tuning");
    return XST_FAILURE;
  }

  /* Write PHY_RGMIICTL */
  PhyWrite(PhyAddr, PHY_REGCR, PHY_REGCR_ADDR);
  PhyWrite(PhyAddr, PHY_ADDAR, PHY_RGMIICTL);
  PhyWrite(PhyAddr, PHY_REGCR, PHY_REGCR_DATA);
  Status = PhyWrite(PhyAddr, PHY_ADDAR, 0xD3);
  if (Status != XST_SUCCESS)
  {
    printf("%s\r\n", "Error in tuning");
    return XST_FAILURE;
  }

  /* Read PHY_RGMIICTL */
  PhyWrite(PhyAddr, PHY_REGCR, PHY_REGCR_ADDR);
  PhyWrite(PhyAddr, PHY_ADDAR, PHY_RGMIICTL);
  PhyWrite(PhyAddr, PHY_REGCR, PHY_REGCR_DATA);
  Status = PhyRead(PhyAddr, PHY_ADDAR, &PhyReg0);
  if (Status != XST_SUCCESS)
  {
    printf("%s\r\n", "Error in tuning");
    return XST_FAILURE;
  }

  Status = PhyRead(PhyAddr, 0x11, &PhyReg0);
  if (Status != XST_SUCCESS)
  {
    printf("%s\r\n", "Error setup phy loopback");
    return XST_FAILURE;
  }

  printf("end TiPhyLoopback(Speed=%x,PhyAddr=%x)\r\n",Speed,PhyAddr);
  return XST_SUCCESS;
}

/**
*
*  triggers transmit circuit to send data currently in TX buffer(s).
*
*****************************************************************************/
void DevEmacPs::Transmit()
{
  uint32_t reg = In32(XEMACPS_NWCTRL_OFFSET);
  Out32(XEMACPS_NWCTRL_OFFSET, reg | XEMACPS_NWCTRL_STARTTX_MASK);
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

uint32_t DevEmacPs::get_IEEE_phy_speed(uint32_t phy_addr)
{
    uint16_t phy_identity;
	uint32_t RetStatus;

	PhyRead(phy_addr, PHY_IDENTIFIER_1_REG,	&phy_identity);
	if (phy_identity == PHY_TI_IDENTIFIER)
    {
		RetStatus = get_TI_phy_speed(phy_addr);
	}
	else if (phy_identity == PHY_REALTEK_IDENTIFIER)
    {
		RetStatus = get_Realtek_phy_speed(phy_addr);
	}
	else if (phy_identity == PHY_XILINX_PCS_PMA_ID1)
    {
		RetStatus = get_Xilinx_pcs_pma_phy_speed(phy_addr);
	}
	else if (phy_identity == PHY_ADI_IDENTIFIER)
    {
		RetStatus = get_Adi_phy_speed(phy_addr);
	}
	else
    {
		RetStatus = get_Marvell_phy_speed(phy_addr);
	}

	return RetStatus;
}

#define PHY_REGCR		0x0D
#define PHY_ADDAR		0x0E
#define PHY_RGMIIDCTL	0x86
#define PHY_RGMIICTL	0x32
#define PHY_STS			0x11
#define PHY_TI_CR		0x10
#define PHY_TI_CFG4		0x31

#define PHY_REGCR_ADDR	0x001F
#define PHY_REGCR_DATA	0x401F
#define PHY_TI_CRVAL	0x5048
#define PHY_TI_CFG4RESVDBIT7	0x80

uint32_t DevEmacPs::get_TI_phy_speed(uint32_t phy_addr)
{
	uint16_t control;
	uint16_t status;
	uint16_t status_speed;
	uint32_t timeout_counter = 0;
	uint32_t phyregtemp;
	uint32_t RetStatus;

	printf("Start TI PHY autonegotiation \r\n");

	PhyRead(phy_addr, 0x1F, (uint16_t *)&phyregtemp);
	phyregtemp |= 0x4000;
	PhyWrite(phy_addr, 0x1F, phyregtemp);
	RetStatus = PhyRead(phy_addr, 0x1F, (uint16_t *)&phyregtemp);
	if (RetStatus != XST_SUCCESS)
    {
		printf("Error during sw reset \n\r");
		return XST_FAILURE;
	}

	PhyRead(phy_addr, 0, (uint16_t *)&phyregtemp);
	phyregtemp |= 0x8000;
	PhyWrite(phy_addr, 0, phyregtemp);

	/*
	 * Delay
	 */
	sleep(1);

	RetStatus = PhyRead(phy_addr, 0, (uint16_t *)&phyregtemp);
	if (RetStatus != XST_SUCCESS)
    {
		printf("Error during reset \n\r");
		return XST_FAILURE;
	}

	/* FIFO depth */
	PhyWrite(phy_addr, PHY_TI_CR, PHY_TI_CRVAL);
	RetStatus = PhyRead(phy_addr, PHY_TI_CR, (uint16_t *)&phyregtemp);
	if (RetStatus != XST_SUCCESS)
    {
		printf("Error writing to 0x10 \n\r");
		return XST_FAILURE;
	}

	/* TX/RX tuning */
	/* Write to PHY_RGMIIDCTL */
	PhyWrite(phy_addr, PHY_REGCR, PHY_REGCR_ADDR);
	PhyWrite(phy_addr, PHY_ADDAR, PHY_RGMIIDCTL);
	PhyWrite(phy_addr, PHY_REGCR, PHY_REGCR_DATA);
	RetStatus = PhyWrite(phy_addr, PHY_ADDAR, 0xA8);
	if (RetStatus != XST_SUCCESS)
    {
		printf("Error in tuning\r\n");
		return XST_FAILURE;
	}

	/* Read PHY_RGMIIDCTL */
	PhyWrite(phy_addr, PHY_REGCR, PHY_REGCR_ADDR);
	PhyWrite(phy_addr, PHY_ADDAR, PHY_RGMIIDCTL);
	PhyWrite(phy_addr, PHY_REGCR, PHY_REGCR_DATA);
	RetStatus = PhyRead(phy_addr, PHY_ADDAR, (uint16_t *)&phyregtemp);
	if (RetStatus != XST_SUCCESS)
    {
		printf("Error in tuning\r\n");
		return XST_FAILURE;
	}

	/* Write PHY_RGMIICTL */
	PhyWrite(phy_addr, PHY_REGCR, PHY_REGCR_ADDR);
	PhyWrite(phy_addr, PHY_ADDAR, PHY_RGMIICTL);
	PhyWrite(phy_addr, PHY_REGCR, PHY_REGCR_DATA);
	RetStatus = PhyWrite(phy_addr, PHY_ADDAR, 0xD3);
	if (RetStatus != XST_SUCCESS)
    {
		printf("Error in tuning\r\n");
		return XST_FAILURE;
	}

	/* Read PHY_RGMIICTL */
	PhyWrite(phy_addr, PHY_REGCR, PHY_REGCR_ADDR);
	PhyWrite(phy_addr, PHY_ADDAR, PHY_RGMIICTL);
	PhyWrite(phy_addr, PHY_REGCR, PHY_REGCR_DATA);
	RetStatus = PhyRead(phy_addr, PHY_ADDAR, (uint16_t *)&phyregtemp);
	if (RetStatus != XST_SUCCESS)
    {
		printf("Error in tuning\r\n");
		return XST_FAILURE;
	}

	/* SW workaround for unstable link when RX_CTRL is not STRAP MODE 3 or 4 */
	PhyWrite(phy_addr, PHY_REGCR, PHY_REGCR_ADDR);
	PhyWrite(phy_addr, PHY_ADDAR, PHY_TI_CFG4);
	PhyWrite(phy_addr, PHY_REGCR, PHY_REGCR_DATA);
	RetStatus = PhyRead(phy_addr, PHY_ADDAR, (uint16_t *)&phyregtemp);
	phyregtemp &= ~(PHY_TI_CFG4RESVDBIT7);
	PhyWrite(phy_addr, PHY_REGCR, PHY_REGCR_ADDR);
	PhyWrite(phy_addr, PHY_ADDAR, PHY_TI_CFG4);
	PhyWrite(phy_addr, PHY_REGCR, PHY_REGCR_DATA);
	RetStatus = PhyWrite(phy_addr, PHY_ADDAR, phyregtemp);

	PhyRead(phy_addr, IEEE_AUTONEGO_ADVERTISE_REG, &control);
	control |= IEEE_ASYMMETRIC_PAUSE_MASK;
	control |= IEEE_PAUSE_MASK;
	control |= ADVERTISE_100;
	control |= ADVERTISE_10;
	PhyWrite(phy_addr, IEEE_AUTONEGO_ADVERTISE_REG, control);

	PhyRead(phy_addr, IEEE_1000_ADVERTISE_REG_OFFSET, &control);
	control |= ADVERTISE_1000;
	PhyWrite(phy_addr, IEEE_1000_ADVERTISE_REG_OFFSET, control);

	PhyRead(phy_addr, IEEE_CONTROL_REG_OFFSET, &control);
	control |= IEEE_CTRL_AUTONEGOTIATE_ENABLE;
	control |= IEEE_STAT_AUTONEGOTIATE_RESTART;
	PhyWrite(phy_addr, IEEE_CONTROL_REG_OFFSET, control);

	PhyRead(phy_addr, IEEE_CONTROL_REG_OFFSET, &control);
	PhyRead(phy_addr, IEEE_STATUS_REG_OFFSET, &status);

	printf("Waiting for PHY to complete autonegotiation.\r\n");

	while ( !(status & IEEE_STAT_AUTONEGOTIATE_COMPLETE) ) {
		sleep(1);
		timeout_counter++;

		if (timeout_counter == 5) {
			printf("Auto negotiation error \r\n");
			return XST_FAILURE;
		}
		PhyRead(phy_addr, IEEE_STATUS_REG_OFFSET, &status);
	}
	printf("autonegotiation complete \r\n");

	PhyRead(phy_addr, PHY_STS, &status_speed);
	if ((status_speed & 0xC000) == 0x8000) {
		return 1000;
	} else if ((status_speed & 0xC000) == 0x4000) {
		return 100;
	} else {
		return 10;
	}

	return XST_SUCCESS;
}

uint32_t DevEmacPs::get_Marvell_phy_speed(uint32_t phy_addr)
{
	uint16_t temp;
	uint16_t control;
	uint16_t status;
	uint16_t status_speed;
	uint32_t timeout_counter = 0;
	uint32_t temp_speed;

	printf("Start Marvell PHY autonegotiation \r\n");

	PhyWrite(phy_addr, IEEE_PAGE_ADDRESS_REGISTER, 2);
	PhyRead(phy_addr, IEEE_CONTROL_REG_MAC, &control);
	control |= IEEE_RGMII_TXRX_CLOCK_DELAYED_MASK;
	PhyWrite(phy_addr, IEEE_CONTROL_REG_MAC, control);

	PhyWrite(phy_addr, IEEE_PAGE_ADDRESS_REGISTER, 0);

	PhyRead(phy_addr, IEEE_AUTONEGO_ADVERTISE_REG, &control);
	control |= IEEE_ASYMMETRIC_PAUSE_MASK;
	control |= IEEE_PAUSE_MASK;
	control |= ADVERTISE_100;
	control |= ADVERTISE_10;
	PhyWrite(phy_addr, IEEE_AUTONEGO_ADVERTISE_REG, control);

	PhyRead(phy_addr, IEEE_1000_ADVERTISE_REG_OFFSET, &control);
	control |= ADVERTISE_1000;
	PhyWrite(phy_addr, IEEE_1000_ADVERTISE_REG_OFFSET, control);

	PhyWrite(phy_addr, IEEE_PAGE_ADDRESS_REGISTER, 0);
	PhyRead(phy_addr, IEEE_COPPER_SPECIFIC_CONTROL_REG,	&control);
	control |= (7 << 12);	/* max number of gigabit attempts */
	control |= (1 << 11);	/* enable downshift */
	PhyWrite(phy_addr, IEEE_COPPER_SPECIFIC_CONTROL_REG, control);
	PhyRead(phy_addr, IEEE_CONTROL_REG_OFFSET, &control);
	control |= IEEE_CTRL_AUTONEGOTIATE_ENABLE;
	control |= IEEE_STAT_AUTONEGOTIATE_RESTART;
	PhyWrite(phy_addr, IEEE_CONTROL_REG_OFFSET, control);

	PhyRead(phy_addr, IEEE_CONTROL_REG_OFFSET, &control);
	control |= IEEE_CTRL_RESET_MASK;
	PhyWrite(phy_addr, IEEE_CONTROL_REG_OFFSET, control);

	while (1)
    {
		PhyRead(phy_addr, IEEE_CONTROL_REG_OFFSET, &control);
		if (control & IEEE_CTRL_RESET_MASK)
			continue;
		else
			break;
	}

	PhyRead(phy_addr, IEEE_STATUS_REG_OFFSET, &status);

	printf("Waiting for PHY to complete autonegotiation.\r\n");

	while ( !(status & IEEE_STAT_AUTONEGOTIATE_COMPLETE) )
    {
		sleep(1);
		PhyRead(phy_addr, IEEE_COPPER_SPECIFIC_STATUS_REG_2,  &temp);
		timeout_counter++;

		if (timeout_counter == 5)
        {
			printf("Auto negotiation error \r\n");
			return XST_FAILURE;
		}
		PhyRead(phy_addr, IEEE_STATUS_REG_OFFSET, &status);
	}
	printf("autonegotiation complete \r\n");

	PhyRead(phy_addr,IEEE_SPECIFIC_STATUS_REG, &status_speed);
	if (status_speed & 0x400)
    {
		temp_speed = status_speed & IEEE_SPEED_MASK;

		if (temp_speed == IEEE_SPEED_1000)
			return 1000;
		else if(temp_speed == IEEE_SPEED_100)
			return 100;
		else
			return 10;
	}

	return XST_SUCCESS;
}

uint32_t DevEmacPs::get_Realtek_phy_speed(uint32_t phy_addr)
{
	uint16_t control;
	uint16_t status;
	uint16_t status_speed;
	uint32_t timeout_counter = 0;
	uint32_t temp_speed;

	printf("Start Realtek PHY autonegotiation \r\n");

	PhyRead(phy_addr, IEEE_AUTONEGO_ADVERTISE_REG, &control);
	control |= IEEE_ASYMMETRIC_PAUSE_MASK;
	control |= IEEE_PAUSE_MASK;
	control |= ADVERTISE_100;
	control |= ADVERTISE_10;
	PhyWrite(phy_addr, IEEE_AUTONEGO_ADVERTISE_REG, control);

	PhyRead(phy_addr, IEEE_1000_ADVERTISE_REG_OFFSET, &control);
	control |= ADVERTISE_1000;
	PhyWrite(phy_addr, IEEE_1000_ADVERTISE_REG_OFFSET, control);

	PhyRead(phy_addr, IEEE_CONTROL_REG_OFFSET, &control);
	control |= IEEE_CTRL_AUTONEGOTIATE_ENABLE;
	control |= IEEE_STAT_AUTONEGOTIATE_RESTART;
	PhyWrite(phy_addr, IEEE_CONTROL_REG_OFFSET, control);

	PhyRead(phy_addr, IEEE_CONTROL_REG_OFFSET, &control);
	control |= IEEE_CTRL_RESET_MASK;
	PhyWrite(phy_addr, IEEE_CONTROL_REG_OFFSET, control);

	while (1)
    {
		PhyRead(phy_addr, IEEE_CONTROL_REG_OFFSET, &control);
		if (control & IEEE_CTRL_RESET_MASK)
			continue;
		else
			break;
	}

	PhyRead(phy_addr, IEEE_STATUS_REG_OFFSET, &status);

	printf("Waiting for PHY to complete autonegotiation.\r\n");

	while ( !(status & IEEE_STAT_AUTONEGOTIATE_COMPLETE) )
    {
		sleep(1);
		timeout_counter++;

		if (timeout_counter == 5)
        {
			printf("Auto negotiation error \r\n");
			return XST_FAILURE;
		}
		PhyRead(phy_addr, IEEE_STATUS_REG_OFFSET, &status);
	}
	printf("autonegotiation complete \r\n");

	PhyRead(phy_addr,IEEE_SPECIFIC_STATUS_REG,
					&status_speed);
	if (status_speed & 0x400) {
		temp_speed = status_speed & IEEE_SPEED_MASK;

		if (temp_speed == IEEE_SPEED_1000)
			return 1000;
		else if(temp_speed == IEEE_SPEED_100)
			return 100;
		else
			return 10;
	}

	return XST_FAILURE;
}

#define ADIN1300_PHY_CTRL1	0x0012
#define ADIN1300_PHY_CTRL2	0x0016
#define ADIN1300_PHY_CTRL3	0x0017
#define ADIN1300_EXT_ADDR	0x0010
#define ADIN1300_EXT_DATA	0x0011
#define ADIN1300_PHY_STS1	0x001A

#define ADIN1300_RGMII_CFG	0xFF23
#define ADIN1300_RMII_CFG	0xFF24

#define ADIN1300_AUTO_MDI_EN	0x400
#define ADIN1300_MAN_MDIX_EN	0x200
#define ADIN1300_DIAG_CLK_EN	0x4

#define ADIN1300_LINKING_EN	0x2000

#define ADIN1300_RGMII_EN		0x0001
#define ADIN1300_RGMII_TXRX_TUNING_EN	0x0006
#define ADIN1300_RGMII_RX_DELAY_MASK	0x01C0
#define ADIN1300_RGMII_TX_DELAY_MASK	0x0038
#define ADIN1300_RGMII_RX_DELAY_VAL_2000PS	0x0
#define ADIN1300_RGMII_TX_DELAY_VAL_2000PS	0x0

#define ADIN1300_SPEED_RETRY_MASK	0x1C00
#define ADIN1300_SPEED_RETRY_FOUR	0x1000
#define ADIN1300_DOWNSPEED_EN		0x0C00

#define ADIN1300_AN_DONE	0x1000
#define ADIN1300_SPEED_MASK	0x0380
#define ADIN1300_SPEED_1G	0x0280
#define ADIN1300_SPEED_100M	0x0180
#define ADIN1300_SPEED_10M	0x0080

uint32_t DevEmacPs::get_Adi_phy_speed(uint32_t phy_addr)
{
	uint16_t temp;
	uint16_t control;
	uint16_t status;
	uint16_t status_speed;
	uint16_t phyreg;
	uint32_t timeout_counter = 0;
	uint32_t temp_speed;

	printf("Start adi PHY autonegotiation \r\n");

	/* PHY soft reset */
	PhyRead(phy_addr, IEEE_CONTROL_REG_OFFSET, &control);
	control |= IEEE_CTRL_RESET_MASK;
	PhyWrite(phy_addr, IEEE_CONTROL_REG_OFFSET, control);

	/* Delay for PHY to be accessible */
	sleep(1);

	/* RGMII TX/RX tuning */
	PhyWrite(phy_addr, ADIN1300_EXT_ADDR, ADIN1300_RGMII_CFG);
	PhyRead(phy_addr, ADIN1300_EXT_DATA, &phyreg);
	phyreg |= (ADIN1300_RGMII_EN | ADIN1300_RGMII_TXRX_TUNING_EN);
	phyreg &= ~(ADIN1300_RGMII_RX_DELAY_MASK | ADIN1300_RGMII_TX_DELAY_MASK);
	phyreg |= ((ADIN1300_RGMII_RX_DELAY_VAL_2000PS << 6) | (ADIN1300_RGMII_TX_DELAY_VAL_2000PS << 3));
	PhyWrite(phy_addr, ADIN1300_EXT_ADDR, ADIN1300_RGMII_CFG);
	PhyWrite(phy_addr, ADIN1300_EXT_DATA, phyreg);

	/* Downspeed */
	PhyRead(phy_addr, ADIN1300_PHY_CTRL3, &phyreg);
	phyreg &= ~(ADIN1300_SPEED_RETRY_MASK);
	phyreg |= ADIN1300_SPEED_RETRY_FOUR;
	PhyWrite(phy_addr, ADIN1300_PHY_CTRL3, phyreg);
	PhyRead(phy_addr, ADIN1300_PHY_CTRL2, &phyreg);
	phyreg |= ADIN1300_DOWNSPEED_EN;
	PhyWrite(phy_addr, ADIN1300_PHY_CTRL2, phyreg);

	/* Diag clock disable */
	PhyRead(phy_addr, ADIN1300_PHY_CTRL1, &phyreg);
	phyreg &= ~ADIN1300_DIAG_CLK_EN;
	PhyWrite(phy_addr, ADIN1300_PHY_CTRL1, phyreg);
	/* Linking Enable */
	PhyRead(phy_addr, ADIN1300_PHY_CTRL3, &phyreg);
	phyreg |= ADIN1300_LINKING_EN;
	PhyWrite(phy_addr, ADIN1300_PHY_CTRL3, phyreg);
	/* Auto MDIX by default */
	PhyRead(phy_addr, ADIN1300_PHY_CTRL1, &phyreg);
	phyreg &= ~ADIN1300_MAN_MDIX_EN;
	phyreg |= ADIN1300_AUTO_MDI_EN;
	PhyWrite(phy_addr, ADIN1300_PHY_CTRL1, phyreg);

	PhyRead(phy_addr, IEEE_AUTONEGO_ADVERTISE_REG, &control);
	control |= IEEE_ASYMMETRIC_PAUSE_MASK;
	control |= IEEE_PAUSE_MASK;
	control |= ADVERTISE_100;
	control |= ADVERTISE_10;
	PhyWrite(phy_addr, IEEE_AUTONEGO_ADVERTISE_REG, control);

	PhyRead(phy_addr, IEEE_1000_ADVERTISE_REG_OFFSET,
					&control);
	control |= ADVERTISE_1000;
	PhyWrite(phy_addr, IEEE_1000_ADVERTISE_REG_OFFSET,
					control);

	PhyRead(phy_addr, IEEE_CONTROL_REG_OFFSET, &control);
	control |= IEEE_CTRL_AUTONEGOTIATE_ENABLE;
	control |= IEEE_STAT_AUTONEGOTIATE_RESTART;
	PhyWrite(phy_addr, IEEE_CONTROL_REG_OFFSET, control);

	while (1)
    {
		PhyRead(phy_addr, IEEE_CONTROL_REG_OFFSET, &control);
		if (control & IEEE_CTRL_RESET_MASK)
			continue;
		else
			break;
	}

	PhyRead(phy_addr, IEEE_STATUS_REG_OFFSET, &status);

	printf("Waiting for PHY to complete autonegotiation.\r\n");

	while ( !(status & IEEE_STAT_AUTONEGOTIATE_COMPLETE) )
    {
		sleep(1);
		PhyRead(phy_addr, IEEE_COPPER_SPECIFIC_STATUS_REG_2,  &temp);
		timeout_counter++;

		if (timeout_counter == 30)
        {
			printf("Auto negotiation error \r\n");
			return XST_FAILURE;
		}
		PhyRead(phy_addr, IEEE_STATUS_REG_OFFSET, &status);
	}
	printf("autonegotiation complete \r\n");

	PhyRead(phy_addr,ADIN1300_PHY_STS1,	&status_speed);
	if (status_speed & ADIN1300_AN_DONE)
    {
		temp_speed = status_speed & ADIN1300_SPEED_MASK;

		if (temp_speed == ADIN1300_SPEED_1G)
			return 1000;
		else if(temp_speed == ADIN1300_SPEED_100M)
			return 100;
		else
			return 10;
	}

	return XST_SUCCESS;
}

#define IEEE_CTRL_ISOLATE_DISABLE               0xFBFF
// xemacpsif_physpeed.c:
uint32_t DevEmacPs::get_Xilinx_pcs_pma_phy_speed(uint32_t phy_addr)
{

#if XPAR_GIGE_PCS_PMA_1000BASEX_CORE_PRESENT == 1 || \
    XPAR_GIGE_PCS_PMA_SGMII_CORE_PRESENT == 1 || defined(SDT)
	uint16_t temp;
#endif

	uint16_t control;
	uint16_t status;

	printf("Start Xilinx_pcs_pma_phy PHY autonegotiation \r\n");

	PhyRead(phy_addr, IEEE_CONTROL_REG_OFFSET, &control);
	control |= IEEE_CTRL_AUTONEGOTIATE_ENABLE;
	control |= IEEE_STAT_AUTONEGOTIATE_RESTART;
	control &= IEEE_CTRL_ISOLATE_DISABLE;
	PhyWrite(phy_addr, IEEE_CONTROL_REG_OFFSET, control);

	printf("Waiting for PHY to complete autonegotiation.\r\n");

	PhyRead(phy_addr, IEEE_STATUS_REG_OFFSET, &status);
	while ( !(status & IEEE_STAT_AUTONEGOTIATE_COMPLETE) ) {
		sleep(1);
		PhyRead(phy_addr, IEEE_STATUS_REG_OFFSET,&status);
	}
	printf("autonegotiation complete \r\n");
#ifndef SDT
#if XPAR_GIGE_PCS_PMA_1000BASEX_CORE_PRESENT == 1
	PhyWrite(phy_addr, IEEE_PAGE_ADDRESS_REGISTER, 1);
	PhyRead(phy_addr, IEEE_PARTNER_ABILITIES_1_REG_OFFSET, &temp);
	if ((temp & 0x0020) == 0x0020) {
		PhyWrite(phy_addr, IEEE_PAGE_ADDRESS_REGISTER, 0);
		return 1000;
	}
	else {
		PhyWrite(phy_addr, IEEE_PAGE_ADDRESS_REGISTER, 0);
		printf("Link error, temp = %x\r\n", temp);
		return 0;
	}
#elif XPAR_GIGE_PCS_PMA_SGMII_CORE_PRESENT == 1
	printf("Waiting for Link to be up; Polling for SGMII core Reg \r\n");
	PhyRead(phy_addr, IEEE_PARTNER_ABILITIES_1_REG_OFFSET, &temp);
	while(!(temp & 0x8000)) {
		PhyRead(phy_addr, IEEE_PARTNER_ABILITIES_1_REG_OFFSET, &temp);
	}
	if((temp & 0x0C00) == 0x0800) {
		return 1000;
	}
	else if((temp & 0x0C00) == 0x0400) {
		return 100;
	}
	else if((temp & 0x0C00) == 0x0000) {
		return 10;
	} else {
		printf("get_IEEE_phy_speed(): Invalid speed bit value, Defaulting to Speed = 10 Mbps\r\n");
		PhyRead(phy_addr, IEEE_CONTROL_REG_OFFSET, &temp);
		PhyWrite(phy_addr, IEEE_CONTROL_REG_OFFSET, 0x0100);
		return 10;
	}
#endif
#else
	if (strcmp(xemacpsp->Config.PhyType, "1000base-x") == 0) {
		PhyWrite(phy_addr, IEEE_PAGE_ADDRESS_REGISTER, 1);
		PhyRead(phy_addr, IEEE_PARTNER_ABILITIES_1_REG_OFFSET, &temp);
		if ((temp & 0x0020) == 0x0020) {
			PhyWrite(phy_addr, IEEE_PAGE_ADDRESS_REGISTER, 0);
			return 1000;
		}
		else {
			PhyWrite(phy_addr, IEEE_PAGE_ADDRESS_REGISTER, 0);
			printf("Link error, temp = %x\r\n", temp);
			return 0;
		}
	}
	if (strcmp(xemacpsp->Config.PhyType, "sgmii") == 0) {
		printf("Waiting for Link to be up; Polling for SGMII core Reg \r\n");
		PhyRead(phy_addr, IEEE_PARTNER_ABILITIES_1_REG_OFFSET, &temp);
		while(!(temp & 0x8000)) {
			PhyRead(phy_addr, IEEE_PARTNER_ABILITIES_1_REG_OFFSET, &temp);
		}
		if((temp & 0x0C00) == 0x0800) {
			return 1000;
		}
		else if((temp & 0x0C00) == 0x0400) {
			return 100;
		}
		else if((temp & 0x0C00) == 0x0000) {
			return 10;
		} else {
			printf("get_IEEE_phy_speed(): Invalid speed bit value, Defaulting to Speed = 10 Mbps\r\n");
			PhyRead(phy_addr, IEEE_CONTROL_REG_OFFSET, &temp);
			PhyWrite(phy_addr, IEEE_CONTROL_REG_OFFSET, 0x0100);
			return 10;
		}
	}
#endif
	return 0;
}


void DevEmacPs::dumpRegisters()
{
    printf("EmacPs-RegisterDump\r\n");
    printf("--------------------\r\n");
    printf("NWCTRL(%x)=%x\tNetworkcontrol reg\r\n",XEMACPS_NWCTRL_OFFSET,In32_silent(XEMACPS_NWCTRL_OFFSET));
    printf("NWCFG(%x)=%x\tNetwork Config reg\r\n",XEMACPS_NWCFG_OFFSET,In32_silent(XEMACPS_NWCFG_OFFSET));
    printf("NWSR(%x)=%x\tNetwork Status reg\r\n",XEMACPS_NWSR_OFFSET,In32_silent(XEMACPS_NWSR_OFFSET));

    printf("DMACR(%x)=%x\r\n",XEMACPS_DMACR_OFFSET,In32_silent(XEMACPS_DMACR_OFFSET)); /**< DMA Control reg */
    printf("TXSR(%x)=%x\r\n",XEMACPS_TXSR_OFFSET,In32_silent(XEMACPS_TXSR_OFFSET)); /**< TX Status reg */
    printf("RXQBASE(%x)=%x\r\n",XEMACPS_RXQBASE_OFFSET,In32_silent(XEMACPS_RXQBASE_OFFSET)); /**< RX Q Base address reg */
    printf("TXQBASE(%x)=%x\r\n",XEMACPS_TXQBASE_OFFSET,In32_silent(XEMACPS_TXQBASE_OFFSET)); /**< TX Q Base address reg */
    printf("RXSR(%x)=%x\r\n",XEMACPS_RXSR_OFFSET,In32_silent(XEMACPS_RXSR_OFFSET)); /**< RX Status reg */

    printf("ISR(%x)=%x\r\n",XEMACPS_ISR_OFFSET,In32_silent(XEMACPS_ISR_OFFSET)); /**< Interrupt Status reg */
    printf("IER(%x)=%x\r\n",XEMACPS_IER_OFFSET,In32_silent(XEMACPS_IER_OFFSET)); /**< Interrupt Enable reg */
    printf("IDR(%x)=%x\r\n",XEMACPS_IDR_OFFSET,In32_silent(XEMACPS_IDR_OFFSET)); /**< Interrupt Disable reg */
    printf("IMR(%x)=%x\r\n",XEMACPS_IMR_OFFSET,In32_silent(XEMACPS_IMR_OFFSET)); /**< Interrupt Mask reg */

    printf("PHYMNTNC(%x)=%x\r\n",XEMACPS_PHYMNTNC_OFFSET,In32_silent(XEMACPS_PHYMNTNC_OFFSET)); /**< Phy Maintaince reg */
    printf("RXPAUSE(%x)=%x\r\n",XEMACPS_RXPAUSE_OFFSET,In32_silent(XEMACPS_RXPAUSE_OFFSET)); /**< RX Pause Time reg */
    printf("TXPAUSE(%x)=%x\r\n",XEMACPS_TXPAUSE_OFFSET,In32_silent(XEMACPS_TXPAUSE_OFFSET)); /**< TX Pause Time reg */

    printf("JUMBOMAXLEN(%x)=%x\r\n",XEMACPS_JUMBOMAXLEN_OFFSET,In32_silent(XEMACPS_JUMBOMAXLEN_OFFSET)); /**< Jumbo max length reg */

    printf("RXWATERMARK(%x)=%x\r\n",XEMACPS_RXWATERMARK_OFFSET,In32_silent(XEMACPS_RXWATERMARK_OFFSET)); /**< RX watermark reg */

    printf("HASHL(%x)=%x\r\n",XEMACPS_HASHL_OFFSET,In32_silent(XEMACPS_HASHL_OFFSET)); /**< Hash Low address reg */
    printf("HASHH(%x)=%x\r\n",XEMACPS_HASHH_OFFSET,In32_silent(XEMACPS_HASHH_OFFSET)); /**< Hash High address reg */

    printf("LADDR1L(%x)=%x\r\n",XEMACPS_LADDR1L_OFFSET,In32_silent(XEMACPS_LADDR1L_OFFSET)); /**< Specific1 addr low reg */
    printf("LADDR1H(%x)=%x\r\n",XEMACPS_LADDR1H_OFFSET,In32_silent(XEMACPS_LADDR1H_OFFSET)); /**< Specific1 addr high reg */
    printf("LADDR2L(%x)=%x\r\n",XEMACPS_LADDR2L_OFFSET,In32_silent(XEMACPS_LADDR2L_OFFSET)); /**< Specific2 addr low reg */
    printf("LADDR2H(%x)=%x\r\n",XEMACPS_LADDR2H_OFFSET,In32_silent(XEMACPS_LADDR2H_OFFSET)); /**< Specific2 addr high reg */
    printf("LADDR3L(%x)=%x\r\n",XEMACPS_LADDR3L_OFFSET,In32_silent(XEMACPS_LADDR3L_OFFSET)); /**< Specific3 addr low reg */
    printf("LADDR3H(%x)=%x\r\n",XEMACPS_LADDR3H_OFFSET,In32_silent(XEMACPS_LADDR3H_OFFSET)); /**< Specific3 addr high reg */
    printf("LADDR4L(%x)=%x\r\n",XEMACPS_LADDR4L_OFFSET,In32_silent(XEMACPS_LADDR4L_OFFSET)); /**< Specific4 addr low reg */
    printf("LADDR4H(%x)=%x\r\n",XEMACPS_LADDR4H_OFFSET,In32_silent(XEMACPS_LADDR4H_OFFSET)); /**< Specific4 addr high reg */

    printf("MATCH1(%x)=%x\r\n",XEMACPS_MATCH1_OFFSET,In32_silent(XEMACPS_MATCH1_OFFSET)); /**< Type ID1 Match reg */
    printf("MATCH2(%x)=%x\r\n",XEMACPS_MATCH2_OFFSET,In32_silent(XEMACPS_MATCH2_OFFSET)); /**< Type ID2 Match reg */
    printf("MATCH3(%x)=%x\r\n",XEMACPS_MATCH3_OFFSET,In32_silent(XEMACPS_MATCH3_OFFSET)); /**< Type ID3 Match reg */
    printf("MATCH4(%x)=%x\r\n",XEMACPS_MATCH4_OFFSET,In32_silent(XEMACPS_MATCH4_OFFSET)); /**< Type ID4 Match reg */

    printf("STRETCH(%x)=%x\r\n",XEMACPS_STRETCH_OFFSET,In32_silent(XEMACPS_STRETCH_OFFSET)); /**< IPG Stretch reg */

    printf("OCTTXL(%x)=%x\r\n",XEMACPS_OCTTXL_OFFSET,In32_silent(XEMACPS_OCTTXL_OFFSET)); /**< Octects transmitted Low reg */
    printf("OCTTXH(%x)=%x\r\n",XEMACPS_OCTTXH_OFFSET,In32_silent(XEMACPS_OCTTXH_OFFSET)); /**< Octects transmitted High reg */

    printf("TXCNT(%x)=%x\r\n",XEMACPS_TXCNT_OFFSET,In32_silent(XEMACPS_TXCNT_OFFSET)); /**< Error-free Frmaes transmitted counter */
    printf("TXBCCNT(%x)=%x\r\n",XEMACPS_TXBCCNT_OFFSET,In32_silent(XEMACPS_TXBCCNT_OFFSET)); /**< Error-free Broadcast Frames counter*/
    printf("TXMCCNT(%x)=%x\r\n",XEMACPS_TXMCCNT_OFFSET,In32_silent(XEMACPS_TXMCCNT_OFFSET)); /**< Error-free Multicast Frame counter */
    printf("TXPAUSECNT(%x)=%x\r\n",XEMACPS_TXPAUSECNT_OFFSET,In32_silent(XEMACPS_TXPAUSECNT_OFFSET)); /**< Pause Frames Transmitted Counter */
    printf("TX64CNT(%x)=%x\r\n",XEMACPS_TX64CNT_OFFSET,In32_silent(XEMACPS_TX64CNT_OFFSET)); /**< Error-free 64 byte Frames Transmitted counter */
    printf("TX65CNT(%x)=%x\r\n",XEMACPS_TX65CNT_OFFSET,In32_silent(XEMACPS_TX65CNT_OFFSET)); /**< Error-free 65-127 byte Frames Transmitted counter */
    printf("TX128CNT(%x)=%x\r\n",XEMACPS_TX128CNT_OFFSET,In32_silent(XEMACPS_TX128CNT_OFFSET)); /**< Error-free 128-255 byte Frames Transmitted counter*/
    printf("TX256CNT(%x)=%x\r\n",XEMACPS_TX256CNT_OFFSET,In32_silent(XEMACPS_TX256CNT_OFFSET)); /**< Error-free 256-511 byte Frames transmitted counter */
    printf("TX512CNT(%x)=%x\r\n",XEMACPS_TX512CNT_OFFSET,In32_silent(XEMACPS_TX512CNT_OFFSET)); /**< Error-free 512-1023 byte Frames transmitted counter */
    printf("TX1024CNT(%x)=%x\r\n",XEMACPS_TX1024CNT_OFFSET,In32_silent(XEMACPS_TX1024CNT_OFFSET)); /**< Error-free 1024-1518 byte Frames transmitted counter */
    printf("TX1519CNT(%x)=%x\r\n",XEMACPS_TX1519CNT_OFFSET,In32_silent(XEMACPS_TX1519CNT_OFFSET)); /**< Error-free larger than 1519 byte Frames transmitted counter */
    printf("TXURUNCNT(%x)=%x\r\n",XEMACPS_TXURUNCNT_OFFSET,In32_silent(XEMACPS_TXURUNCNT_OFFSET)); /**< TX under run error counter */

    printf("SNGLCOLLCNT(%x)=%x\r\n",XEMACPS_SNGLCOLLCNT_OFFSET,In32_silent(XEMACPS_SNGLCOLLCNT_OFFSET)); /**< Single Collision Frame Counter */
    printf("MULTICOLLCNT(%x)=%x\r\n",XEMACPS_MULTICOLLCNT_OFFSET,In32_silent(XEMACPS_MULTICOLLCNT_OFFSET)); /**< Multiple Collision Frame Counter */
    printf("EXCESSCOLLCNT(%x)=%x\r\n",XEMACPS_EXCESSCOLLCNT_OFFSET,In32_silent(XEMACPS_EXCESSCOLLCNT_OFFSET)); /**< Excessive Collision Frame Counter */
    printf("LATECOLLCNT(%x)=%x\r\n",XEMACPS_LATECOLLCNT_OFFSET,In32_silent(XEMACPS_LATECOLLCNT_OFFSET)); /**< Late Collision Frame Counter */
    printf("TXDEFERCNT(%x)=%x\r\n",XEMACPS_TXDEFERCNT_OFFSET,In32_silent(XEMACPS_TXDEFERCNT_OFFSET)); /**< Deferred Transmission Frame Counter */
    printf("TXCSENSECNT(%x)=%x\r\n",XEMACPS_TXCSENSECNT_OFFSET,In32_silent(XEMACPS_TXCSENSECNT_OFFSET)); /**< Transmit Carrier Sense Error Counter */

    printf("OCTRXL(%x)=%x\r\n",XEMACPS_OCTRXL_OFFSET,In32_silent(XEMACPS_OCTRXL_OFFSET)); /**< Octects Received register Low */
    printf("OCTRXH(%x)=%x\r\n",XEMACPS_OCTRXH_OFFSET,In32_silent(XEMACPS_OCTRXH_OFFSET)); /**< Octects Received register High */

    printf("RXCNT(%x)=%x\r\n",XEMACPS_RXCNT_OFFSET,In32_silent(XEMACPS_RXCNT_OFFSET)); /**< Error-free Frames Received Counter */
    printf("RXBROADCNT(%x)=%x\r\n",XEMACPS_RXBROADCNT_OFFSET,In32_silent(XEMACPS_RXBROADCNT_OFFSET)); /**< Error-free Broadcast Frames Received Counter */
    printf("RXMULTICNT(%x)=%x\r\n",XEMACPS_RXMULTICNT_OFFSET,In32_silent(XEMACPS_RXMULTICNT_OFFSET)); /**< Error-free Multicast Frames Received Counter */
    printf("RXPAUSECNT(%x)=%x\r\n",XEMACPS_RXPAUSECNT_OFFSET,In32_silent(XEMACPS_RXPAUSECNT_OFFSET)); /**< Pause Frames Received Counter */
    printf("RX64CNT(%x)=%x\r\n",XEMACPS_RX64CNT_OFFSET,In32_silent(XEMACPS_RX64CNT_OFFSET)); /**< Error-free 64 byte Frames Received Counter */
    printf("RX65CNT(%x)=%x\r\n",XEMACPS_RX65CNT_OFFSET,In32_silent(XEMACPS_RX65CNT_OFFSET)); /**< Error-free 65-127 byte Frames Received Counter */
    printf("RX128CNT(%x)=%x\r\n",XEMACPS_RX128CNT_OFFSET,In32_silent(XEMACPS_RX128CNT_OFFSET)); /**< Error-free 128-255 byte Frames Received Counter */
    printf("RX256CNT(%x)=%x\r\n",XEMACPS_RX256CNT_OFFSET,In32_silent(XEMACPS_RX256CNT_OFFSET)); /**< Error-free 256-512 byte Frames Received Counter */
    printf("RX512CNT(%x)=%x\r\n",XEMACPS_RX512CNT_OFFSET,In32_silent(XEMACPS_RX512CNT_OFFSET)); /**< Error-free 512-1023 byte Frames Received Counter */
    printf("RX1024CNT(%x)=%x\r\n",XEMACPS_RX1024CNT_OFFSET,In32_silent(XEMACPS_RX1024CNT_OFFSET)); /**< Error-free 1024-1518 byte Frames Received Counter */
    printf("RX1519CNT(%x)=%x\r\n",XEMACPS_RX1519CNT_OFFSET,In32_silent(XEMACPS_RX1519CNT_OFFSET)); /**< Error-free 1519-max byte Frames Received Counter */
    printf("RXUNDRCNT(%x)=%x\r\n",XEMACPS_RXUNDRCNT_OFFSET,In32_silent(XEMACPS_RXUNDRCNT_OFFSET)); /**< Undersize Frames Received Counter */
    printf("RXOVRCNT(%x)=%x\r\n",XEMACPS_RXOVRCNT_OFFSET,In32_silent(XEMACPS_RXOVRCNT_OFFSET)); /**< Oversize Frames Received Counter */
    printf("RXJABCNT(%x)=%x\r\n",XEMACPS_RXJABCNT_OFFSET,In32_silent(XEMACPS_RXJABCNT_OFFSET)); /**< Jabbers Received Counter */
    printf("RXFCSCNT(%x)=%x\r\n",XEMACPS_RXFCSCNT_OFFSET,In32_silent(XEMACPS_RXFCSCNT_OFFSET)); /**< Frame Check Sequence Error Counter */
    printf("RXLENGTHCNT(%x)=%x\r\n",XEMACPS_RXLENGTHCNT_OFFSET,In32_silent(XEMACPS_RXLENGTHCNT_OFFSET)); /**< Length Field Error Counter */
    printf("RXSYMBCNT(%x)=%x\r\n",XEMACPS_RXSYMBCNT_OFFSET,In32_silent(XEMACPS_RXSYMBCNT_OFFSET)); /**< Symbol Error Counter */
    printf("RXALIGNCNT(%x)=%x\r\n",XEMACPS_RXALIGNCNT_OFFSET,In32_silent(XEMACPS_RXALIGNCNT_OFFSET)); /**< Alignment Error Counter */
    printf("RXRESERRCNT(%x)=%x\r\n",XEMACPS_RXRESERRCNT_OFFSET,In32_silent(XEMACPS_RXRESERRCNT_OFFSET)); /**< Receive Resource Error Counter */
    printf("RXORCNT(%x)=%x\r\n",XEMACPS_RXORCNT_OFFSET,In32_silent(XEMACPS_RXORCNT_OFFSET)); /**< Receive Overrun Counter */
    printf("RXIPCCNT(%x)=%x\r\n",XEMACPS_RXIPCCNT_OFFSET,In32_silent(XEMACPS_RXIPCCNT_OFFSET)); /**< IP header Checksum Error Counter */
    printf("RXTCPCCNT(%x)=%x\r\n",XEMACPS_RXTCPCCNT_OFFSET,In32_silent(XEMACPS_RXTCPCCNT_OFFSET)); /**< TCP Checksum Error Counter */
    printf("RXUDPCCNT(%x)=%x\r\n",XEMACPS_RXUDPCCNT_OFFSET,In32_silent(XEMACPS_RXUDPCCNT_OFFSET)); /**< UDP Checksum Error Counter */
    printf("LAST(%x)=%x\r\n",XEMACPS_LAST_OFFSET,In32_silent(XEMACPS_LAST_OFFSET)); /**< Last statistic counter offset, for clearing */
    
    printf("1588_SEC(%x)=%x\r\n",XEMACPS_1588_SEC_OFFSET,In32_silent(XEMACPS_1588_SEC_OFFSET));  /**< 1588 second counter */
    printf("1588_NANOSEC(%x)=%x\r\n",XEMACPS_1588_NANOSEC_OFFSET,In32_silent(XEMACPS_1588_NANOSEC_OFFSET)); /**< 1588 nanosecond counter */
    printf("1588_ADJ(%x)=%x\r\n",XEMACPS_1588_ADJ_OFFSET,In32_silent(XEMACPS_1588_ADJ_OFFSET));   /**< 1588 nanosecond adjustment counter */
    printf("1588_INC(%x)=%x\r\n",XEMACPS_1588_INC_OFFSET,In32_silent(XEMACPS_1588_INC_OFFSET));   /**< 1588 nanosecond increment counter */
    printf("PTP_TXSEC(%x)=%x\r\n",XEMACPS_PTP_TXSEC_OFFSET,In32_silent(XEMACPS_PTP_TXSEC_OFFSET));   /**< 1588 PTP transmit second counter */
    printf("PTP_TXNANOSEC(%x)=%x\r\n",XEMACPS_PTP_TXNANOSEC_OFFSET,In32_silent(XEMACPS_PTP_TXNANOSEC_OFFSET)); /**< 1588 PTP transmit nanosecond counter */
    printf("PTP_RXSEC(%x)=%x\r\n",XEMACPS_PTP_RXSEC_OFFSET,In32_silent(XEMACPS_PTP_RXSEC_OFFSET));  /**< 1588 PTP receive second counter */
    printf("PTP_RXNANOSEC(%x)=%x\r\n",XEMACPS_PTP_RXNANOSEC_OFFSET,In32_silent(XEMACPS_PTP_RXNANOSEC_OFFSET)); /**< 1588 PTP receive nanosecond counter */
    printf("PTPP_TXSEC(%x)=%x\r\n",XEMACPS_PTPP_TXSEC_OFFSET,In32_silent(XEMACPS_PTPP_TXSEC_OFFSET)); /**< 1588 PTP peer transmit second counter */
    printf("PTPP_TXNANOSEC(%x)=%x\r\n",XEMACPS_PTPP_TXNANOSEC_OFFSET,In32_silent(XEMACPS_PTPP_TXNANOSEC_OFFSET)); /**< 1588 PTP peer transmit nanosecond counter */
    printf("PTPP_RXSEC(%x)=%x\r\n",XEMACPS_PTPP_RXSEC_OFFSET,In32_silent(XEMACPS_PTPP_RXSEC_OFFSET)); /**< 1588 PTP peer receive second counter */
    printf("PTPP_RXNANOSEC(%x)=%x\r\n",XEMACPS_PTPP_RXNANOSEC_OFFSET,In32_silent(XEMACPS_PTPP_RXNANOSEC_OFFSET)); /**< 1588 PTP peer receive nanosecond counter */
    printf("PCS_CONTROL(%x)=%x\r\n",XEMACPS_PCS_CONTROL_OFFSET,In32_silent(XEMACPS_PCS_CONTROL_OFFSET)); /** PCS control register */
    printf("PCS_STATUS(%x)=%x\r\n",XEMACPS_PCS_STATUS_OFFSET,In32_silent(XEMACPS_PCS_STATUS_OFFSET)); /** PCS status register */

    printf("DCFG6(%x)=%x\r\n",XEMACPS_DCFG6_OFFSET,In32_silent(XEMACPS_DCFG6_OFFSET)); /** designcfg_debug6 register */

    printf("INTQ1_STS(%x)=%x\r\n",XEMACPS_INTQ1_STS_OFFSET,In32_silent(XEMACPS_INTQ1_STS_OFFSET)); /**< Interrupt Q1 Status reg */
    printf("TXQ1BASE(%x)=%x\r\n",XEMACPS_TXQ1BASE_OFFSET,In32_silent(XEMACPS_TXQ1BASE_OFFSET)); /**< TX Q1 Base address reg */
    printf("RXQ1BASE(%x)=%x\r\n",XEMACPS_RXQ1BASE_OFFSET,In32_silent(XEMACPS_RXQ1BASE_OFFSET)); /**< RX Q1 Base address reg */
    printf("DMA_RXQ1_BUFSIZE(%x)=%x\r\n",XEMACPS_DMA_RXQ1_BUFSIZE_OFFSET,In32_silent(XEMACPS_DMA_RXQ1_BUFSIZE_OFFSET)); /**< RX Q1 DMA buffer size reg */
    printf("MSBBUF_TXQBASE(%x)=%x\r\n",XEMACPS_MSBBUF_TXQBASE_OFFSET,In32_silent(XEMACPS_MSBBUF_TXQBASE_OFFSET)); /**< MSB Buffer TX Q Base reg */
    printf("MSBBUF_RXQBASE(%x)=%x\r\n",XEMACPS_MSBBUF_RXQBASE_OFFSET,In32_silent(XEMACPS_MSBBUF_RXQBASE_OFFSET)); /**< MSB Buffer RX Q Base reg */
    printf("SCREEN_TYPE2_REG0(%x)=%x\r\n",XEMACPS_SCREEN_TYPE2_REG0,In32_silent(XEMACPS_SCREEN_TYPE2_REG0)); /** Screening Type2 Reg0 **/

    printf("INTQ1_IER(%x)=%x\r\n",XEMACPS_INTQ1_IER_OFFSET,In32_silent(XEMACPS_INTQ1_IER_OFFSET)); /**< Interrupt Q1 Enable reg */
    printf("INTQ1_IDR(%x)=%x\r\n",XEMACPS_INTQ1_IDR_OFFSET,In32_silent(XEMACPS_INTQ1_IDR_OFFSET)); /**< Interrupt Q1 Disable reg */
    printf("INTQ1_IMR(%x)=%x\r\n",XEMACPS_INTQ1_IMR_OFFSET,In32_silent(XEMACPS_INTQ1_IMR_OFFSET)); /**< Interrupt Q1 Mask reg */
}

/**
* Master interrupt handler for EMAC driver. This routine will query the
* status of the device, bump statistics, and invoke user callbacks.
*
* This routine must be connected to an interrupt controller using OS/BSP
* specific methods.
*
******************************************************************************/
void DevEmacPs::handle_irq()
{
  uint32_t RegISR;
  uint32_t RegSR;
  uint32_t RegCtrl;
  uint32_t RegQ1ISR = 0U;

  Xil_AssertVoid(isReady_ == (uint32_t) XIL_COMPONENT_IS_READY);

  printf("INT-C++\r\n");
  /* This ISR will try to handle as many interrupts as it can in a single
   * call. However, in most of the places where the user's error handler
   * is called, this ISR exits because it is expected that the user will
   * reset the device in nearly all instances.
   */
  RegISR = In32(XEMACPS_ISR_OFFSET);

  /* Read Transmit Q1 ISR */
  if (Version > 2)
  {
    RegQ1ISR = In32(XEMACPS_INTQ1_STS_OFFSET);
  }

  /* Clear the interrupt status register */
  Out32(XEMACPS_ISR_OFFSET, RegISR);

  /* Receive complete interrupt */
  if ((RegISR & XEMACPS_IXR_FRAMERX_MASK) != 0x00000000U)
  {
    RecvHandler();
    /* Clear RX status register RX complete indication but preserve
     * error bits if there is any */
    Out32(XEMACPS_RXSR_OFFSET, ((uint32_t) XEMACPS_RXSR_FRAMERX_MASK | (uint32_t) XEMACPS_RXSR_BUFFNA_MASK));
  }

  /* Transmit Q1 complete interrupt */
  if ((Version > 2) && ((RegQ1ISR & XEMACPS_INTQ1SR_TXCOMPL_MASK) != 0x00000000U))
  {
    /* Clear TX status register TX complete indication but preserve
     * error bits if there is any */
    Out32(XEMACPS_INTQ1_STS_OFFSET, XEMACPS_INTQ1SR_TXCOMPL_MASK);
    Out32(XEMACPS_TXSR_OFFSET, ((uint32_t) XEMACPS_TXSR_TXCOMPL_MASK | (uint32_t)XEMACPS_TXSR_USEDREAD_MASK));
    SendHandler();
  }

  /* Transmit complete interrupt */
  if ((RegISR & XEMACPS_IXR_TXCOMPL_MASK) != 0x00000000U)
  {
    /* Clear TX status register TX complete indication but preserve
     * error bits if there is any */
    Out32(XEMACPS_TXSR_OFFSET, ((uint32_t) XEMACPS_TXSR_TXCOMPL_MASK | (uint32_t)XEMACPS_TXSR_USEDREAD_MASK));
    SendHandler();
  }

  /* Receive error conditions interrupt */
  if ((RegISR & XEMACPS_IXR_RX_ERR_MASK) != 0x00000000U)
  {
    /* Clear RX status register */
    RegSR = In32(XEMACPS_RXSR_OFFSET);
    Out32(XEMACPS_RXSR_OFFSET, RegSR);

    /* Fix for CR # 692702. Write to bit 18 of net_ctrl
     * register to flush a packet out of Rx SRAM upon
     * an error for receive buffer not available. */
    if ((RegISR & XEMACPS_IXR_RXUSED_MASK) != 0x00000000U)
    {
      RegCtrl = In32(XEMACPS_NWCTRL_OFFSET);
      RegCtrl |= (uint32_t) XEMACPS_NWCTRL_FLUSH_DPRAM_MASK;
      Out32(XEMACPS_NWCTRL_OFFSET, RegCtrl);
    }

    if (RegSR != 0)
    {
      ErrorHandler(XEMACPS_RECV, RegSR);
    }
  }

  /* When XEMACPS_IXR_TXCOMPL_MASK is flagged, XEMACPS_IXR_TXUSED_MASK
   * will be asserted the same time.
   * Have to distinguish this bit to handle the real error condition.
   */
  /* Transmit Q1 error conditions interrupt */
  if ((Version > 2) &&
      ((RegQ1ISR & XEMACPS_INTQ1SR_TXERR_MASK) != 0x00000000U) &&
      ((RegQ1ISR & XEMACPS_INTQ1SR_TXCOMPL_MASK) != 0x00000000U))
  {
    /* Clear Interrupt Q1 status register */
    Out32(XEMACPS_INTQ1_STS_OFFSET, RegQ1ISR);
    ErrorHandler(XEMACPS_SEND, RegQ1ISR);
  }

  /* Transmit error conditions interrupt */
  if (((RegISR & XEMACPS_IXR_TX_ERR_MASK) != 0x00000000U) &&
      (!(RegISR & XEMACPS_IXR_TXCOMPL_MASK) != 0x00000000U))
  {
    /* Clear TX status register */
    RegSR = In32(XEMACPS_TXSR_OFFSET);
    Out32(XEMACPS_TXSR_OFFSET, RegSR);
    ErrorHandler(XEMACPS_SEND, RegSR);
  }
  
  _irq->unmask();
}

  /*
   * This the Transmit handler callback function and will increment a shared
   * counter that can be shared by the main thread of operation.  
   */
void DevEmacPs::SendHandler()
{
  /*
   * Disable the transmit related interrupts
   */
  intdisable(XEMACPS_IXR_TXCOMPL_MASK | XEMACPS_IXR_TX_ERR_MASK);
  if (Version > 2)
  {
    intQ1Disable(XEMACPS_INTQ1_IXR_ALL_MASK);
  }
  if(observer_ != nullptr)
  {
      observer_->notifySend();
  }
  intenable(XEMACPS_IXR_TXCOMPL_MASK | XEMACPS_IXR_TX_ERR_MASK);
  if (Version > 2)
  {
    intQ1Enable(XEMACPS_INTQ1_IXR_ALL_MASK);
  }
}



void DevEmacPs::RecvHandler()
{
  /*
   * Disable the transmit related interrupts
   */
  intdisable(XEMACPS_IXR_FRAMERX_MASK | XEMACPS_IXR_RX_ERR_MASK);
  printf("DevEmacPs::RecvHandler()\r\n");
  ReadAck();
  if(observer_ != nullptr)
  {
      observer_->notifyReceive();
  }
  intenable(XEMACPS_IXR_FRAMERX_MASK | XEMACPS_IXR_RX_ERR_MASK);/**/
}

void DevEmacPs::ReadAck()
{
    uint32_t RegSR=0;
    /* Clear RX status register */
    RegSR = In32(XEMACPS_RXSR_OFFSET);
    printf("ReadAck RXSR=%x \r\n",RegSR);
    Out32(XEMACPS_RXSR_OFFSET, RegSR);
}

void DevEmacPs::ErrorHandler(uint8_t direction, uint32_t ErrorWord)
{
  if(observer_ != nullptr)
  {
      observer_->notifyError(direction,ErrorWord);
  }
  switch (direction)
  {
  case XEMACPS_RECV:
    if (ErrorWord & XEMACPS_RXSR_HRESPNOK_MASK)
    {
      printf("Receive DMA error\r\n");
    }
    if (ErrorWord & XEMACPS_RXSR_RXOVR_MASK)
    {
      printf("Receive over run\r\n");
    }
    if (ErrorWord & XEMACPS_RXSR_BUFFNA_MASK)
    {
      printf("Receive buffer not available\r\n");
    }
    break;
  case XEMACPS_SEND:
    if (ErrorWord & XEMACPS_TXSR_HRESPNOK_MASK)
    {
      printf("Transmit DMA error\r\n");
    }
    if (ErrorWord & XEMACPS_TXSR_URUN_MASK)
    {
      printf("Transmit under run\r\n");
    }
    if (ErrorWord & XEMACPS_TXSR_BUFEXH_MASK)
    {
      printf("Transmit buffer exhausted\r\n");
    }
    if (ErrorWord & XEMACPS_TXSR_RXOVR_MASK)
    {
      printf("Transmit retry excessed limits\r\n");
    }
    if (ErrorWord & XEMACPS_TXSR_FRAMERX_MASK)
    {
      printf("Transmit collision\r\n");
    }
    if (ErrorWord & XEMACPS_TXSR_USEDREAD_MASK)
    {
      printf("Transmit buffer not available\r\n");
    }
    break;
  }
  /*
   * Bypassing the reset functionality as the default tx status for q0 is
   * USED BIT READ. so, the first interrupt will be tx used bit and it resets
   * the core always.
   */
  if (Version == 2)
  {
    ResetDevice();
  }
}
