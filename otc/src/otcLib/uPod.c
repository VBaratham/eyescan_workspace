/*
 * $Id: $
 *-
 *-   Purpose and Methods: Provide a mid-level interface to the micropod I2C
 *-      address space.
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
#include "uPod.h"

static int DevID = UPOD_I2C_DEV_ID;
static int uPodAddress = UPOD_RX_A_BASE_ADDRESS;

u8 _upod_addresses[8] = { \
        UPOD_RX_A_BASE_ADDRESS , UPOD_RX_B_BASE_ADDRESS , \
        UPOD_RX_C_BASE_ADDRESS, UPOD_RX_D_BASE_ADDRESS , \
        UPOD_TX_A_BASE_ADDRESS, UPOD_TX_B_BASE_ADDRESS, \
        UPOD_TX_C_BASE_ADDRESS, UPOD_TX_D_BASE_ADDRESS \
};

u8 upod_address( int idx ) { return _upod_addresses[idx]; };

/*
 *-
 *-   Purpose and Methods: Set the low level IIC interface instance index.
 *-
 */
int SetUPodDevID(int IICDevID) {
  DevID = IICDevID;
  return XST_SUCCESS;
}

int SetUPodI2CAddress(u8 upAddr) {
  int inRange = 
    ((upAddr>=UPOD_RX_A_BASE_ADDRESS) & (upAddr<=UPOD_RX_D_BASE_ADDRESS)) ||
    ((upAddr>=UPOD_TX_A_BASE_ADDRESS) & (upAddr<=UPOD_TX_D_BASE_ADDRESS));
  if( !inRange ) return XST_FAILURE;

  uPodAddress = upAddr;
  return XST_SUCCESS;
}

/*
 *-
 *-   Purpose and Methods:
 *-
 */
uPodMonitorData* GetUPodStatus() {
  int status;
  u8 upstat, bytes[8];
  uPodMonitorData *upmd = malloc(sizeof(uPodMonitorData));

  /* Get the raw 8 bytes, */
  status = ReadUPodByte(REG_STATUS,&upstat);
  if( status != XST_SUCCESS ) return 0;
  status = ReadUPodPage(REG_TEMPMON,bytes);
  if( status != XST_SUCCESS ) return 0;

  /* and pack them into a monitor data structure. Convert the temperature 
   * fraction from 1/256 units to 1/1000 units. */
  upmd->status = status;
  upmd->tempWhole = bytes[0];
  upmd->tempFrac = (u16)(((u32)(1000*bytes[1]))/256);
  upmd->v33 = (bytes[4]<<8) | bytes[5];
  upmd->v25 = (bytes[6]<<8) | bytes[7];
  return upmd;
}

/*
 *-
 *-   Purpose and Methods:
 *-
 */
void PrintUPodConfig() {
  uPodMonitorData *mondata = GetUPodStatus();
  if( !mondata ) {
    xil_printf("ERROR: uPod.c, PrintUPodStatus(), no data from getUPodStatus()\n");
    return;
  }

  xil_printf("uPod Monitor Data\n");
  xil_printf("   Status = 0x%02x\n",mondata->status);
  xil_printf("   Temperature %d.%03d C\n",mondata->tempWhole,mondata->tempFrac);
  xil_printf("   3.3V = %d uV\n",100*mondata->v33);
  xil_printf("   2.5V = %d uV\n",100*mondata->v25);
  free(mondata);
  return;
}


/*
 *-
 *-   Purpose and Methods: 
 *-
 */
int WriteUPodByte(u8 regaddr, u8 data) {
  int status;
  u8 buffer[2];
  buffer[0] = regaddr;
  buffer[1] = data;
  status = IICMasterWrite(DevID,uPodAddress,2,buffer);
  return status;
}

/*
 *-
 *-   Purpose and Methods: 
 *-
 */
int  ReadUPodByte(u8 regaddr, u8 *data) {
  int status;

  /* Set the register address */
  status = IICMasterWrite(DevID,uPodAddress,1,&regaddr);
  if( status != XST_SUCCESS ) return status;

  /* and do the read */
  status = IICMasterRead(DevID,uPodAddress,1,data);
  return status;
}

/*
 *-
 *-   Purpose and Methods: 
 *-
 */
int WriteUPodPage(u8 regaddr, u8 *data) {
  int status, idx;
  u8 buffer[9];
  buffer[0] = regaddr;
  for( idx=0 ; idx<8 ; idx++ ) buffer[idx+1] = data[idx];
  status = IICMasterWrite(DevID,uPodAddress,9,buffer);
  return status;
}

/*
 *-
 *-   Purpose and Methods: 
 *-
 */
int  ReadUPodPage(u8 regaddr, u8 *data) {
  int status;

  /* Set the register address */
  status = IICMasterWrite(DevID,uPodAddress,1,&regaddr);
  if( status != XST_SUCCESS ) return status;

  /* and do the read */
  status = IICMasterRead(DevID,uPodAddress,8,data);
  return status;
}

/*
 *-
 *-   Purpose and Methods: 
 *-
 */
int  ReadUPodRegister(u16 regaddr, u16 *data) {
}
