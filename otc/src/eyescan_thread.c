/*
 * $Id: 
 *-
 *-   Purpose and Methods: 
 *-
 *-   Inputs  :
 *-   Outputs :
 *-   Controls:
 *-
 *-   Created  27-APR-2014   John D. Hobbs
 *- $Revision: $
 *-
*/
#include "xmk.h"
#include <stdio.h>
#include <string.h>

#include "xaxi_eyescan.h"


void *eyescan_thread(char *arg) {
 int chidx = 1,i;

  xil_printf("\n--------------------- eyescan_thread.c ----------------------------------\n");
  sleep(5000);   /* Keep monitor and lcd_controller out of phase until thread safe. */

  /* Global register access.  Test the GTX configuration reading */
  u16 val = xaxi_eyescan_read_global(XAXI_EYESCAN_NGTX);
  int ngtx = (int)val & XAXI_EYESCAN_GTX_MASK;
  xil_printf("Eyescan base register:  %08x.  GTX count = %d, Quad count = %d\n",val,val&XAXI_EYESCAN_GTX_MASK,(val&XAXI_EYESCAN_QUAD_MASK)>>8);
  val = xaxi_eyescan_read_global(XAXI_EYESCAN_NLEFT);
  xil_printf("Eyescan left register:  %08x.  GTX count = %d, Quad count = %d\n",val,val&XAXI_EYESCAN_GTX_MASK,(val&XAXI_EYESCAN_QUAD_MASK)>>8);
  val = xaxi_eyescan_read_global(XAXI_EYESCAN_NRIGHT);
  xil_printf("Eyescan right register: %08x.  GTX count = %d, Quad count = %d\n",val,val&XAXI_EYESCAN_GTX_MASK,(val&XAXI_EYESCAN_QUAD_MASK)>>8);
  val = xaxi_eyescan_read_global(XAXI_EYESCAN_QPLL_LOCK);
  xil_printf("QPLL lock status: 0x%04x\n",val);
  val = xaxi_eyescan_read_global(XAXI_EYESCAN_QPLL_LOST);
  xil_printf("QPLL lost status: 0x%04x\n",val);

  /* Check the QPLL refclk frquency counters */
  val = xaxi_eyescan_read_global(XAXI_EYESCAN_GLOBAL_RESET);
  val |= (u16)(1<<9);       /* Clear contents and clear enable */
  xaxi_eyescan_write_global(XAXI_EYESCAN_GLOBAL_RESET,val);
  val &= ~(1<<9);
  val |= (1<<8);            /* Release clear and set enable */
  xaxi_eyescan_write_global(XAXI_EYESCAN_GLOBAL_RESET,val);
  sleep(1000);
  u16 fval = xaxi_eyescan_read_global(XAXI_EYESCAN_QPLL_BASEFREQ_COUNT);
  xil_printf("  %s frequency counter = %d\n","QPLL Base",fval);
  fval = xaxi_eyescan_read_global(XAXI_EYESCAN_QPLL_FREQ0_COUNT);
  xil_printf("  %s frequency counter = %d\n","QPLL left clock Q1",fval);
  fval = xaxi_eyescan_read_global(XAXI_EYESCAN_QPLL_FREQ1_COUNT);
  xil_printf("  %s frequency counter = %d\n","QPLL left clock Q2",fval);
  fval = xaxi_eyescan_read_global(XAXI_EYESCAN_QPLL_FREQ2_COUNT);
  xil_printf("  %s frequency counter = %d\n","QPLL right clock Q1",fval);
  fval = xaxi_eyescan_read_global(XAXI_EYESCAN_QPLL_FREQ3_COUNT);
  xil_printf("  %s frequency counter = %d\n","QPLL right clock Q2",fval);

  /* Now check each channel, looking to see:
   *   (1) Is the eyescan enabled?
   *   (2) Are there any data clocks?
   *   (3) Is the reset functioning?
   */
  const u16 PMAREG = 0x82;
  const u16 EYESCAN_HWEN_MASK = 0x20;
  const u16 FREQ_CHANNEL_SEL_MASK = 0xFC00;
  int errcode=0, igtx;
  for( igtx=0 ; igtx<ngtx ; igtx++ ) {
      xil_printf("Channel %d\n",igtx);
      /* Eyescan enable check */
      //u32 reg = 0x3D;
      //xaxi_eyescan_write_channel_drp(0,reg,0x0300);
      //u32 val1 = xaxi_eyescan_read_channel_drp(igtx,reg);
      u16 pmaval = xaxi_eyescan_read_channel_drp(igtx,PMAREG);
      if( (pmaval&EYESCAN_HWEN_MASK) == 0 ) { xil_printf("  Eyescan disabled. ERROR\n"); errcode += 1; }
      else xil_printf("  Eyescan enabled (0x%04x)\n",pmaval);
      //pmaval |= EYESCAN_HWEN_MASK;       // Enable the eyescan circuity.
      //xaxi_eyescan_write_channel_drp(0,PMAREG,pmaval);
      //u32 valrsv2 = xaxi_eyescan_read_channel_drp(0,PMAREG);
      //xil_printf("PMA_RSV2 = 0x%04x\n",valrsv2);
      //if( esval != valrsv2 ) xil_printf("ERROR: Failed to write PMA_RSV2 with expected value\n");

      /* Check the clocks for each channel.  This requires a short wait (2^15 / SYSTEM CLOCK FREQ) s */
      u16 crval = xaxi_eyescan_read_global(XAXI_EYESCAN_GLOBAL_RESET);
      crval &= ~FREQ_CHANNEL_SEL_MASK;
      crval |= (u16)((igtx << 10) + (1<<9));
      xaxi_eyescan_write_global(XAXI_EYESCAN_GLOBAL_RESET,crval);
      crval &= ~(1<<9);
      crval |= (1<<8);
      xaxi_eyescan_write_global(XAXI_EYESCAN_GLOBAL_RESET,crval);
      sleep(1000);
      u16 fval = xaxi_eyescan_read_global(XAXI_EYESCAN_BASEFREQ_COUNT);
      xil_printf("  %s frequency counter = %d\n","Base",fval);
      fval = xaxi_eyescan_read_global(XAXI_EYESCAN_FREQ0_COUNT);
      xil_printf("  %s frequency counter = %d\n","Tx user clock 1",fval);
      fval = xaxi_eyescan_read_global(XAXI_EYESCAN_FREQ1_COUNT);
      xil_printf("  %s frequency counter = %d\n","Tx user clock 2",fval);
      fval = xaxi_eyescan_read_global(XAXI_EYESCAN_FREQ2_COUNT);
      xil_printf("  %s frequency counter = %d\n","Rx user clock 1",fval);
      fval = xaxi_eyescan_read_global(XAXI_EYESCAN_FREQ3_COUNT);
      xil_printf("  %s frequency counter = %d\n","Rx user clock 2",fval);
  }

  if ( 1 ) return;

  /* Set the "Dont reset on error bit' */
  u32 grd = xaxi_eyescan_read_global(XAXI_EYESCAN_GLOBAL_RESET);
  xaxi_eyescan_write_global(XAXI_EYESCAN_GLOBAL_RESET,0x2);
  u32 grd2 = xaxi_eyescan_read_global(XAXI_EYESCAN_GLOBAL_RESET);
  xil_printf("Eyescan global reset register: %08x\n",grd);
  xil_printf("Eyescan global reset register: %08x\n",grd2);


  /* Now do the per channel configuration and reset sequence */
  u32 txVal = 0x1;
  for( chidx=0 ; chidx<ngtx ; chidx++ ) {
      // Configure
      xaxi_eyescan_write_channel_reg(chidx,XAXI_EYESCAN_TXCFG,txVal);
      xaxi_eyescan_write_channel_reg(chidx,XAXI_EYESCAN_RXCFG,1);
      xil_printf("Eyescan channel %d: TX config register: %08x\n",chidx,xaxi_eyescan_read_channel_reg(chidx,XAXI_EYESCAN_TXCFG));
      xil_printf("Eyescan channel %d: RX config register: %08x\n",chidx,xaxi_eyescan_read_channel_reg(chidx,XAXI_EYESCAN_RXCFG));
      // Reset the TX
      xaxi_eyescan_write_channel_reg(chidx,XAXI_EYESCAN_RESET,0x20);
      xaxi_eyescan_write_channel_reg(chidx,XAXI_EYESCAN_RESET,0);
  }

  sleep(1000);

  for( chidx=0 ; chidx<ngtx ; chidx++ ) {
      // and now reset the RX
      xaxi_eyescan_write_channel_reg(chidx,XAXI_EYESCAN_RESET,0x10);
      xaxi_eyescan_write_channel_reg(chidx,XAXI_EYESCAN_RESET,0);
  }

  /* Make sure we can read the eyescan registers if the eyescan is requested.
   * This simply ensures that the hardware is enabled.  If not, this hangs the whole system. */
  u16 esval = xaxi_eyescan_read_channel_drp(0,PMAREG);
  u32 reg = 0x151;
  if( esval & EYESCAN_HWEN_MASK == EYESCAN_HWEN_MASK ) {
      u32 rv = xaxi_eyescan_read_channel_drp(0,reg);
      xil_printf("0x%x = %x\n",reg,rv);
  }

  /* and another */
  //for( i=0x11 ; i<0x17 ; i++ ) xaxi_eyescan_write_channel_drp(0,i,i-0x10);
  //for( i=0x11 ; i<0x17 ; i++ ) xil_printf("0x%x = %x\n",i,xaxi_eyescan_read_channel_drp(0,i));

  /* Check the frequency counting logic on channel 0 only, but in the global space. */
  grd = xaxi_eyescan_read_global(XAXI_EYESCAN_GLOBAL_RESET);
  xaxi_eyescan_write_global(XAXI_EYESCAN_GLOBAL_RESET,grd | 0x200);  // Clear it
  xaxi_eyescan_write_global(XAXI_EYESCAN_GLOBAL_RESET,grd | 0x100);  // Enable it (and remove the clear)
  sleep(1000);   // Probably long enough to halt with max count
  xaxi_eyescan_write_global(XAXI_EYESCAN_GLOBAL_RESET,grd);  // Disable it (w/out clearing it)
  xil_printf("Base frequency count: %d\n",xaxi_eyescan_read_global(XAXI_EYESCAN_BASEFREQ_COUNT));
  xil_printf("Chn0 frequency count: %d\n",xaxi_eyescan_read_global(XAXI_EYESCAN_FREQ0_COUNT));
  xil_printf("Chn1 frequency count: %d\n",xaxi_eyescan_read_global(XAXI_EYESCAN_FREQ1_COUNT));
  xil_printf("Chn2 frequency count: %d\n",xaxi_eyescan_read_global(XAXI_EYESCAN_FREQ2_COUNT));
  xil_printf("Chn3 frequency count: %d\n",xaxi_eyescan_read_global(XAXI_EYESCAN_FREQ3_COUNT));

  grd2 = xaxi_eyescan_read_global(XAXI_EYESCAN_GLOBAL_RESET);
  xil_printf("Eyescan global reset register: %08x\n",grd);
  xil_printf("Eyescan global reset register: %08x\n",grd2);


  chidx=1;
  while(1) {
    sleep(10000);

    /* Channel register access */
    for( chidx=0 ; chidx<ngtx ; chidx++ ) {
        u32 read1 = xaxi_eyescan_read_channel_reg(chidx,XAXI_EYESCAN_RESET);
        //xaxi_eyescan_write_channel_reg(chidx,XAXI_EYESCAN_RESET,0xFFFF);
        //u32 read2 = xaxi_eyescan_read_channel_reg(chidx,XAXI_EYESCAN_RESET);
        //xaxi_eyescan_write_channel_reg(chidx,XAXI_EYESCAN_RESET,0);
        //u32 read3 = xaxi_eyescan_read_channel_reg(chidx,XAXI_EYESCAN_RESET);
        xil_printf("Channel %d: Reset register(init): %08x\n",chidx,read1);
        //xil_printf("Channel %d: Reset register(2): %08x\n",chidx,read2);
        //xil_printf("Channel %d: Reset register(3): %08x\n",chidx,read3);
        //if( (txVal&0x2) != 0) {
            u32 errcnt = xaxi_eyescan_read_channel_reg(chidx,XAXI_EYESCAN_MONITOR) & 0x80FF;
            xil_printf("Channel %d: Monitor register: %08x\n",chidx,errcnt);
        //}
    }


    /* Channel DRP access */
    //int regidx = 0x3C;
    //u32 readDRP1 = xaxi_eyescan_read_channel_drp(chidx,regidx);
    //xaxi_eyescan_write_channel_drp(chidx,regidx,0xFFFF);
    //u32 readDRP2 = xaxi_eyescan_read_channel_drp(chidx,regidx);
    //xaxi_eyescan_write_channel_drp(chidx,regidx,0x0000);
    //u32 readDRP3 = xaxi_eyescan_read_channel_drp(chidx,regidx);
    //xil_printf("Channel %d, DRP register 0x%x (1): 0x%08x\n",chidx,regidx,readDRP1);
    //xil_printf("Channel %d, DRP register 0x%x (2): 0x%08x\n",chidx,regidx,readDRP2);
    //xil_printf("Channel %d, DRP register 0x%x (3): 0x%08x\n",chidx,regidx,readDRP3);
  }
}
