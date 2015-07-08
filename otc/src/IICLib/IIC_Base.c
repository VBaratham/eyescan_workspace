/*
 * $Id: $
 *-
 *- IIC_Base.c
 *-
 *-   Purpose and Methods: Mid-level interface to I2C devices.  Heavily based
 *-     XilKernel example code.
 *-
 *- A particular interface is identified by an integer device ID.  The id is 
 *- one of the available XPAR_IIC_*_DEVICE_ID (* = 0, 1, ...) IDs found in 
 *- xparameters.h  For the SBU AMC card there are two hardware interfaces:
 *-
 *-     devid = 0, master interface to clock and micro pod
 *-     devic = 1, slave interface to MMC
 *-
 *-   Inputs  : various
 *-   Outputs : various
 *-   Controls:
 *-
 *-   Created  16-JAN-2014   John D. Hobbs
 *-   $Revision: 1.3 $
 *-
 */
#define IIC_BASE_C

#include "xmk.h"
#include "xparameters.h"
#include "xiic.h"
#include "xil_exception.h"
#include "xintc.h"

#include "IIC_Base.h"
#include "IIC_InterruptIDConfig.h"

typedef u8 AddressType;
typedef XIic*  XIicInstPtr;

/************************** Local Function Prototypes ******************************/

static int IICInit();
static int IICInitDev(int DevID);
static void SendHandler(XIic *InstancePtr);
static void ReceiveHandler(XIic *InstancePtr);
static void StatusHandler(XIic *InstancePtr, int Event);

/************************** Variable Definitions *****************************/

u8 iic_doDebug=0;
static u8 initdone=0;
extern XIntc sys_intc;  /* Use the interrupt controller instance from the hw_init master file. */

/* The instances of the IIC devices and pointers to them (for convenience). */
XIic IicInstance[XPAR_XIIC_NUM_INSTANCES];  
XIicInstPtr IicInstancePtr[XPAR_XIIC_NUM_INSTANCES];

volatile u8 TransmitComplete;   /* Flag to check completion of Transmission */
volatile u8 ReceiveComplete;    /* Flag to check completion of Reception */

/************************** Function Definitions *****************************/

int IICMasterRead(int DevID, u8 targetAddr, u16 nbytes, u8 *data) {
  int Status;

  if( iic_doDebug ) xil_printf("In IICMasterRead(DevID=%d,targetAddr=%02x, nbytes=%d, *data=%08x). initdone=%d\n",DevID,targetAddr,nbytes,data,initdone);
  if( !initdone ) IICInit();
  if( DevID<0 || DevID>=XPAR_XIIC_NUM_INSTANCES  ) return XST_INVALID_PARAM;

  /* Set the Slave address. */
  if( iic_doDebug>1 ) xil_printf("  --(r)> About to XIic_SetAddress(IicInstancePtr[DevID]=%08x, XII_ADDR_TO_SEND_TYPE, targetAddr=%02x)\n",IicInstancePtr[DevID],targetAddr);
  Status = XIic_SetAddress(IicInstancePtr[DevID], XII_ADDR_TO_SEND_TYPE,targetAddr);
  if (Status != XST_SUCCESS) return XST_FAILURE;

  /* Set the Defaults. */
  ReceiveComplete = 1;

  /* Start the IIC device for the read */  
  if( iic_doDebug>1 ) xil_printf("  --(r)> About to XIic_Start(IicInstancePtr[DevID]=%08x)\n",IicInstancePtr[DevID]);
  Status = XIic_Start(IicInstancePtr[DevID]);
  if (Status != XST_SUCCESS) return XST_FAILURE;
  
  /* Receive the Data. */
  if( iic_doDebug>1 ) xil_printf("  --(r)> About to XIic_MasterRecv(IicInstancePtr[DevID]=%08x, data=%08x, nbytes=%d)\n",IicInstancePtr[DevID],data,nbytes);
  Status = XIic_MasterRecv(IicInstancePtr[DevID], data, nbytes);
  if (Status != XST_SUCCESS) return XST_FAILURE;
  
  /* Wait till all the data is received. */
  while ((ReceiveComplete) || (XIic_IsIicBusy(IicInstancePtr[DevID]) == TRUE)) {}
  
  /* Stop the IIC device. */
  if( iic_doDebug>1 ) xil_printf("  --(r)> About to XIic_Stop(IicInstancePtr[DevID]=%08x)\n",IicInstancePtr[DevID]);
  Status = XIic_Stop(IicInstancePtr[DevID]);
  if (Status != XST_SUCCESS) return XST_FAILURE;
  
  return XST_SUCCESS;
}


int IICMasterWrite(int DevID, u8 addr7bit, u16 nbytes, u8 *data){
  int Status;
  if( iic_doDebug ) xil_printf("In IICMasterWrite(DevID=%d,targetAddr=%02x, nbytes=%d, *data=%08x). initdone=%d\n",DevID,addr7bit,nbytes,data,initdone);

  if( !initdone ) IICInit();
  if( DevID<0 || DevID>=XPAR_XIIC_NUM_INSTANCES  ) return XST_INVALID_PARAM;

  /* Set the Slave address. */
  if( iic_doDebug>1 ) xil_printf("  --(w)> About to XIic_SetAddress(IicInstancePtr[DevID]=%08x, XII_ADDR_TO_SEND_TYPE, addr7bit=%02x)\n",IicInstancePtr[DevID],addr7bit);
  Status = XIic_SetAddress(IicInstancePtr[DevID], XII_ADDR_TO_SEND_TYPE,addr7bit);
  if( Status != XST_SUCCESS ) return XST_FAILURE;

  /* Set the defaults. */
  TransmitComplete = 1;
  IicInstance[DevID].Stats.TxErrors = 0;

  /* Start the IIC device. */
  if( iic_doDebug>1 ) xil_printf("  --(w)> About to XIic_Start(IicInstancePtr=%08x)\n",IicInstancePtr[DevID]);
  Status = XIic_Start(IicInstancePtr[DevID]);
  if( Status != XST_SUCCESS ) return XST_FAILURE;

  /* Send the Data.*/
  if( iic_doDebug>1 ) xil_printf("  --(w)> About to XIic_MasterSend(IicInstancePtr=%08x, data=%08x, nbytes=%d)\n",IicInstancePtr[DevID],data,nbytes);
  Status = XIic_MasterSend(IicInstancePtr[DevID], data, nbytes);
  if( Status != XST_SUCCESS ) return XST_FAILURE;

  /* Wait till the transmission is completed. */
  while( (TransmitComplete) || (XIic_IsIicBusy(IicInstancePtr[DevID]) == TRUE) ) {
    /*
     * This condition is required to be checked in the case where we
     * are writing two consecutive buffers of data to the EEPROM.
     * The EEPROM takes about 2 milliseconds time to update the data
     * internally after a STOP has been sent on the bus.
     * A NACK will be generated in the case of a second write before
     * the EEPROM updates the data internally resulting in a
     * Transmission Error.
     */
    if( IicInstance[DevID].Stats.TxErrors != 0 ) {
      if( iic_doDebug>1 ) xil_printf("      inside 2nd write block\n");

      /* Enable the IIC device.*/
      Status = XIic_Start(IicInstancePtr[DevID]);
      if (Status != XST_SUCCESS) return XST_FAILURE;

      if( !XIic_IsIicBusy(IicInstancePtr[DevID]) ) {
    /* Send the Data. */
    Status = XIic_MasterSend(IicInstancePtr[DevID],data,nbytes);
    if( Status == XST_SUCCESS ) {  IicInstance[DevID].Stats.TxErrors = 0; }
      }
    }
  }

  /* Stop the IIC device. */
  if( iic_doDebug>1 ) xil_printf("  --(w)> About to XIic_Stop(IicInstancePtr[DevID]=%08x)\n",IicInstancePtr[DevID]);
  Status = XIic_Stop(IicInstancePtr[DevID]);
  if( Status != XST_SUCCESS ) return XST_FAILURE;

  return XST_SUCCESS;
}

/***************************** Internal Routines *****************************/

static int IICInit() {
  int i=0, status=0;

  /* No repeats */
  if( initdone ) return XST_SUCCESS;

  /* Do each interface (crudely done) */
  if( iic_doDebug ) xil_printf("IICInit, XPAR_XIIC_NUM_INSTANCES = %d\n",XPAR_XIIC_NUM_INSTANCES);
  for( i=0 ; i<XPAR_XIIC_NUM_INSTANCES ; i++ ) {
    status = IICInitDev(i);
    if( status != XST_SUCCESS ) return status;
  }

  initdone = 1;
  return XST_SUCCESS;
}

static int IICInitDev(int DevID) {
  int Status;
  XIic_Config *ConfigPtr;

  if( iic_doDebug ) xil_printf("In IICInitDev(DevID=%d)\n",DevID);

  /* Get the device information from the UserConfiguration.  This is purely a
   * convenience which allows translation from the Xilinx #define constants
   * to an array structure indexed by device ID.  This replaces use of the
   * following xparameters.h #defines:
   *
   * and adds an additional field indicating whether it's a master or slave
   */
  /* Setup the pointer/instance correspondence */
  IicInstancePtr[DevID] = &(IicInstance[DevID]);

  /* Initialize the IIC driver so that it is ready to use. */
  if( iic_doDebug>1 ) xil_printf("  --(i)> About to XIic_LookupConfig(IIC_DEVICE_ID=%d)\n",DevID);
  ConfigPtr = XIic_LookupConfig(DevID);
  if (ConfigPtr == NULL) return XST_FAILURE;
  if( iic_doDebug>1 ) xil_printf("  --> About to XIic_CfgInitialize(IicInstancePtr[DevID]=%08x, ConfigPtr, ConfigPtr->BaseAddress)\n",IicInstancePtr[DevID]);
  Status = XIic_CfgInitialize(IicInstancePtr[DevID], ConfigPtr, ConfigPtr->BaseAddress);
  if (Status != XST_SUCCESS) return XST_FAILURE;


  /* Setup the Interrupt System.  ALWAYS USE THE SYSTEM INTERRUPT 
   * CONTROLLER INSTANCE. 
   *
   * Step 1:
   * Connect the device driver handler that will be called when an
   * interrupt for the device occurs, the handler defined above performs
   * the specific interrupt processing for the device.
   */
  XIicInstPtr current = &(IicInstance[DevID]);
  Status = XIntc_Connect(&sys_intc, IICInterruptVecID[DevID],
             (XInterruptHandler) XIic_InterruptHandler,
             current);
  if (Status != XST_SUCCESS) return XST_FAILURE;

  /* Step 2: Enable the interrupts for the IIC device. */
  XIntc_Enable(&sys_intc, IICInterruptVecID[DevID]);

  /* Step 3: Disable exceptions during the registration. Necessary? */
  Xil_ExceptionDisable();

  /* Step 4: Initialize the exceptions. */
  Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
                   (Xil_ExceptionHandler)XIntc_InterruptHandler, 
                   &sys_intc);

  /* Step 5 (last for controller setup): Re-enable non-critical exceptions */
  Xil_ExceptionEnable();

  /* Set the Handlers for transmit and reception. */
  XIic_SetSendHandler(IicInstancePtr[DevID], IicInstancePtr[DevID], (XIic_Handler) SendHandler);
  XIic_SetRecvHandler(IicInstancePtr[DevID], IicInstancePtr[DevID], (XIic_Handler) ReceiveHandler);
  XIic_SetStatusHandler(IicInstancePtr[DevID], IicInstancePtr[DevID], (XIic_StatusHandler) StatusHandler);
  return XST_SUCCESS;

}


/*****************************************************************************/
/**
 * This Send handler is called asynchronously from an interrupt
 * context and indicates that data in the specified buffer has been sent.
 *
 * @param   InstancePtr is not used, but contains a pointer to the IIC
 *      device driver instance which the handler is being called for.
 *
 * @return  None.
 *
 * @note        None.
 *
 ******************************************************************************/
static void SendHandler(XIic *InstancePtr)
{
  TransmitComplete = 0;
}

/*****************************************************************************/
/**
 * This Receive handler is called asynchronously from an interrupt
 * context and indicates that data in the specified buffer has been Received.
 *
 * @param   InstancePtr is not used, but contains a pointer to the IIC
 *      device driver instance which the handler is being called for.
 *
 * @return  None.
 *
 * @note        None.
 *
 ******************************************************************************/
static void ReceiveHandler(XIic *InstancePtr)
{
  ReceiveComplete = 0;
}

/*****************************************************************************/
/**
 * This Status handler is called asynchronously from an interrupt
 * context and indicates the events that have occurred.
 *
 * @param   InstancePtr is a pointer to the IIC driver instance for which
 *      the handler is being called for.
 * @param   Event indicates the condition that has occurred.
 *
 * @return  None.
 *
 * @note        None.
 *
 ******************************************************************************/
static void StatusHandler(XIic *InstancePtr, int Event)
{
}
