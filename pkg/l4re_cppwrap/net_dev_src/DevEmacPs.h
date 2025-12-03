/*
 * DevEmacPs.h
 *
 *  Created on: Apr 14, 2022
 *      Author: weber
 */

#ifndef SRC_DEVEMACPS_H_
#define SRC_DEVEMACPS_H_
#include "IORegion.h"
#include <l4/sys/types.h>
#include <l4/re/error_helper>
#include <l4/re/util/object_registry>
#include <l4/re/util/br_manager>
#include "xemacps_hw.h"
#include "xemacps_bd.h"
#include "xemacps_bdring.h"
#include "ethernet.h"

// aus BSP xparameter.h
#define XPAR_PSU_ETHERNET_3_IS_CACHE_COHERENT 0
/* Canonical definitions for peripheral PSU_ETHERNET_3 */
#define XPAR_XEMACPS_0_DEVICE_ID XPAR_PSU_ETHERNET_3_DEVICE_ID
#define XPAR_XEMACPS_0_BASEADDR 0xFF0E0000
#define XPAR_XEMACPS_0_HIGHADDR 0xFF0EFFFF
#define XPAR_XEMACPS_0_ENET_CLK_FREQ_HZ 125000000
#define XPAR_XEMACPS_0_ENET_SLCR_1000Mbps_DIV0 12
#define XPAR_XEMACPS_0_ENET_SLCR_1000Mbps_DIV1 1
#define XPAR_XEMACPS_0_ENET_SLCR_100Mbps_DIV0 60
#define XPAR_XEMACPS_0_ENET_SLCR_100Mbps_DIV1 1
#define XPAR_XEMACPS_0_ENET_SLCR_10Mbps_DIV0 60
#define XPAR_XEMACPS_0_ENET_SLCR_10Mbps_DIV1 10
#define XPAR_XEMACPS_0_ENET_TSU_CLK_FREQ_HZ 250000000

#define XEMACPS_PROMISC_OPTION               0x00000001U
/**< Accept all incoming packets.
 *   This option defaults to disabled (cleared) */

#define XEMACPS_FRAME1536_OPTION             0x00000002U
/**< Frame larger than 1516 support for Tx & Rx.
 *   This option defaults to disabled (cleared) */

#define XEMACPS_VLAN_OPTION                  0x00000004U
/**< VLAN Rx & Tx frame support.
 *   This option defaults to disabl,hw_irqed (cleared) */

#define XEMACPS_FLOW_CONTROL_OPTION          0x00000010U
/**< Enable recognition of flow control frames on Rx
 *   This option defaults to enabled (set) */

#define XEMACPS_FCS_STRIP_OPTION             0x00000020U
/**< Strip FCS and PAD from incoming frames. Note: PAD from VLAN frames is not
 *   stripped.
 *   This option defaults to enabled (set) */

#define XEMACPS_FCS_INSERT_OPTION            0x00000040U
/**< Generate FCS field and add PAD automatically for outgoing frames.
 *   This option defaults to disabled (cleared) */

#define XEMACPS_LENTYPE_ERR_OPTION           0x00000080U
/**< Enable Length/Type error checking for incoming frames. When this option is
 *   set, the MAC will filter frames that have a mismatched type/length field
 *   and if XEMACPS_REPORT_RXERR_OPTION is set, the user is notified when these
 *   types of frames are encountered. When this option is cleared, the MAC will
 *   allow these types of frames to be received.
 *
 *   This option defaults to disabled (cleared) */

#define XEMACPS_TRANSMITTER_ENABLE_OPTION    0x00000100U
/**< Enable the transmitter.
 *   This option defaults to enabled (set) */

#define XEMACPS_RECEIVER_ENABLE_OPTION       0x00000200U
/**< Enable the receiver
 *   This option defaults to enabled (set) */

#define XEMACPS_BROADCAST_OPTION             0x00000400U
/**< Allow reception of the broadcast address
 *   This option defaults to enabled (set) */

#define XEMACPS_MULTICAST_OPTION             0x00000800U
/**< Allows reception of multicast addresses programmed into hash
 *   This option defaults to disabled (clear) */

#define XEMACPS_RX_CHKSUM_ENABLE_OPTION      0x00001000U
/**< Enable the RX checksum offload
 *   This option defaults to enabled (set) */

#define XEMACPS_TX_CHKSUM_ENABLE_OPTION      0x00002000U
/**< Enable the TX checksum offload
 *   This option defaults to enabled (set) */

#define XEMACPS_JUMBO_ENABLE_OPTION	0x00004000U
#define XEMACPS_SGMII_ENABLE_OPTION	0x00008000U

#define XEMACPS_DEFAULT_OPTIONS                     \
    ((u32)XEMACPS_FLOW_CONTROL_OPTION |                  \
     (u32)XEMACPS_FCS_INSERT_OPTION |                    \
     (u32)XEMACPS_FCS_STRIP_OPTION |                     \
     (u32)XEMACPS_BROADCAST_OPTION |                     \
     (u32)XEMACPS_LENTYPE_ERR_OPTION |                   \
     (u32)XEMACPS_TRANSMITTER_ENABLE_OPTION |            \
     (u32)XEMACPS_RECEIVER_ENABLE_OPTION |               \
     (u32)XEMACPS_RX_CHKSUM_ENABLE_OPTION |              \
     (u32)XEMACPS_TX_CHKSUM_ENABLE_OPTION)

/* Constants to determine the configuration of the hardware device. They are
 * used to allow the driver to verify it can operate with the hardware.
 */
#define XEMACPS_MDIO_DIV_DFT    MDC_DIV_32 /**< Default MDIO clock divisor */

/* The next few constants help upper layers determine the size of memory
 * pools used for Ethernet buffers and descriptor lists.
 */
#ifndef XEMACPS_MAC_ADDR_SIZE
#define XEMACPS_MAC_ADDR_SIZE   6U	/* size of Ethernet header */
#endif

#define XEMACPS_MTU             1500U	/* max MTU size of Ethernet frame */
#define XEMACPS_MTU_JUMBO       10240U	/* max MTU size of jumbo frame */
#ifndef XEMACPS_HDR_SIZE
#define XEMACPS_HDR_SIZE        14U	/* size of Ethernet header */
#endif
#define XEMACPS_HDR_VLAN_SIZE   18U	/* size of Ethernet header with VLAN */
#define XEMACPS_TRL_SIZE        4U	/* size of Ethernet trailer (FCS) */
#define XEMACPS_MAX_FRAME_SIZE       (XEMACPS_MTU + XEMACPS_HDR_SIZE + \
        XEMACPS_TRL_SIZE)
#define XEMACPS_MAX_VLAN_FRAME_SIZE  (XEMACPS_MTU + XEMACPS_HDR_SIZE + \
        XEMACPS_HDR_VLAN_SIZE + XEMACPS_TRL_SIZE)
#define XEMACPS_MAX_VLAN_FRAME_SIZE_JUMBO  (XEMACPS_MTU_JUMBO + XEMACPS_HDR_SIZE + \
        XEMACPS_HDR_VLAN_SIZE + XEMACPS_TRL_SIZE)

/* DMACR Bust length hash defines */

#define XEMACPS_SINGLE_BURST	0x00000001
#define XEMACPS_4BYTE_BURST		0x00000004
#define XEMACPS_8BYTE_BURST		0x00000008
#define XEMACPS_16BYTE_BURST	0x00000010



class DevObserver
{
public:
    virtual void notifySend() = 0;
    virtual void notifyReceive() = 0;
    virtual void notifyError(uint8_t direction, uint32_t errorword) = 0;
};

class DevEmacPs : public L4::Irqep_t<DevEmacPs>, public IORegion
{
public:
  enum Direction_t
  {
    Send = 1U,
    Recv = 2U
  };
  DevEmacPs();
  virtual ~DevEmacPs();
  void  initmem(l4_addr_t phyaddr_start, l4_addr_t phyaddr_end);
  void  set_irq(int hw_irq, L4::Cap < L4::Irq > irq, L4_irq_mode irqmode)
  {
    _irq = irq;
    L4Re::chksys(_icu->set_mode(hw_irq, irqmode), "Set IRQ mode");
    // IRQ an den interrupt binden
    L4Re::chksys(_icu->bind(hw_irq, _irq));
    printf("icu bind(%d)\n\r",hw_irq);
  }

  void unmask()
  {
    printf("irq unmask\n\r");
    L4Re::chkipc(_irq->unmask(), "umask IRQ");
  }

  uint32_t GemVersion() { return ((In32(0xFC)) >> 16) & 0xFFF; }
  /* get cached Version */
  uint32_t getGemVersion() { return Version; }

  void SetQueuePtr(UINTPTR Qptr, uint8_t QueueNum, Direction_t Direction);
	/**
	*
	* Enable interrupts specified in <i>Mask</i>. The corresponding interrupt for
	* each bit set to 1 in <i>Mask</i>, will be enabled.
	*
	* @param InstancePtr is a pointer to the instance to be worked on.
	* @param Mask contains a bit mask of interrupts to enable. The mask can
	*        be formed using a set of bitwise or'd values.
	*
	* @note
	* The state of the transmitter and receiver are not modified by this function.
	* C-style signature
	*     void XEmacPs_IntEnable(XEmacPs *InstancePtr, u32 Mask)
	*
	*****************************************************************************/
  void intenable(uint32_t mask) { printf("intenable(%x)\n",mask); Out32(XEMACPS_IER_OFFSET, ((mask) & XEMACPS_IXR_ALL_MASK)); }

	/**
	*
	* Disable interrupts specified in <i>Mask</i>. The corresponding interrupt for
	* each bit set to 1 in <i>Mask</i>, will be enabled.
	*
	* @param Mask contains a bit mask of interrupts to disable. The mask can
	*        be formed using a set of bitwise or'd values.
	*
	* @note
	* The state of the transmitter and receiver are not modified by this function.
	*****************************************************************************/
  void intdisable(uint32_t mask) { printf("intdisable(%x)\n",mask); Out32(XEMACPS_IDR_OFFSET, mask & XEMACPS_IXR_ALL_MASK); }
	/**
	*
	* Enable interrupts specified in <i>Mask</i>. The corresponding interrupt for
	* each bit set to 1 in <i>Mask</i>, will be enabled.
	*
	* @param InstancePtr is a pointer to the instance to be worked on.
	* @param Mask contains a bit mask of interrupts to enable. The mask can
	*        be formed using a set of bitwise or'd values.
	*
	* @note
	* The state of the transmitter and receiver are not modified by this function.
	*****************************************************************************/
  void intQ1Enable(uint32_t Mask) { printf("intQ1Enable(%x)\n",Mask); Out32(XEMACPS_INTQ1_IER_OFFSET, ((Mask) & XEMACPS_INTQ1_IXR_ALL_MASK)); }
	/**
	*
	* Enable interrupts specified in <i>Mask</i>. The corresponding interrupt for
	* each bit set to 1 in <i>Mask</i>, will be enabled.
	*
	* @param InstancePtr is a pointer to the instance to be worked on.
	* @param Mask contains a bit mask of interrupts to enable. The mask can
	*        be formed using a set of bitwise or'd values.
	*
	* @note
	* The state of the transmitter and receiver are not modified by this function.
	*****************************************************************************/
  void intQ1Disable(uint32_t mask) { printf("intQ1Disable(%x)\n",mask); Out32(XEMACPS_INTQ1_IER_OFFSET, mask & XEMACPS_INTQ1_IXR_ALL_MASK); }

  void StartDevice();
  /* Stop the device and reset hardware */
  void StopDevice();
  void ResetDevice();
  void ClearHash();
  void GetMacAddress(void *AddressPtr, uint8_t Index);
  long SetMacAddress(void *AddressPtr, uint8_t Index);
  long SetTypeIdCheck(uint32_t Id_Check, uint8_t Index);
  long SetOptions(uint32_t Options);
  long ClearOptions(uint32_t Options);
  void SetMdioDivisor(XEmacPs_MdcDiv Divisor);
  void SetOperatingSpeed(uint16_t Speed);

  long EnterLoopback(uint32_t Speed);
  long MarvellPhyLoopback(uint32_t Speed, uint32_t PhyAddr);
  long TiPhyLoopback(uint32_t Speed, uint32_t PhyAddr);
  uint32_t DetectPHY();
  long PhyRead(uint32_t PhyAddress, uint32_t RegisterNum, uint16_t * PhyDataPtr);
  long PhyWrite(uint32_t PhyAddress, uint32_t RegisterNum, uint16_t PhyData);

  void SetRXQ1Base(UINTPTR ptr) { Out32(XEMACPS_RXQ1BASE_OFFSET, ptr); }
  void SetTXQbase(UINTPTR ptr) { Out32(XEMACPS_TXQBASE_OFFSET, ptr); }

  void  Transmit();

  /* This is the main InterruptHandling */
  virtual void handle_irq();
  void SendHandler();
  void RecvHandler();
    /**
    *
    * This is the Error handler callback function and this function increments
    * the error counter so that the main thread knows the number of errors.
    *
    * @param	Direction is passed in from the driver specifying which
    *		direction error has occurred.
    * @param	ErrorWord is the status register value passed in.
    *
    * @return	None.
    *
    * @note		None.
    *
    *****************************************************************************/
  void  ErrorHandler(uint8_t direction, uint32_t errorword);
  /**
   * Read and Write the RXStatusRegister for Acknolege
   */
  void ReadAck();
  uint32_t GetRxFrameSize(XEmacPs_Bd * BdPtr) { return (XEmacPs_BdRead((BdPtr), XEMACPS_BD_STAT_OFFSET) & RxBufMask); }

  uint32_t getInt_status() { return In32_silent(0x24); }
  void registerObserver(DevObserver *obs) { observer_=obs; }
  const uint8_t *getPhysaddr() { return physaddr_; }
  uint32_t get_IEEE_phy_speed(uint32_t phy_addr);
  void dumpRegisters();
  XEmacPs_BdRing    TxBdRing;			/* Transmit BD ring */
  XEmacPs_BdRing    RxBdRing;			/* Receive BD ring */

private:
  uint32_t get_TI_phy_speed(uint32_t phy_addr);
  uint32_t get_Marvell_phy_speed(uint32_t phy_addr);
  uint32_t get_Realtek_phy_speed(uint32_t phy_addr);
  uint32_t get_Adi_phy_speed(uint32_t phy_addr);
  uint32_t get_Xilinx_pcs_pma_phy_speed(uint32_t phy_addr);

  /* ------ State ------ */
  uint32_t    isStarted_;
  uint32_t    isReady_;

  uint32_t    options_;
  uint32_t    Version;
  uint32_t    RxBufMask;
  uint32_t    MaxMtuSize;
  uint32_t    MaxFrameSize;
  uint32_t    MaxVlanFrameSize;

  L4::Cap < L4::Icu > _icu;
  L4::Cap < L4::Irq > _irq;

  DevObserver *observer_;
};

#endif /* SRC_DEVEMACPS_H_ */
