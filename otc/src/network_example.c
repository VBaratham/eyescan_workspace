/*
 * Copyright (c) 2007-2009 Xilinx, Inc.  All rights reserved.
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
#if __MICROBLAZE__ || __PPC__
#include "xmk.h"
#include "sys/timer.h"
#include "xenv_standalone.h"
#endif
#include "xparameters.h"
#include "lwipopts.h"

#include "platform_config.h"
#include "platform.h"

#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/init.h"
#include "netif/xadapter.h"
#include "netif/xaxiemacif.h"  /* JDH: Added this */
#include "lwip/dhcp.h"
#include "config_apps.h"
#if __arm__
#include "task.h"
#include "portmacro.h"
#include "xil_printf.h"
void* network_main_thread(void *args);
#endif

/* JDH */
#include "xparameters.h"

#include "localNetworkConfig.h"
#include "SysStatus.h"

#define IS_OTC_BOARD

#ifndef IS_OTC_BOARD
#define LWIP_DHCP 1
#endif

void print_headers();
void launch_app_threads();

int verbosity=1;

void print_ip(char *msg, struct ip_addr *ip)
{
    print(msg);
    xil_printf("%d.%d.%d.%d\r\n", ip4_addr1(ip), ip4_addr2(ip),
            ip4_addr3(ip), ip4_addr4(ip));
}

void print_ip_settings(struct ip_addr *ip, struct ip_addr *mask, struct ip_addr *gw)
{

    print_ip("Board IP: ", ip);
    print_ip("Netmask : ", mask);
    print_ip("Gateway : ", gw);
}

struct netif server_netif;

void get_temac_info(XAxiEthernet *enetraw ) {
  ethStatus.regVal[XAE_IDREG_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_IDREG_OFFSET);
  ethStatus.regVal[XAE_ARREG_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_ARREG_OFFSET);
  ethStatus.regVal[XAE_FMI_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_FMI_OFFSET);
  ethStatus.regVal[XAE_RAF_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_RAF_OFFSET);
  ethStatus.regVal[XAE_UAW0_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_UAW0_OFFSET);
  ethStatus.regVal[XAE_UAWL_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_UAWL_OFFSET);
  ethStatus.regVal[XAE_EMMC_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_EMMC_OFFSET);
  ethStatus.regVal[XAE_PHYC_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_PHYC_OFFSET);
  ethStatus.regVal[XAE_RCW1_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_RCW1_OFFSET);
  ethStatus.regVal[XAE_TC_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_TC_OFFSET);
  ethStatus.regVal[XAE_IE_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_IE_OFFSET);
  ethStatus.regVal[XAE_IS_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_IS_OFFSET);
  ethStatus.regVal[XAE_IP_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_IP_OFFSET);
  ethStatus.regVal[0x30] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,0x30);
  ethStatus.regVal[XAE_TXBL_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_TXBL_OFFSET);
  ethStatus.regVal[XAE_TXBL_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_TXBU_OFFSET);
  ethStatus.regVal[XAE_RXBL_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_RXBL_OFFSET);
  ethStatus.regVal[XAE_RXBL_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_RXBU_OFFSET);
  ethStatus.regVal[XAE_RXUNDRL_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_RXUNDRL_OFFSET);
  ethStatus.regVal[XAE_RXFRAGL_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_RXFRAGL_OFFSET);
  ethStatus.regVal[XAE_RX64BL_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_RX64BL_OFFSET);
  ethStatus.regVal[XAE_RX65B127L_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_RX65B127L_OFFSET);
  ethStatus.regVal[XAE_RX128B255L_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_RX128B255L_OFFSET);
  ethStatus.regVal[XAE_RX256B511L_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_RX256B511L_OFFSET);
  ethStatus.regVal[XAE_RX512B1023L_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_RX512B1023L_OFFSET);
  ethStatus.regVal[XAE_RX1024BL_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_RX1024BL_OFFSET);
  ethStatus.regVal[XAE_RXOVRL_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_RXOVRL_OFFSET);
  ethStatus.regVal[XAE_RX65B127L_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_RX65B127L_OFFSET);
  ethStatus.regVal[XAE_RXFL_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_RXFL_OFFSET);
  ethStatus.regVal[XAE_RXFCSERL_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_RXFCSERL_OFFSET);
  ethStatus.regVal[XAE_RXBCSTFL_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_RXBCSTFL_OFFSET);
  ethStatus.regVal[XAE_RXMCSTFL_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_RXMCSTFL_OFFSET);
  ethStatus.regVal[XAE_RXCTRFL_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_RXCTRFL_OFFSET);
  ethStatus.regVal[XAE_RXLTERL_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_RXLTERL_OFFSET);
  ethStatus.regVal[XAE_RXVLANFL_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_RXVLANFL_OFFSET);
  ethStatus.regVal[XAE_RXPFL_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_RXPFL_OFFSET);
  ethStatus.regVal[XAE_RXUOPFL_OFFSET] = XAxiEthernet_ReadReg(enetraw->Config.BaseAddress,XAE_RXUOPFL_OFFSET);
}

void print_temac_info() {
    xil_printf("Identification register = %08x\n",ethStatus.regVal[XAE_IDREG_OFFSET]);
    xil_printf("ability (AR) register = %08x\n",ethStatus.regVal[XAE_ARREG_OFFSET]);
    xil_printf("filter mask (FMI) register = %08x\n",ethStatus.regVal[XAE_FMI_OFFSET]);
    xil_printf("reset and address filter (RAF) register = %08x\n",ethStatus.regVal[XAE_RAF_OFFSET]);
    xil_printf("Lower MAC address (UAW0) = %08x\n",ethStatus.regVal[XAE_UAW0_OFFSET]);
    xil_printf("Unicast Lower MAC address (UAWL) = %08x\n",ethStatus.regVal[XAE_UAWL_OFFSET]);
    xil_printf("EMAC configuration (EMMC) register = %08x\n",ethStatus.regVal[XAE_EMMC_OFFSET]);
    xil_printf("SGMII configuration (PHYC) register = %08x\n",ethStatus.regVal[XAE_PHYC_OFFSET]);
    xil_printf("Receive configuration (RCW1) register 1= %08x\n",ethStatus.regVal[XAE_RCW1_OFFSET]);
    xil_printf("Transmit configuration (TC) register = %08x\n",ethStatus.regVal[XAE_TC_OFFSET]);
    xil_printf("interrupt enable (IE) register = %08x\n",ethStatus.regVal[XAE_IE_OFFSET]);
    xil_printf("interrupt status (IS) register = %08x\n",ethStatus.regVal[XAE_IS_OFFSET]);
    xil_printf("interrupt pending (IP) register = %08x\n",ethStatus.regVal[XAE_IP_OFFSET]);
    xil_printf("PCS status (PPST) register = %08x\n",ethStatus.regVal[0x30]);
    xil_printf("TX packet count = %d\n",ethStatus.regVal[XAE_TXBL_OFFSET]);
    xil_printf("RX packet count = %d\n",ethStatus.regVal[XAE_RXBL_OFFSET]);
    xil_printf("RX undersize frame count = %d\n",ethStatus.regVal[XAE_RXUNDRL_OFFSET]);
    xil_printf("RX fragment frames count = %d\n",ethStatus.regVal[XAE_RXFRAGL_OFFSET]);
    xil_printf("RX 64 byte frame count = %d\n",ethStatus.regVal[XAE_RX64BL_OFFSET]);
    xil_printf("RX 65 to 127 byte frame count = %d\n",ethStatus.regVal[XAE_RX65B127L_OFFSET]);
    xil_printf("RX 128 to 255 byte frame count = %d\n",ethStatus.regVal[XAE_RX128B255L_OFFSET]);
    xil_printf("RX 256 to 511 byte frame count = %d\n",ethStatus.regVal[XAE_RX256B511L_OFFSET]);
    xil_printf("RX 512 to 1023 byte frame count = %d\n",ethStatus.regVal[XAE_RX512B1023L_OFFSET]);
    xil_printf("RX >= 1024 byte frame count = %d\n",ethStatus.regVal[XAE_RX1024BL_OFFSET]);
    xil_printf("RX oversize frame count = %d\n",ethStatus.regVal[XAE_RXOVRL_OFFSET]);
    xil_printf("RX 65 to 127 byte frame count = %d\n",ethStatus.regVal[XAE_RX65B127L_OFFSET]);
    xil_printf("RX OK frame count = %d\n",ethStatus.regVal[XAE_RXFL_OFFSET]);
    xil_printf("RX frame check sequenc errors count = %d\n",ethStatus.regVal[XAE_RXFCSERL_OFFSET]);
    xil_printf("RX OK broadcast frame count = %d\n",ethStatus.regVal[XAE_RXBCSTFL_OFFSET]);
    xil_printf("RX OK multicast frame count = %d\n",ethStatus.regVal[XAE_RXMCSTFL_OFFSET]);
    xil_printf("RX OK control frame count = %d\n",ethStatus.regVal[XAE_RXCTRFL_OFFSET]);
    xil_printf("RX length/type error frame count = %d\n",ethStatus.regVal[XAE_RXLTERL_OFFSET]);
    xil_printf("RX VLAN frame count = %d\n",ethStatus.regVal[XAE_RXVLANFL_OFFSET]);
    xil_printf("RX pause frame count = %d\n",ethStatus.regVal[XAE_RXPFL_OFFSET]);
    xil_printf("RX control frame bad opcode count = %d\n",ethStatus.regVal[XAE_RXUOPFL_OFFSET]);
}

void print_phy_info(XAxiEthernet *enetraw, int iphy, int nreg) {
    u16 phyreg,ireg;
    for( ireg=0 ; ireg<nreg ; ireg++ ) {
      XAxiEthernet_PhyRead(enetraw,iphy,ireg,&phyreg);
      xil_printf("PHY register %d = 0x%04x\n",ireg,phyreg);
    }
}

void* network_thread(void *p)
{
    struct netif *netif;
    struct ip_addr ipaddr, netmask, gw;
    u16 phyreg;
#if LWIP_DHCP==1
    int mscnt = 0;
#endif
    xil_printf("In network_thread\n");
    /* the mac address of the board. this should be unique per board */
    //unsigned char mac_ethernet_address[] = { 0x00, 0x0a, 0x35, 0x00, 0x01, 0x02 };
    unsigned char mac_ethernet_address[] = { myMac[0], myMac[1], myMac[2], myMac[3], myMac[4], myMac[5] };

    netif = &server_netif;

#if LWIP_DHCP==0
    /* initliaze IP addresses to be used. */
    IP4_ADDR(&ipaddr,  myIP[0], myIP[1], myIP[2], myIP[3]);
    IP4_ADDR(&netmask, myMask[0], myMask[1], myMask[2], myMask[3]);
    IP4_ADDR(&gw,      myGateway[0], myGateway[1], myGateway[2], myGateway[3]);
#endif

    /* print out IP settings of the board */
    print("\r\n\r\n");
    print("-----lwIP Socket Mode Demo Application ------\r\n");

#if LWIP_DHCP==0
    print_ip_settings(&ipaddr, &netmask, &gw);
    /* print all application headers */
#endif

#if LWIP_DHCP==1
    ipaddr.addr = 0;
    gw.addr = 0;
    netmask.addr = 0;
#endif
    /* Add network interface to the netif_list, and set it as default */
    if (!xemac_add(netif, &ipaddr, &netmask, &gw, mac_ethernet_address, PLATFORM_EMAC_BASEADDR)) {
        xil_printf("Error adding N/W interface\r\n");
        return 0;
    }
    netif_set_default(netif);

    /* specify that the network if is up */
    netif_set_up(netif);

    /* SBU/Hobbs/Schamberger: CRITICAL. Try setting the EMMC register... */
    u32 regval = 0x50000000;
    struct xemac_s *emac = (struct xemac_s *)server_netif.state;
    if( !emac ) { xil_printf("No emac available. Done\n"); return 0; }
    xaxiemacif_s *emacif = (xaxiemacif_s *)(emac->state);
    if( !emacif ) { xil_printf("No emacif available. Done\n"); return 0; }
    XAxiEthernet *enetraw = &(emacif->axi_ethernet);

#ifndef IS_OTC_BOARD
    unsigned readspeed = get_IEEE_phy_speed(enetraw);
    xil_printf("Read speed = %d\n",readspeed);

    /* Determine the speed setting for the EMMC register */
    u32 speedval = 0x00000000;
    u32 regmask =  0xD0000000;
    if( readspeed == 10 ) speedval = 0x10000000;
    else if( readspeed == 100 ) speedval = 0x50000000;
    else if( readspeed == 1000 ) speedval = 0x90000000;

    /* Set up the EMMC register */
    regval = XAxiEthernet_ReadReg(XPAR_AXIETHERNET_0_BASEADDR,XAE_EMMC_OFFSET);
    regval &= ~regmask;
    regval |= speedval;
#endif
    unsigned readspeed = get_IEEE_phy_speed(enetraw);
    xil_printf("Read speed = %d\n",readspeed);
    readspeed = XAxiEthernet_GetOperatingSpeed(enetraw);
    xil_printf("Read speed again = %d\n",readspeed);

    XAxiEthernet_WriteReg(XPAR_AXIETHERNET_0_BASEADDR,XAE_EMMC_OFFSET,regval);
#ifdef IS_OTC_BOARD
    u32 phy_wr_data = 0x1040;
    XAxiEthernet_PhyWrite(enetraw,0,0,phy_wr_data);
#else
    xil_printf("\nModified? EMMC content: 0x%08x\n",XAxiEthernet_ReadReg(XPAR_AXIETHERNET_0_BASEADDR,XAE_EMMC_OFFSET));
#endif
    /* End SBU/Hobbs */

    /* start packet receive thread - required for lwIP operation */
    sys_thread_new("xemacif_input_thread", (void(*)(void*))xemacif_input_thread, netif,
            THREAD_STACKSIZE,
            DEFAULT_THREAD_PRIO);

#if LWIP_DHCP==1
    dhcp_start(netif);
    while (1) {
#ifdef OS_IS_FREERTOS
        vTaskDelay(DHCP_FINE_TIMER_MSECS / portTICK_RATE_MS);
#else
        sleep(DHCP_FINE_TIMER_MSECS);
#endif
        dhcp_fine_tmr();
        mscnt += DHCP_FINE_TIMER_MSECS;
        if (mscnt >= DHCP_COARSE_TIMER_SECS*1000) {
            dhcp_coarse_tmr();
            mscnt = 0;
        }
    }
#else
    print_headers();
    launch_app_threads();
#ifdef OS_IS_FREERTOS
    vTaskDelete(NULL);
#endif
#endif

#ifdef IS_OTC_BOARD
    sleep(2500);
#if XPAR_AXIETHERNET_0_CONNECTED_TYPE == XPAR_AXI_DMA
    enable_interrupt(emacif->axi_ethernet.Config.AxiDmaRxIntr);
#else
    enable_interrupt(emacif->axi_ethernet.Config.AxiFifoIntr);
#endif
#endif

    /* JDH: Add infinite loop for some debugging info to be gathered/printed */
    int doForever=1,nloops=0;
    sleep(5000);
    xil_printf("Address: %08x\n",enetraw);
    while( doForever || (nloops-- > 0) ) {
        get_temac_info(enetraw);
        if( verbosity>1 ) {
            xil_printf("---------------------------------------------------------------\n");
            xil_printf("TEMAC information\n");
            print_temac_info();
            xil_printf("\nMarvell PHY information\n");
            print_phy_info(enetraw,7,16);  /* Marvell */
            xil_printf("\nInternal PHY information\n");
            print_phy_info(enetraw,1,18);  /* Internal */
            //my_debugmon_dump_proc_info();
            //debugmon_dump_sched_info();
        }
        sleep(5000);
    }

    return 0;
}

void* network_main_thread(void *args)
{
#if LWIP_DHCP==1
    int mscnt = 0;
#endif
    /* initialize lwIP before calling sys_thread_new */
    sleep(5000);
    xil_printf("About to lwip_init\n");
    lwip_init();
    xil_printf("Done to lwip_init\n");

    /* any thread using lwIP should be created using sys_thread_new */
    sys_thread_new("NW_THRD", network_thread, NULL,
            THREAD_STACKSIZE,
            DEFAULT_THREAD_PRIO);
#if LWIP_DHCP==1
    while (1) {
#ifdef OS_IS_FREERTOS
        vTaskDelay(DHCP_FINE_TIMER_MSECS / portTICK_RATE_MS);
#else
        sleep(DHCP_FINE_TIMER_MSECS);
#endif
        if (server_netif.ip_addr.addr) {
            struct ip_addr *ip = &(server_netif.ip_addr);
            myIP[0] = ip4_addr1(ip);
            myIP[1] = ip4_addr2(ip);
            myIP[2] = ip4_addr3(ip);
            myIP[3] = ip4_addr4(ip);

            xil_printf("DHCP request success\r\n");
            print_ip_settings(&(server_netif.ip_addr), &(server_netif.netmask), &(server_netif.gw));
            /* print all application headers */
            print_headers();
            /* now we can start application threads */
            launch_app_threads();
            break;
        }
        mscnt += DHCP_FINE_TIMER_MSECS;
        if (mscnt >= 10000) {
            xil_printf("ERROR: DHCP request timed out\r\n");
            xil_printf("Configuring default IP of 192.168.1.99\r\n");
            IP4_ADDR(&(server_netif.ip_addr),  192, 168,   1, 99);
            IP4_ADDR(&(server_netif.netmask), 255, 255, 255,  0);
            IP4_ADDR(&(server_netif.gw),      192, 168,   1,  1);
            print_ip_settings(&(server_netif.ip_addr), &(server_netif.netmask), &(server_netif.gw));
            /* print all application headers */
            print_headers();
            launch_app_threads();
            break;
        }

    }
#ifdef OS_IS_FREERTOS
    vTaskDelete(NULL);
#endif
#endif

    return 0;
}
#ifdef __arm__
void vApplicationMallocFailedHook( void )
{
    /* vApplicationMallocFailedHook() will only be called if
    configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
    function that will get called if a call to pvPortMalloc() fails.
    pvPortMalloc() is called internally by the kernel whenever a task, queue or
    semaphore is created.  It is also called by various parts of the demo
    application.  If heap_1.c or heap_2.c are used, then the size of the heap
    available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
    FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
    to query the size of free heap space that remains (although it does not
    provide information on how the remaining heap might be fragmented). */
    taskDISABLE_INTERRUPTS();
    for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( xTaskHandle *pxTask, signed char *pcTaskName )
{
    ( void ) pcTaskName;
    ( void ) pxTask;

    /* vApplicationStackOverflowHook() will only be called if
    configCHECK_FOR_STACK_OVERFLOW is set to either 1 or 2.  The handle and name
    of the offending task will be passed into the hook function via its
    parameters.  However, when a stack has overflowed, it is possible that the
    parameters will have been corrupted, in which case the pxCurrentTCB variable
    can be inspected directly. */
    taskDISABLE_INTERRUPTS();
    for( ;; );
}
void vApplicationSetupHardware( void )
{

}

#endif

