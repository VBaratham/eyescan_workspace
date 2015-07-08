/*
 * SysStatus.h
 *
 *  Created on: Feb 21, 2014
 *      Author: hobbs
 */

#ifndef SYSSTATUS_H_
#define SYSSTATUS_H_

#include "xaxiethernet.h"

struct procInfo {
    int uptime;
    float v7temp;
    float v7vCCINT;
    float v7vCCAUX;
    float v7vBRAM;
};

struct ethInfo {
  u32 regVal[0x800];
};

#ifndef SYSSTATUS_C
extern struct procInfo procStatus;
extern struct ethInfo  ethStatus;
#endif

#define POLL_UPOD_TEMPS TRUE

#endif /* SYSSTATUS_H_ */
