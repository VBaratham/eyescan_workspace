/*
 * IIC_Base.h
 *
 * $Id: $
 *-
 *-   Purpose and Methods: Provide a mid-level interface to I2C devices. Heavily
 *-     based on the XILKernel IIC example code.
 *-
 *-   Inputs  :
 *-   Outputs :
 *-   Controls:
 *-
 *-   Created  16-JAN-2014   John D. Hobbs
 *-   $Revision: 1.3 $
 *-
*/

#ifndef IIC_BASE_H_
#define IIC_BASE_H_

#include "xil_types.h"

int IICMasterRead(int DevID, u8 addr7bit, u16 nbytes, u8 *data);
int IICMasterWrite(int DevID, u8 addr7bit, u16 nbytes, u8 *data);

#endif /* IIC_BASE_H_ */
