/*
 * Copyright (c) 2009 Xilinx, Inc.  All rights reserved.
 *
 * Xilinx, Inc.
 * XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS" AS A
 * COURTESY TO YOU.  BY PROVIDING THIS DESIGN, CODE, OR INFORMATION AS
 * ONE POSSIBLE   IMPLEMENTATION OF THIS FEATURE, APPLICATION OR
 * STANDARD, XILINX IS MAKING NO REPRESENTATION THAT THIS IMPLEMENTATION
 * IS FREE FROM ANY CLAIMS OF INFRINGEMENT, AND YOU ARE RESPONSIBLE
 * FOR OBTAINING ANY RIGHTS YOU MAY REQUIRE FOR YOUR IMPLEMENTATION.
 * XILINX EXPRESSLY DISCLAIMS ANY WARRANTY WHATSOEVER WITH RESPECT TO
 * THE ADEQUACY OF THE IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO
 * ANY WARRANTIES OR REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE
 * FROM CLAIMS OF INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#include "xparameters.h"
#include "es_controller.h"
#include "es_simple_eye_acq.h"
#include "drp.h"
#include "xaxi_eyescan.h"
#include <semaphore.h>
#include "safe_printf.h"

#define MODIFY_CURSOR TRUE
#define MODIFY_DRP_REGISTERS TRUE
#define TURN_ON_CONFIG_REGISTERS TRUE
// #define FREQUENCY 125
// #define FREQUENCY 625
#define FREQUENCY 640
#define DEBUG FALSE
#define RUN_ES_ACQUISITION TRUE

sem_t eyescan_sem;

void eyescan_lock() {
    if( sem_wait( &eyescan_sem ) == -1 ) {
        sem_init( &eyescan_sem , 0 , 1 );
        sem_wait( &eyescan_sem );
    }
}

void eyescan_unlock() {
    sem_post( &eyescan_sem );
}

eye_scan * eye_scan_lanes[MAX_NUMBER_OF_LANES];

u8 do_global_run_eyescan = FALSE;
u8 is_global_upload_ready = FALSE;

void global_run_eye_scan() { do_global_run_eyescan = TRUE; }
void global_stop_eye_scan() { do_global_run_eyescan = FALSE; }

u8 global_upload_ready() { return is_global_upload_ready; }
void global_upload_unrdy() { is_global_upload_ready = FALSE; }

eye_scan * get_eye_scan_lane( int lane ) {
    if( lane > MAX_NUMBER_OF_LANES )
        return NULL;
    return eye_scan_lanes[lane];
}

void init_eye_scan_struct( eye_scan * p_lane ) {
    //Initialize struct members to default values
    p_lane->enable = FALSE;
    p_lane->initialized = FALSE;
    p_lane->state = WAIT_STATE;
    p_lane->p_upload_rdy = FALSE;
    if( p_lane->pixels == NULL ) {
        p_lane->pixels = malloc( sizeof(eye_scan_pixel) * NUM_PIXELS_TOTAL );
    }
    p_lane->horz_offset = 0;
    p_lane->vert_offset = 0;
    p_lane->ut_sign = 0;
    p_lane->pixel_count = 0;
    p_lane->lane_number = 0;
    p_lane->prescale = 0;
}

/*
 * Functions: configure_eye_scan, init_eye_scan
 * Description: Configure/Initialize eye_scan data struct members and write related attributes through DRP
 *
 * Parameters:
 * p_lane: pointer to eye scan data structure
 *
 * Returns: True if completed, False if not
 */

int configure_eye_scan(eye_scan* p_lane, u8 curr_lane) {
    u8 i;
    
    //Initialize other struct members to default values
    p_lane->state = RESET_STATE;

    p_lane->lane_number = curr_lane;
    //p_lane->lane_name[0] = lane_name;

    if( DEBUG ) xil_printf( "set cursor\n");
    // if( MODIFY_CURSOR ) xaxi_eyescan_write_channel_reg(curr_lane, XAXI_EYESCAN_CURSOR, 0x0703);
    // if( MODIFY_CURSOR ) xaxi_eyescan_write_channel_reg(curr_lane, XAXI_EYESCAN_CURSOR, 0x8703);
    if( MODIFY_CURSOR ) xaxi_eyescan_write_channel_reg(curr_lane, XAXI_EYESCAN_CURSOR, 0x3067);
    // if( MODIFY_CURSOR ) xaxi_eyescan_write_channel_reg(curr_lane, XAXI_EYESCAN_CURSOR, 0xb067);

    if( DEBUG ) xil_printf( "eyescan enable\n");
    //Write ES_ERRDET_EN, ES_EYESCAN_EN attributes to enable eye scan
    drp_write(0x1, ES_EYESCAN_EN, curr_lane );
    drp_write(0x1, ES_ERRDET_EN, curr_lane );

    if( DEBUG ) xil_printf( "align word\n");
    if( TURN_ON_CONFIG_REGISTERS ) drp_write(0x1, ALIGN_COMMA_WORD, curr_lane);
    if( TURN_ON_CONFIG_REGISTERS ) drp_write_raw( 0x7f , 0x03E , 0 , 9 , curr_lane ); // ALIGN_COMMA_ENABLE
    
    if( DEBUG ) xil_printf( "fix sdata mask\n");
    //Write ES_SDATA_MASK0-1 attribute based on parallel data width
    switch (p_lane->data_width)
        {
          case 40: {
#if MODIFY_DRP_REGISTERS
              drp_write(0x1 , TX_INT_DATAWIDTH , curr_lane );
              drp_write(0x1 , RX_INT_DATAWIDTH , curr_lane );
              drp_write(0x5 , TX_DATA_WIDTH , curr_lane );
              drp_write(0x5 , RX_DATA_WIDTH , curr_lane );
#endif
#if TURN_ON_CONFIG_REGISTERS
              drp_write(0x0000, ES_SDATA_MASK0, curr_lane);
              drp_write(0x0000, ES_SDATA_MASK1, curr_lane);
#endif
              }
              break;
          case 32: {
#if MODIFY_DRP_REGISTERS
              drp_write(0x1 , TX_INT_DATAWIDTH , curr_lane );
              drp_write(0x1 , RX_INT_DATAWIDTH , curr_lane );
              drp_write(0x4 , TX_DATA_WIDTH , curr_lane );
              drp_write(0x4 , RX_DATA_WIDTH , curr_lane );
#endif
#if TURN_ON_CONFIG_REGISTERS
              drp_write(0x00FF, ES_SDATA_MASK0, curr_lane);
              drp_write(0x0000, ES_SDATA_MASK1, curr_lane);
#endif
              }
              break;
          case 20: {
#if MODIFY_DRP_REGISTERS
              drp_write(0x0 , TX_INT_DATAWIDTH , curr_lane );
              drp_write(0x0 , RX_INT_DATAWIDTH , curr_lane );
              drp_write(0x3 , TX_DATA_WIDTH , curr_lane );
              drp_write(0x3 , RX_DATA_WIDTH , curr_lane );
#endif
#if TURN_ON_CONFIG_REGISTERS
              drp_write(0xFFFF, ES_SDATA_MASK0, curr_lane);
              drp_write(0x000F, ES_SDATA_MASK1, curr_lane);
#endif
              }
              break;
          case 16: {
#if MODIFY_DRP_REGISTERS
              drp_write(0x0 , TX_INT_DATAWIDTH , curr_lane );
              drp_write(0x0 , RX_INT_DATAWIDTH , curr_lane );
              drp_write(0x2 , TX_DATA_WIDTH , curr_lane );
              drp_write(0x2 , RX_DATA_WIDTH , curr_lane );
#endif
#if TURN_ON_CONFIG_REGISTERS
              drp_write(0xFFFF, ES_SDATA_MASK0, curr_lane);
              drp_write(0x00FF, ES_SDATA_MASK1, curr_lane);
#endif
              }
              break;
          default:{
#if TURN_ON_CONFIG_REGISTERS
              drp_write(0xFFFF, ES_SDATA_MASK0, curr_lane);
              drp_write(0xFFFF, ES_SDATA_MASK1, curr_lane);
#endif
              }
        }

    //Write SDATA_MASK2-4 attributes.  Values are same for all data widths (16, 20,32,40)
    drp_write(0xFF00, ES_SDATA_MASK2, curr_lane);
    for(i=ES_SDATA_MASK3; i <= ES_SDATA_MASK4; i++){
            drp_write(0xFFFF, i, curr_lane);
    }

    if( DEBUG ) xil_printf( "fix prescale\n");
    //Write ES_PRESCALE attribute to 0
    drp_write(0x0000, ES_PRESCALE, curr_lane);

    //Write ES_QUAL_MASK attribute to all 1's
    for(i = ES_QUAL_MASK0; i <= ES_QUAL_MASK4; i++){
        drp_write(0xFFFF, i, curr_lane);
    }

#if MODIFY_DRP_REGISTERS
    drp_write_raw( 0x17c , 0x04C , 0 , 9 , curr_lane ); // CHAN_BOND_SEQ_1_1
    drp_write_raw( 0x100 , 0x04D , 0 , 9 , curr_lane ); // CHAN_BOND_SEQ_1_2
    drp_write_raw( 0x100 , 0x04E , 0 , 9 , curr_lane ); // CHAN_BOND_SEQ_1_3
    drp_write_raw( 0x100 , 0x04F , 0 , 9 , curr_lane ); // CHAN_BOND_SEQ_1_3
    drp_write_raw( 0x100 , 0x050 , 0 , 9 , curr_lane ); // CHAN_BOND_SEQ_2_1
    drp_write_raw( 0x100 , 0x051 , 0 , 9 , curr_lane ); // CHAN_BOND_SEQ_2_2
    drp_write_raw( 0x100 , 0x052 , 0 , 9 , curr_lane ); // CHAN_BOND_SEQ_2_3
    drp_write_raw( 0x100 , 0x053 , 0 , 9 , curr_lane ); // CHAN_BOND_SEQ_2_4
    drp_write_raw( 0x13 , 0x045 , 10 , 15 , curr_lane ); // CLK_COR_MAX_LAT
    drp_write_raw( 0xf , 0x046 , 10 , 15 , curr_lane ); // CLK_COR_MIN_LAT
    drp_write_raw( 0x11c , 0x044 , 0 , 9 , curr_lane ); // CLK_COR_SEQ_1_1
    drp_write_raw( 0xf , 0x044 , 10 , 13 , curr_lane ); // CLK_COR_SEQ_1_ENABLE
    drp_write_raw( 0x100 , 0x045 , 0 , 9 , curr_lane ); // CLK_COR_SEQ_1_2
    drp_write_raw( 0x100 , 0x046 , 0 , 9 , curr_lane ); // CLK_COR_SEQ_1_3
    drp_write_raw( 0x100 , 0x047 , 0 , 9 , curr_lane ); // CLK_COR_SEQ_1_4
// #if 0
    drp_write_raw( 0x1 , 0x03D , 15 , 15 , curr_lane ); // RX_DISPERR_SEQ_MATCH
    drp_write_raw( 0x1 , 0x052 , 11 , 11 , curr_lane ); // CBCC_DATA_SOURCE_SEL
    drp_write_raw( 0x3c , 0x1a , 0 , 7 , curr_lane ); // PD_TRANS_TIME_NONE_P2
    drp_write_raw( 0x7 , 0x53 , 12 , 15 , curr_lane ); // CHAN_BOND_MAX_SKEW
    drp_write_raw( 0xa01 , 0x86 , 0 , 15 , curr_lane ); // DMONITOR_CFG
    drp_write_raw( 0xf , 0x51 , 12 , 15 , curr_lane ); // FTS_LANE_DESKEW_CFG
    drp_write_raw( 0xf , 0x52 , 12 , 15 , curr_lane ); // FTS_DESKEW_SEQ_ENABLE
    drp_write_raw( 0x4 , 0x9c , 0 , 5 , curr_lane ); // RXBUF_THRESH_UNDFLW
    drp_write_raw( 0x0 , 0x13 , 0 , 2 , curr_lane ); // SATA_EIDLE_CFG
    drp_write_raw( 0x0 , 0x3d , 14 , 14 , curr_lane ); // DEC_PCOMMA_DETECT
    drp_write_raw( 0x0 , 0x3d , 12 , 12 , curr_lane ); // DEC_VALID_COMMA_ONLY
    drp_write_raw( 0x0 , 0x11 , 0 , 0 , curr_lane ); // RXPRBS_ERR_LOOPBACK
    drp_write_raw( 0x0 , 0x3d , 13 , 13 , curr_lane ); // DEC_MCOMMA_DETECT
    drp_write_raw( 0x0 , 0x13 , 0 , 2 , curr_lane ); // SATA_EIDLE_CFG
    drp_write_raw( 0x64 , 0x1a , 8 , 15 , curr_lane ); // PD_TRANS_TIME_TO_P2
    //drp_write_raw( 0x1c0 , 0x5f , 0 , 15 , curr_lane ); // CPLL_LOCK_CFG
// #endif
    drp_write_raw( 0x10 , 0x074 , 11 , 15 , curr_lane ); // RX_DFE_KL_CFG2
    drp_write_raw( 0x6 , 0x007f , 10 , 14 , curr_lane ); // RX_DFE_KL_CFG2
    drp_write_raw( 0xc , 0x7f , 0 , 3 , curr_lane ); // RX_DFE_KL_CFG2
#if FREQUENCY == 125
    drp_write_raw( 0x4 , 0x11 , 1 , 3 , curr_lane ); // RX_CM_TRIM
    drp_write_raw( 0x4008 , 0x0a9 , 0 , 15 , curr_lane ); // RXCDR_CFG
#elif FREQUENCY == 625
    drp_write_raw( 0x1020 , 0x0a9 , 0 , 15 , curr_lane ); // RXCDR_CFG
#elif FREQUENCY == 640
    drp_write_raw( 0x4 , 0x11 , 1 , 3 , curr_lane ); // RX_CM_TRIM
    drp_write_raw( 0xb , 0xac , 0 , 15 , curr_lane ); // RXCDR_CFG
#endif
//     drp_write_raw( 0x8000 , 0x0ab , 0 , 15 , curr_lane ); // RXCDR_CFG
#endif

    return TRUE;
}

int init_eye_scan(eye_scan* p_lane, u8 curr_lane) {
    u8 i;

    //Scan parameters should already be loaded into the structs by host pc

    if(p_lane->enable == FALSE){
      p_lane->state = WAIT_STATE;
      return FALSE;
    }

    if( DEBUG ) xil_printf( "starting init_eye_scan\n");

    configure_eye_scan( p_lane , curr_lane );

    /* And then decide whether or not to enable eyescan circuitry port. */

    if( DEBUG ) xil_printf( "enable eyescan circuitry\n");
    u16 esval = 0x2070; // Enable the eyescan circuity.
    drp_write(esval , PMA_RSV2 , curr_lane );
    u16 valrsv2 = drp_read( PMA_RSV2 , curr_lane );
    if( DEBUG ) xil_printf("PMA_RSV2 = 0x%04x\n",valrsv2);
    if( esval != valrsv2 )
        xil_printf("ERROR: Failed to write PMA_RSV2 with expected value\n");

#if DEBUG
    u32 monreg = xaxi_eyescan_read_channel_reg(curr_lane,XAXI_EYESCAN_MONITOR);
    xil_printf("Channel %d: Monitor register: %08lx\n",curr_lane,monreg);
    u32 errcnt = monreg & 0x80FF;
    xil_printf("Channel %d: Error Count: %08lx\n",curr_lane,errcnt);

    u32 read1 = xaxi_eyescan_read_channel_reg(curr_lane,XAXI_EYESCAN_RESET);
    xil_printf("Channel %d: Reset register(init): %08x\n",curr_lane,read1);
#endif

    if( DEBUG ) xil_printf( "do resets\n");
    xaxi_eyescan_write_channel_reg(curr_lane, XAXI_EYESCAN_TXCFG, 1 | ( 1 << 8 ) );
    xaxi_eyescan_write_channel_reg(curr_lane, XAXI_EYESCAN_RXCFG, 1 | ( 1 << 8 ) );
    xaxi_eyescan_reset_channel(curr_lane);

#if DEBUG
    u32 txuserready = xaxi_eyescan_read_channel_reg( curr_lane , XAXI_EYESCAN_TXCFG );
    u32 rxuserready = xaxi_eyescan_read_channel_reg( curr_lane , XAXI_EYESCAN_RXCFG );
    xil_printf( "txuserready 0x%08x, rxuserready 0x%08x\n" , txuserready , rxuserready );

    monreg = xaxi_eyescan_read_channel_reg(curr_lane,XAXI_EYESCAN_MONITOR);
    xil_printf("Channel %d: Monitor register: %08lx\n",curr_lane,monreg);
    errcnt = monreg & 0x80FF;
    xil_printf("Channel %d: Error Count: %08lx\n",curr_lane,errcnt);
#endif

    sleep(200);
    if( DEBUG ) xil_printf( "leaving init_eye_scan\n");

    return TRUE;
}

void global_reset_eye_scan() {
	u32 rstval = 0x0;
	//rstval = ( 1 << 3  | 1 << 4 );
	//xaxi_eyescan_write_global(XAXI_EYESCAN_GLOBAL_RESET,rstval);
	//xaxi_eyescan_write_global(XAXI_EYESCAN_GLOBAL_RESET,0x0);
	//rstval = ( 1 << 1 | 1 << 2 | 1 << 3  | 1 << 4 );
	//rstval = ( 1 << 1 | 1 << 4 );
    //xaxi_eyescan_write_global(XAXI_EYESCAN_GLOBAL_RESET,rstval);
	rstval = ( 1 << 1 );
    xaxi_eyescan_write_global(XAXI_EYESCAN_GLOBAL_RESET,rstval);
}

void *es_controller_thread(char * arg) {
    xil_printf( "starting es_controller_thread\n");

    // Global initialization
    global_reset_eye_scan();

    u32 n_gtx = xaxi_eyescan_read_global(XAXI_EYESCAN_NGTX);
    u32 n_quad = n_gtx >> 8;
    n_gtx &= 0x00FF;
    xil_printf( "n_gtx %d\n" , n_gtx );
    xil_printf( "n_quad %d\n" , n_quad );

    //Eye scan data structure
    u32 num_lanes = n_gtx;
    u8 curr_lane , is_all_ready;

    if( num_lanes > MAX_NUMBER_OF_LANES ) {
        xil_printf( "Too many channels, %d given %d allowed\n" , num_lanes , MAX_NUMBER_OF_LANES );
        exit(0);
    }

    for( curr_lane=0; curr_lane<num_lanes; curr_lane++ ) {
        eye_scan_lanes[curr_lane] = malloc( sizeof(eye_scan) );
        memset( eye_scan_lanes[curr_lane] , 0 , sizeof(eye_scan) );
        init_eye_scan_struct( eye_scan_lanes[curr_lane] );
    }

    // Turn off all channels
    u32 i;
    for (i = 0; i < n_gtx; ++i)
    	xaxi_eyescan_disable_channel(i);

    if( DEBUG ) xil_printf( "memory initialized\n");

    //Main Loop
    while (1) {
        for(curr_lane=0; curr_lane<num_lanes ;curr_lane++){
            //Start a new scan if it's not currently running
            if(eye_scan_lanes[curr_lane]->initialized == FALSE) {
                eye_scan_lanes[curr_lane]->initialized = init_eye_scan(eye_scan_lanes[curr_lane], curr_lane);//Initialize scan parameters
                continue;
            }

            if( do_global_run_eyescan == FALSE )
                continue;

            if( RUN_ES_ACQUISITION ) es_simple_eye_acq(eye_scan_lanes[curr_lane]);

            if( eye_scan_lanes[curr_lane]->state == DONE_STATE && eye_scan_lanes[curr_lane]->p_upload_rdy == FALSE ) {
                xil_printf( "scan done lane %d\n" , curr_lane );
                if( RUN_ES_ACQUISITION ) eye_scan_lanes[curr_lane]->p_upload_rdy = TRUE;
            }
        }
        is_all_ready = TRUE;
        for( curr_lane = 0 ; curr_lane < num_lanes ; curr_lane++ ) {
            if( eye_scan_lanes[curr_lane]->enable == FALSE ) // If channel isn't enabled, don't worry about whether or not its ready to upload...
                continue;
            if( eye_scan_lanes[curr_lane]->initialized == FALSE )
                is_all_ready = FALSE;
            if( eye_scan_lanes[curr_lane]->p_upload_rdy == FALSE )
                is_all_ready = FALSE;
        }
        is_global_upload_ready = is_all_ready;
    }
    free(eye_scan_lanes);
}

void eyescan_global_debug( char * dbgstr ) {
    safe_sprintf( dbgstr , "Global registers:\r\n" );

    safe_sprintf( dbgstr , "%sn_gtx        0x%08lx\r\n" , dbgstr , xaxi_eyescan_read_global(XAXI_EYESCAN_NGTX) );
    safe_sprintf( dbgstr , "%sn_left       0x%08lx\r\n" , dbgstr , xaxi_eyescan_read_global(XAXI_EYESCAN_NLEFT) );
    safe_sprintf( dbgstr , "%sn_right      0x%08lx\r\n" , dbgstr , xaxi_eyescan_read_global(XAXI_EYESCAN_NRIGHT) );
    safe_sprintf( dbgstr , "%sqpll_lock    0x%08lx\r\n" , dbgstr , xaxi_eyescan_read_global(XAXI_EYESCAN_QPLL_LOCK) );
    safe_sprintf( dbgstr , "%sqpll_lost    0x%08lx\r\n" , dbgstr , xaxi_eyescan_read_global(XAXI_EYESCAN_QPLL_LOST) );
    safe_sprintf( dbgstr , "%sglobal_reset 0x%08lx\r\n" , dbgstr , xaxi_eyescan_read_global(XAXI_EYESCAN_GLOBAL_RESET) );

    if( DEBUG ) xil_printf( "got here %s\n",dbgstr);

#ifdef XAXI_EYESCAN_BASEFREQ_COUNT
    /* Check the frequency counting logic on channel 0 only, but in the global space. */
  u32 grd = xaxi_eyescan_read_global(XAXI_EYESCAN_GLOBAL_RESET);
  xaxi_eyescan_write_global(XAXI_EYESCAN_GLOBAL_RESET,grd | 0x200);  // Clear it
  xaxi_eyescan_write_global(XAXI_EYESCAN_GLOBAL_RESET,grd | 0x100);  // Enable it (and remove the clear)
  sleep(100);   // Probably long enough to halt with max count
  xaxi_eyescan_write_global(XAXI_EYESCAN_GLOBAL_RESET,grd);  // Disable it (w/out clearing it)
  safe_sprintf( dbgstr , "%sXAXI_EYESCAN_BASEFREQ_COUNT 0x%04x\r\n" , dbgstr , xaxi_eyescan_read_global(XAXI_EYESCAN_BASEFREQ_COUNT) );
  safe_sprintf( dbgstr , "%sXAXI_EYESCAN_FREQ0_COUNT    0x%04x\r\n" , dbgstr , xaxi_eyescan_read_global(XAXI_EYESCAN_FREQ0_COUNT) );
  safe_sprintf( dbgstr , "%sXAXI_EYESCAN_FREQ1_COUNT    0x%04x\r\n" , dbgstr , xaxi_eyescan_read_global(XAXI_EYESCAN_FREQ1_COUNT) );
  safe_sprintf( dbgstr , "%sXAXI_EYESCAN_FREQ2_COUNT    0x%04x\r\n" , dbgstr , xaxi_eyescan_read_global(XAXI_EYESCAN_FREQ2_COUNT) );
  safe_sprintf( dbgstr , "%sXAXI_EYESCAN_FREQ3_COUNT    0x%04x\r\n" , dbgstr , xaxi_eyescan_read_global(XAXI_EYESCAN_FREQ3_COUNT) );
  xaxi_eyescan_write_global(XAXI_EYESCAN_GLOBAL_RESET,grd | 0x100);  // Enable it (and remove the clear)

  if( DEBUG ) xil_printf( "and here %s\n",dbgstr);
#endif

    return;
}

void eyescan_debugging( int lane , char * dbgstr ) {
    if( lane < 0 ) {
        eyescan_global_debug( dbgstr );
    }
    else {
        safe_sprintf( dbgstr , "%seyescan_reset   0x%08lx\r\n" , dbgstr , xaxi_eyescan_read_channel_reg( lane , XAXI_EYESCAN_RESET ) );
        safe_sprintf( dbgstr , "%seyescan_txcfg   0x%08lx\r\n" , dbgstr , xaxi_eyescan_read_channel_reg( lane , XAXI_EYESCAN_TXCFG ) );
        safe_sprintf( dbgstr , "%seyescan_rxcfg   0x%08lx\r\n" , dbgstr , xaxi_eyescan_read_channel_reg( lane , XAXI_EYESCAN_RXCFG ) );
        safe_sprintf( dbgstr , "%seyescan_clkcfg  0x%08lx\r\n" , dbgstr , xaxi_eyescan_read_channel_reg( lane , XAXI_EYESCAN_CLKCFG ) );
        safe_sprintf( dbgstr , "%seyescan_monitor 0x%08lx\r\n" , dbgstr , xaxi_eyescan_read_channel_reg( lane , XAXI_EYESCAN_MONITOR ) );
        safe_sprintf( dbgstr , "%seyescan_cursor  0x%08lx\r\n" , dbgstr , xaxi_eyescan_read_channel_reg( lane , XAXI_EYESCAN_CURSOR ) );

        safe_sprintf( dbgstr , "%ses_control        0x%04x\r\n" , dbgstr , drp_read( ES_CONTROL , lane ) );
        safe_sprintf( dbgstr , "%ses_horz_offset    0x%04x\r\n" , dbgstr , drp_read( ES_HORZ_OFFSET , lane ) );
        safe_sprintf( dbgstr , "%ses_prescale       0x%04x\r\n" , dbgstr , drp_read( ES_PRESCALE , lane ) );
        safe_sprintf( dbgstr , "%ses_vert_offset    0x%04x\r\n" , dbgstr , drp_read( ES_VERT_OFFSET , lane ) );
        safe_sprintf( dbgstr , "%ses_control_status 0x%04x\r\n" , dbgstr , drp_read( ES_CONTROL_STATUS , lane ) );
        safe_sprintf( dbgstr , "%ses_error_count    0x%04x\r\n" , dbgstr , drp_read( ES_ERROR_COUNT , lane ) );
        safe_sprintf( dbgstr , "%ses_sample_count   0x%04x\r\n" , dbgstr , drp_read( ES_SAMPLE_COUNT , lane ) );
        safe_sprintf( dbgstr , "%ses_eyescan_en     0x%04x\r\n" , dbgstr , drp_read( ES_EYESCAN_EN , lane ) );
        safe_sprintf( dbgstr , "%ses_errdet_en      0x%04x\r\n" , dbgstr , drp_read( ES_ERRDET_EN , lane ) );
        safe_sprintf( dbgstr , "%ses_sdata_mask0    0x%04x\r\n" , dbgstr , drp_read( ES_SDATA_MASK0 , lane ) );
        safe_sprintf( dbgstr , "%ses_sdata_mask1    0x%04x\r\n" , dbgstr , drp_read( ES_SDATA_MASK1 , lane ) );
        safe_sprintf( dbgstr , "%ses_sdata_mask2    0x%04x\r\n" , dbgstr , drp_read( ES_SDATA_MASK2 , lane ) );
        safe_sprintf( dbgstr , "%ses_sdata_mask3    0x%04x\r\n" , dbgstr , drp_read( ES_SDATA_MASK3 , lane ) );
        safe_sprintf( dbgstr , "%ses_sdata_mask4    0x%04x\r\n" , dbgstr , drp_read( ES_SDATA_MASK4 , lane ) );
        safe_sprintf( dbgstr , "%ses_qual_mask0     0x%04x\r\n" , dbgstr , drp_read( ES_QUAL_MASK0 , lane ) );
        safe_sprintf( dbgstr , "%ses_qual_mask1     0x%04x\r\n" , dbgstr , drp_read( ES_QUAL_MASK1 , lane ) );
        safe_sprintf( dbgstr , "%ses_qual_mask2     0x%04x\r\n" , dbgstr , drp_read( ES_QUAL_MASK2 , lane ) );
        safe_sprintf( dbgstr , "%ses_qual_mask3     0x%04x\r\n" , dbgstr , drp_read( ES_QUAL_MASK3 , lane ) );
        safe_sprintf( dbgstr , "%ses_qual_mask4     0x%04x\r\n" , dbgstr , drp_read( ES_QUAL_MASK4 , lane ) );
        safe_sprintf( dbgstr , "%spma_rsv2          0x%04x\r\n" , dbgstr , drp_read( PMA_RSV2 , lane ) );
    }
    return;
}

void eyescan_debug_addr( int lane , u32 drp_addr , char * dbgstr ) {
    safe_sprintf( dbgstr , "%slane %d addr 0x%04x val 0x%04x\r\n" , dbgstr , lane , drp_addr , \
            xaxi_eyescan_read_channel_drp( lane , drp_addr ) );
    return;
}

