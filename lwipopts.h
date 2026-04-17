#ifndef _LWIPOPTS_EXAMPLE_COMMONH_H
#define _LWIPOPTS_EXAMPLE_COMMONH_H

// Common settings used in most of the pico_w examples
// (see https://www.nongnu.org/lwip/2_1_x/group__lwip__opts.html for details)

// Use lwIP without OS (bare-metal mode)
#ifndef NO_SYS
#define NO_SYS                      1
#endif

// Disable BSD socket API (use raw API instead)
#ifndef LWIP_SOCKET
#define LWIP_SOCKET                 0
#endif

#if PICO_CYW43_ARCH_POLL
// Use standard libc malloc in polling mode
#define MEM_LIBC_MALLOC             1
#else
// Disable libc malloc for non-polling (threaded/IRQ) mode
#define MEM_LIBC_MALLOC             0
#endif

// Memory alignment (4 bytes for ARM)
#define MEM_ALIGNMENT 4

// Heap size for lwIP dynamic allocations (bytes)
#ifndef MEM_SIZE
#define MEM_SIZE                    4000
#endif

// Number of TCP segments in memory pool
#define MEMP_NUM_TCP_SEG            32

// ARP queue length (pending packets waiting for resolution)
#define MEMP_NUM_ARP_QUEUE          10

// Number of pbufs in pool (packet buffers)
#define PBUF_POOL_SIZE              24

// Enable ARP protocol
#define LWIP_ARP                    1

// Enable Ethernet support
#define LWIP_ETHERNET               1

// Enable ICMP (e.g., ping)
#define LWIP_ICMP                   1

// Enable raw API (low-level access)
#define LWIP_RAW                    1

// TCP receive window size
#define TCP_WND                     (8 * TCP_MSS)

// Maximum TCP segment size (bytes)
#define TCP_MSS                     1460

// TCP send buffer size
#define TCP_SND_BUF                 (8 * TCP_MSS)

// TCP send queue length (derived from buffer size)
#define TCP_SND_QUEUELEN            ((4 * (TCP_SND_BUF) + (TCP_MSS - 1)) / (TCP_MSS))

// Enable network interface status callback
#define LWIP_NETIF_STATUS_CALLBACK  1

// Enable link status callback (up/down events)
#define LWIP_NETIF_LINK_CALLBACK    1

// Enable hostname support
#define LWIP_NETIF_HOSTNAME         1

// Disable netconn API (higher-level API)
#define LWIP_NETCONN                0

// Disable memory and system statistics (saves RAM)
#define MEM_STATS                   0
#define SYS_STATS                   0
#define MEMP_STATS                  0
#define LINK_STATS                  0
// Optional Ethernet padding (disabled)
// #define ETH_PAD_SIZE                2

// Checksum algorithm selection (platform optimized)
#define LWIP_CHKSUM_ALGORITHM       3

// Enable DHCP client
#define LWIP_DHCP                   1

// Enable IPv4 stack
#define LWIP_IPV4                   1

// Enable TCP protocol
#define LWIP_TCP                    1

// Enable UDP protocol
#define LWIP_UDP                    1

// Enable DNS client
#define LWIP_DNS                    1

// Enable TCP keepalive mechanism
#define LWIP_TCP_KEEPALIVE          1

// Use single pbuf for TX (reduces fragmentation)
#define LWIP_NETIF_TX_SINGLE_PBUF   1

// Disable ARP check during DHCP (faster but less safe)
#define DHCP_DOES_ARP_CHECK         0

// Disable Address Conflict Detection (ACD)
#define LWIP_DHCP_DOES_ACD_CHECK    0

#ifndef NDEBUG
// Enable debugging and statistics in debug builds
#define LWIP_DEBUG                  1
#define LWIP_STATS                  1
#define LWIP_STATS_DISPLAY          1
#endif

// Disable all module-specific debug outputs
#define ETHARP_DEBUG                LWIP_DBG_OFF
#define NETIF_DEBUG                 LWIP_DBG_OFF
#define PBUF_DEBUG                  LWIP_DBG_OFF
#define API_LIB_DEBUG               LWIP_DBG_OFF
#define API_MSG_DEBUG               LWIP_DBG_OFF
#define SOCKETS_DEBUG               LWIP_DBG_OFF
#define ICMP_DEBUG                  LWIP_DBG_OFF
#define INET_DEBUG                  LWIP_DBG_OFF
#define IP_DEBUG                    LWIP_DBG_OFF
#define IP_REASS_DEBUG              LWIP_DBG_OFF
#define RAW_DEBUG                   LWIP_DBG_OFF
#define MEM_DEBUG                   LWIP_DBG_OFF
#define MEMP_DEBUG                  LWIP_DBG_OFF
#define SYS_DEBUG                   LWIP_DBG_OFF
#define TCP_DEBUG                   LWIP_DBG_OFF
#define TCP_INPUT_DEBUG             LWIP_DBG_OFF
#define TCP_OUTPUT_DEBUG            LWIP_DBG_OFF
#define TCP_RTO_DEBUG               LWIP_DBG_OFF
#define TCP_CWND_DEBUG              LWIP_DBG_OFF
#define TCP_WND_DEBUG               LWIP_DBG_OFF
#define TCP_FR_DEBUG                LWIP_DBG_OFF
#define TCP_QLEN_DEBUG              LWIP_DBG_OFF
#define TCP_RST_DEBUG               LWIP_DBG_OFF
#define UDP_DEBUG                   LWIP_DBG_OFF
#define TCPIP_DEBUG                 LWIP_DBG_OFF
#define PPP_DEBUG                   LWIP_DBG_OFF
#define SLIP_DEBUG                  LWIP_DBG_OFF
#define DHCP_DEBUG                  LWIP_DBG_OFF

// Enable MQTT protocol support
#define LWIP_MQTT 1

// Number of system timeouts (timers)
#define MEMP_NUM_SYS_TIMEOUT 32
#endif /* __LWIPOPTS_H__ */