/*
 * $Id: $
 *-
 *-   Purpose and Methods: This should be included only in IIC_Base.c  It 
 *-     defines an array to convert the IIC interrupt IDs from the XilKernel
 *-     #defines to an indexible form.
 *-
 *-   Inputs  :
 *-   Outputs :
 *-   Controls:
 *-
 *-   Created  19-JAN-2014   John D. Hobbs
 *-
 *- $Revision: 1.3 $
 *-
*/
#ifndef IIC_USERCONFIGURATION_H_
#define IIC_USERCONFIGURATION_H_

#include "xparameters.h"
#ifdef IIC_BASE_C  

#if XPAR_XIIC_NUM_INSTANCES == 1
static int IICInterruptVecID[XPAR_XIIC_NUM_INSTANCES] = 
  {
    XPAR_INTC_0_IIC_0_VEC_ID 
  };

#elif XPAR_XIIC_NUM_INSTANCES == 2
static int IICInterruptVecID[XPAR_XIIC_NUM_INSTANCES] = 
  {
    XPAR_INTC_0_IIC_0_VEC_ID, 
    XPAR_INTC_0_IIC_1_VEC_ID
  };
#endif


#endif
#endif /* IIC_USERCONFIGURATION_H_ */
