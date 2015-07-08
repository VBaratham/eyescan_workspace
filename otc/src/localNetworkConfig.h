#ifndef LOCALNETWORKCONFIG
#define LOCALNETWORKCONFIG

#include "xil_types.h"

/* Switch between a private net 192.168.1.99 and SBU IP address */

#define PRIVATE    0
#define STONYBROOK 1

#define LOCAL_NETWORK PRIVATE

#ifndef LOCALNETWORKCONFIG_C
extern u8 myMac[6];
extern u8 myIP[4];
extern u8 myMask[4];
extern u8 myGateway[4];
extern u8 udpSendDest[4];
#endif

#endif
