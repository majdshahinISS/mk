#ifndef ETHERNET_H
#define ETHERNET_H
/* Ethernet packet typs */
#define NX_ETHERNET_IP   0x0800
#define NX_ETHERNET_ARP  0x0806
#define NX_ETHERNET_RARP 0x8035
#define NX_ETHERNET_IPV6 0x86DD
#define NX_ETHERNET_HOMEPLUG 0x88E1
typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
#define XEMACPS_MAC_ADDR_SIZE   6U  /* size of Ethernet address */
struct EthernetHeader
{
   uint8_t destaddr[XEMACPS_MAC_ADDR_SIZE];
   uint8_t srcaddr[XEMACPS_MAC_ADDR_SIZE];
   uint16_t type;
};
#define XEMACPS_HDR_SIZE 14
#endif
