/*
 * Copyright (c) 2009 Xilinx, Inc.  All rights reserved.
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

/* helloworld_xmk.c: launch a thread that prints out Hello World */

#include "xmk.h"
#include <stdio.h>
#include <string.h>
#include "sys/timer.h"

/* This file has the basic configuration info, including device IDs */
#include "xparameters.h"

/* The default XADC access functions. */
#include "xsysmon_hw.h"
#include "xsysmon.h"

#include "otcLib/clock.h"
#include "otcLib/uPod.h"

#include "SysStatus.h"

char msgtime[32],msgtv[32];

void print_upod(u8 upodaddr) {
  SetUPodI2CAddress(upodaddr);
  PrintUPodConfig();
}

/* Display the message buffer */
void *monitor(char *arg) {
  int nsleep = 1;
  XSysMon adcHandle;
  XSysMon_Config *xcfg = XSysMon_LookupConfig(0);

  xil_printf("\n----------------------------------------------------------------------------\n");
  XSysMon_CfgInitialize(&adcHandle,xcfg,xcfg->BaseAddress);
  //xil_printf("XSysMon size = %d, array size = %d\n",sizeof(adcHandle));

  SetClockDevID(0);
  //PrintClockConfig();
  //InitClockRegisters();
  //PrintClockConfig();

  while(1) {
    sleep(1000);

    /* Store the current timer value as a text string in the message buffer.*/
    //sprintf(msgtime,"\nUp: %d s",nsleep);
    //xil_printf("%s\n",msgtime);

    /* Store the temperature and voltage values as a text string in the message buffer */
    u16 rawTemp = XSysMon_GetAdcData(&adcHandle,XSM_CH_TEMP);
    float temp = XSysMon_RawToTemperature(rawTemp);
    procStatus.uptime = nsleep;
    procStatus.v7temp = temp;

    u16 rawVolts = XSysMon_GetAdcData(&adcHandle,XSM_CH_VCCINT);
    float volt = XSysMon_RawToVoltage(rawVolts);
    procStatus.v7vCCINT = volt;
    rawVolts = XSysMon_GetAdcData(&adcHandle,XSM_CH_VCCAUX);
    volt = XSysMon_RawToVoltage(rawVolts);
    procStatus.v7vCCAUX = volt;
    rawVolts = XSysMon_GetAdcData(&adcHandle,XSM_CH_VBRAM);
    volt = XSysMon_RawToVoltage(rawVolts);
    procStatus.v7vBRAM = volt;

#if POLL_UPOD_TEMPS
    int idx = 0;
    for( idx = 0 ; idx < 8 ; idx++ ) {
        u8 i2c_addr = upod_address(idx);
        SetUPodI2CAddress( i2c_addr );
        upodStatus[idx] = GetUPodStatus();
    }
#endif

    nsleep += 1;
  }
}
