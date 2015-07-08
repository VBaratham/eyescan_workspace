/*
 * $Id: $
 *-
 *-   Purpose and Methods: 
 *-
 *-   Inputs  :
 *-   Outputs :
 *-   Controls:
 *-
 *-   Created  19-JAN-2014   John D. Hobbs
 *-
 *- $Revision: $
 *-
*/

#ifndef UPOD_H_
#define UPOD_H_

#include "xparameters.h"
#include "xil_types.h"

/* Define the 8 micropod I2C 7 bit addresses */
#define UPOD_RX_A_BASE_ADDRESS  0x30    /* U5 */
#define UPOD_RX_B_BASE_ADDRESS  0x31    /* U6 */
#define UPOD_RX_C_BASE_ADDRESS  0x32    /* U7 */
#define UPOD_RX_D_BASE_ADDRESS  0x33    /* U8 */

#define UPOD_TX_A_BASE_ADDRESS  0x28    /* U16 */
#define UPOD_TX_B_BASE_ADDRESS  0x29    /* U17 */
#define UPOD_TX_C_BASE_ADDRESS  0x2A    /* U18 */
#define UPOD_TX_D_BASE_ADDRESS  0x2B    /* U19 */

/* Define the I2C interface in the microblaze AXI space */
#define UPOD_I2C_DEV_ID XPAR_AXI_IIC_0_DEVICE_ID

/* and now register-specific constants (NYI) */
#define REG_STATUS  2
#define REG_TEMPMON 28
#define REG_V33MON  32
#define REG_V25MON  34

typedef struct {
  u8 status;
  char tempWhole;
  u16 tempFrac;
  u16 v33, v25;
} uPodMonitorData;

uPodMonitorData * upodStatus[8];

u8 upod_address( int idx );

/* ------------------------------- User Routines ---------------------------- */
/*
 *- This must be called before any other routines here if the AXI I2C device
 *- ID differs from UPOD_I2C_DEV_ID above.  The argument is the  XilKernel 
 *- device ID (XPAR_AXI_IIC_?_DEVICE_ID) of the microblaze I2C interface 
 *- attached to the micropods.  For the OTC, this should be ID=0.
 */
int SetUPodDevID(int IICDevID);
int SetUPodI2CAddress(u8 upAddr);

/* 
 * Higher level interfaces. 
 */
uPodMonitorData* GetUPodStatus(); /* Caller owns returned struct. Must free() it */
void PrintUPodConfig();

/*  
 * Basic register access interface.  Provide the offset and the data buffer 
 */
int WriteUPodByte(u8 regaddr, u8 data);
int  ReadUPodByte(u8 regaddr, u8 *data);   /* User supplies 1 byte storage... */
int WriteUPodPage(u8 regaddr, u8 *data);
int  ReadUPodPage(u8 regaddr, u8 *data);  /* User supplies 8 bytes storage... */

#endif /* UPOD_H_ */
