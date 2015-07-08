/*
 * Copyright (c) 2008 Xilinx, Inc.  All rights reserved.
 *
 * Xilinx, Inc.
 * XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS" AS A
 * COURTESY TO YOU.  BY PROVIDING THIS DESIGN, CODE, OR INFORMATION AS
 * ONE POSSIBLE   IMPLEMENTATION OF THIS FEATURE, APPLICATION OR
 * STANDARD, XILINX IS MAKING NO REPRESENTATION THAT THIS IMPLEMENTATION
 * IS FREE FROM ANY CLAIMS OF INFRINGEMENT, AND YOU ARE RESPONSIBLE
 * FOR OBTAINING ANY RIGHTS YOU MAY REQUIRE FOR YOUR IMPLEMENTATION.
 * XILINX EXPRESSLY DISCLAIMS ANY WARRANTY WHATSOEVER WITH RESPECT TO
 * THE ADEQUACY OF THE IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO
 * ANY WARRANTIES OR REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE
 * FROM CLAIMS OF INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#include <stdio.h>
#include <string.h>

#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwipopts.h"
#if __MICROBLAZE__ || __PPC__
#include "sys/timer.h"
#endif

#include "netif/etharp.h"
#include "localNetworkConfig.h"

void print_udpsend_app_header()
{
  xil_printf("%20s to %d.%d.%d.%d\r\n", "udpsend", udpSendDest[0], udpSendDest[1], udpSendDest[2], udpSendDest[3]);
}

void udpsend_thread(void *p)
{
    int sd;
    struct sockaddr_in server;
    struct sockaddr_in to;
    int BUFSIZE = 8192;
    char buf[BUFSIZE];
    struct ip_addr to_ipaddr;
    int n, i;

    sleep(10000);

    /* create a new socket to send responses to this client */
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd < 0) {
        xil_printf("%s: error creating socket, return value = %d\r\n", __FUNCTION__, sd);
        return ;
    }

    memset(&server, 0, sizeof server);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = 9130;
    if (lwip_bind(sd, (struct sockaddr *)&server, sizeof server) < 0)  {
        printf("error binding");
        return ;
    }

    IP4_ADDR(&to_ipaddr,  udpSendDest[0], udpSendDest[1], udpSendDest[2], udpSendDest[3]);

    if( udpSendDest[0] == 129 && udpSendDest[1] == 49 ) {
      xil_printf("udpsend: Configuring at SBU\r\n");
      IP4_ADDR(&to_ipaddr, 129, 49, 60, 224 );  /* Add SBPCJH to ARP table */
      struct eth_addr eth_sbpcjh = {{0x00,0x1E,0x4F,0x51,0xED,0x87}};
      //etharp_add_static_entry(&to_ipaddr,&eth_sbpcjh);
      struct eth_addr to_ethaddr = {{0x00,0x26,0xB9,0xDE,0x85,0x85}};
      //etharp_add_static_entry(&to_ipaddr,&to_ethaddr);
    }

    memset(&to, 0, sizeof to);
    to.sin_family = AF_INET;
    to.sin_addr.s_addr = to_ipaddr.addr;
    to.sin_port = htons(9123);

    memset(buf, 0, sizeof buf);
    strcpy(buf,"heartbeat 0");

    /* send one packet to create ARP entry */
    lwip_sendto(sd, buf, strlen(buf), 0, (struct sockaddr *)&to, sizeof(to) );

    /* wait until receive'd arp entry updates ARP cache */
    sleep(20);
    xil_printf("Awakw\n");

#if 1
    /* now send real packets */
    int npackets = -1;
    int delaymillisecs = 1000;
    /* Choose an interval for sending info to the console */
    int divisor = 10000;
    if( delaymillisecs>0 ) divisor = divisor/delaymillisecs;
    if( divisor==0 ) divisor=1;
    for (i = 0; npackets<0 || i<npackets; i++) {
      sprintf( buf , "heartbead %d" , i+1 );
      n = lwip_sendto(sd, buf, strlen(buf), 0, (struct sockaddr *)&to, sizeof(to) );
      if( i<10 ) xil_printf("packet %d, sent bytes = %d\r\n",i, n);
      else if( i%divisor==0 ) xil_printf("packet %d...\r\n",i);
      if( delaymillisecs>0 ) sleep(delaymillisecs);
    }
#endif
}
