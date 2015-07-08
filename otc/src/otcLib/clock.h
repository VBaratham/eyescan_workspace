/*
 * $Id: $
 *-
 *-   Purpose and Methods: High-level access to the TI CDCM6208 on the OTC
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

#ifndef CLOCK_H_
#define CLOCK_H_

#include "xil_types.h"

/* The I2C address of the TI CDCM6208 */
#define CLOCK_BASE_ADDRESS 0x54

/* and now register-specific constants (NYI) */

/* ------------------------------- User Routines ---------------------------- */
/*
 *- This must be called before any other routines here.  The argument is the 
 *- XilKernel device ID (XPAR_AXI_IIC_?_DEVICE_ID) of the microblaze I2C 
 *- interface attached to the clock chip.  For the OTC, this should be ID=0.
 */
int SetClockDevID(int IICDevID); /* Could this be automated in this library? */

/*
 *  Hardwired initial register values
 */
int InitClockRegisters();

/* 
 * Higher level interfaces. 
 */
u16* GetClockConfig();   /* Caller owns the returned pointer.  Must free() it */
void PrintClockConfig();

/*  
 * Basic register access interface.  Provide the offset and the data buffer 
 */
int WriteClockRegister(u16 regaddr, u16 data);
int  ReadClockRegister(u16 regaddr, u16 *data);  /* User supplies storage... */

#endif /* CLOCK_H_ */
