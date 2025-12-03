#ifndef XEMACPSIF_H
#define XEMACPSIF_H
// #include "nx_packet.h"
#include <pthread.h>
#include "EVBuffer.h"

typedef int err_t;
typedef unsigned int UINT;
#ifndef __cplusplus
err_t xemacps_init();
#else
extern "C" {
err_t xemacps_init();
}
#include "xil_types.h" 
#include "CRLARB.h"
#include "DevEmacPs.h"
#define NX_SUCCESS 0
const uint32_t JUMBO_FRAME_SIZE = 10240;
const uint32_t FRAME_HDR_SIZE = 18;
const uint32_t RXBD_CNT = 32; /*  Number of RxBDs to use */

enum ethernet_link_status           // xadapter.h:62
{
	ETH_LINK_UNDEFINED = 0,
	ETH_LINK_UP,
	ETH_LINK_DOWN,
	ETH_LINK_NEGOTIATING
};

static const size_t XEMACPSIF_MAX_FRAME_SIZE = 1536;

struct EmacRawFrame
{
    uint8_t data[XEMACPSIF_MAX_FRAME_SIZE];
    uint32_t len;   // number of valid bytes in data[]
};

class Xemacpsif : public DevObserver
{
public:
   virtual ~Xemacpsif();
   Xemacpsif();
   void init(UCHAR *macaddr);
   void setPhyLoopback();
   void initNormal();
   void startDevice();
   void stopDevice();

   // NX_PACKET *receive();//
   EmacRawFrame *receive(); // REVIEW 
   void releaseReceivedFrame(EmacRawFrame *frame);// must be called after smoltcp finishes processing a received packet.
   // int send(NX_PACKET *ts);
   int send(const uint8_t *data, uint32_t len); // TODO

   /* Observer */
   void notifySend() override;
   void notifyReceive() override;
   void notifyError(uint8_t direction, uint32_t errorword) override;

   uint32_t magic_ = 0x5137434D; // 'Q1CM'
   private:
   int allocResourcesFromIo();
   int allocDMASpaces();
   void initializeDevice();
   int setMacAddr(UCHAR *macaddr);
   LONG createRXRing();
   LONG createTXRing();
   void setPriorityBuffers();

   int prepareReceive();

  /*
   * Detect phy
   * returns link_speed
   */
   uint32_t detect_phy();
   void phy_identify(uint32_t phy_addr, uint32_t emacnum);
   // return Linkspeed
   uint32_t phy_setup_emacps(uint32_t phy_addr);

   uint8_t *RxBdSpacePtrVirt;
   uint8_t *TxBdSpacePtrVirt;
   uint8_t *RxBdSpacePtrPhys;
   uint8_t *TxBdSpacePtrPhys;
   uint8_t *BdRxTerminatePtrVirt;
   uint8_t *BdRxTerminatePtrPhys;
   uint8_t *BdTxTerminatePtrVirt;
   uint8_t *BdTxTerminatePtrPhys;
   DevEmacPs etherdev;
   CRL_ARB crl_arb;
   uint32_t    phymapemac0[32];
   uint32_t    phymapemac1[32];
   ethernet_link_status eth_link_status_;
   pthread_mutex_t mutex_;
   pthread_cond_t  signalrecv_;
   pthread_cond_t  signalsendfinished_;

   //EVBuffer<NX_PACKET *,RXBD_CNT> recvqueue_;  
   // NX_PACKET *rx_buffer_correspond[RXBD_CNT];
   EVBuffer<EmacRawFrame *, RXBD_CNT> recvqueue_;
   EmacRawFrame rx_frames_[RXBD_CNT]; 
};
#endif
#endif
