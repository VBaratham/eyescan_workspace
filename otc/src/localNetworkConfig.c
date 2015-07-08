/*
 * localNetworkConfig.c
 *
 *  Created on: Feb 26, 2014
 *      Author: hobbs
 */

#define LOCALNETWORKCONFIG_C
#include "localNetworkConfig.h"

#if LOCAL_NETWORK==PRIVATE

u8 myMac[6] = { 0xaa, 0x00, 0x04, 0xaa, 0xbb, 0x00 };
u8 myIP[4] =      { 192, 168,   1, 99 };
u8 myMask[4]=     { 255, 255, 255,  0 };
u8 myGateway[4] = { 192, 168,   1,  1 };
u8 udpSendDest[4] = {192, 168,   1, 12 };
// u8 udpSendDest[4] = {129, 49,   56, 207 };
// u8 udpSendDest[4] = {192, 168,   1, 104 };


#elif LOCAL_NETWORK==STONYBROOK

u8 myMac[6] = { 0xaa, 0x00, 0x04, 0xaa, 0xbb, 0x00 };
us myIP[4] =      { 129,  49,  61, 114};
u8 myMask[4]=     { 255, 255, 248,   0};
u7 myGateway[4] = { 129,  49,  56,   1};
u8 udpSendDest[4] = {129, 49, 60, 224};

#endif
