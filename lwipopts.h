#ifndef LWIPOPTS_H
#define LWIPOPTS_H

// Pico SDK + lwIP: perusasetuksia
#define NO_SYS                      1
#define LWIP_SOCKET                 0
#define LWIP_NETCONN                0

#define LWIP_TCP                    1
#define LWIP_UDP                    1
#define LWIP_DNS                    1
#define LWIP_DHCP                   1

// Muistit – nämä voi säätää myöhemmin
#define MEM_LIBC_MALLOC             0
#define MEM_SIZE                    32768
#define PBUF_POOL_SIZE              32
#define TCP_MSS                     1460
#define TCP_WND                     (8 * TCP_MSS)
#define TCP_SND_BUF                 (8 * TCP_MSS)
#define MEMP_NUM_TCP_PCB            5
#define MEMP_NUM_TCP_PCB_LISTEN     2
#define MEMP_NUM_TCP_SEG            32

// Debug pois oletuksena
//#define LWIP_DEBUG

#endif