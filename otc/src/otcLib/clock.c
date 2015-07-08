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

#include <stdlib.h>
#include "xstatus.h"
#include "xil_types.h"

#include "../IICLib/IIC_Base.h"
#include "clock.h"

static int DevID = -1;

static void pack16(u8 *buffer, u16 value) {
  buffer[0] = value>>8;
  buffer[1] = value&0xFF;
}

static u16 unpack16(u8 *buffer) {
  u16 value;
  value = (buffer[0]<<8) | buffer[1];
  return value;
}


static void pack32(u8 *buffer, u32 value) {
  buffer[0] = (value>>24) & 0xFF;
  buffer[1] = (value>>16) & 0xFF;
  buffer[2] = (value>>8)  & 0xFF;
  buffer[3] = value & 0xFF;
}

static u32 unpack32(u8 *buffer) {
  u32 value;
  value = (buffer[0]<<24) | (buffer[1]<<16) | (buffer[2]<<8) | buffer[3];
  return value;
}

/*
 *-
 *-   Purpose and Methods: Set the low level IIC interface instance index.
 *-
 */
int SetClockDevID(int IICDevID) {
  DevID = IICDevID;
  return XST_SUCCESS;
}


/*
 *    Purpose and Methods: Make a default clock register initialization
 */
int InitClockRegisters() {
    u16 regval[21]={

            0x01B9, 0x24C4, 0x74FA, 0x04FA, 0x306F,
            0x0023, 0x0003, 0x0023, 0x0003, 0x00C3,
            0x0030, 0x0000, 0x00C3, 0x0030, 0x0000,
            0x00C3, 0x0030, 0x0000, 0x00C3, 0x0030,
            0x0000

    };
    u16 ireg;

    xil_printf("Initializing clock registers\n");
    for( ireg=0 ; ireg<21 ; ireg++ ) WriteClockRegister(ireg,regval[ireg]);
    WriteClockRegister(3,regval[3] & 0xFFBF);  /* clear the reset bit to reset */
    WriteClockRegister(3,regval[3]);           /* return to normal running state */
    return 0;
}


/*
 *-
 *-   Purpose and Methods:
 *-
 */
u16* GetClockConfig() {
  u16 ireg;
  u16 *cfgdata, *curdata;
  int status;

  curdata = cfgdata = malloc(sizeof(u16)*21);  /* Room for each register value */
  for( ireg=0 ; ireg<21 ; ireg++ ) {
    status = ReadClockRegister(ireg,curdata++);
    if( status != XST_SUCCESS ) {
      free(cfgdata);
      return 0;
    }
  }

  return cfgdata;
}

/*
 *-
 *-   Purpose and Methods:
 *-
 */
void PrintClockConfig() {
  int ireg=0;
  u16 *cfgdata = GetClockConfig();
  if( !cfgdata ) {
    xil_printf("ERROR: clock.c, printClockConfig(), no data from getClockConfig()\n");
    return;
  }

  xil_printf("Clock registers:\n");
  for( ireg=0 ; ireg<21 ; ireg++ ) xil_printf(" Reg %2d, value = %04x\n",ireg,cfgdata[ireg]);
  free(cfgdata);
  return;
}

/*
 *-
 *-   Purpose and Methods: 
 *-
 */
int WriteClockRegister(u16 regaddr, u16 data) {
  int status;
  u8 buffer[sizeof(u16)+sizeof(u16)];
  pack16(buffer,regaddr);
  pack16(&buffer[sizeof(u16)],data);
  status = IICMasterWrite(DevID,CLOCK_BASE_ADDRESS,sizeof(u16)+sizeof(u16),buffer);
  return status;
}

/*
 *-
 *-   Purpose and Methods: 
 *-
 */
int  ReadClockRegister(u16 regaddr, u16 *data) {
  u8 buffer[sizeof(u16)];
  int status;

  /* Set the register address */
  pack16(buffer,regaddr);
  status = IICMasterWrite(DevID,CLOCK_BASE_ADDRESS,sizeof(u16),buffer);
  if( status != XST_SUCCESS ) return status;

  /* and do the read */
  status = IICMasterRead(DevID,CLOCK_BASE_ADDRESS,sizeof(u16),buffer);
  if( status != XST_SUCCESS ) return status;

  *data = unpack16(buffer);
  return XST_SUCCESS;
}
