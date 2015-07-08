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
#include "xil_types.h"
//#include "xiomodule.h"

#define IS_OTC_BOARD

#define DONE_STATE         0x0001
#define WAIT_STATE         0x0000
#define RESET_STATE        0x0010
#define SETUP_STATE        0x0020
#define COUNT_STATE        0x0030

#define LPM_MODE 0
#define DFE_MODE 1

#define MAX_NUMBER_OF_LANES 48

#define NUM_PIXELS_TOTAL 4096 //maximum number of pixels assuming vert/horz step sizes = 1, "hex" rate used (max_horz_offset = in this case)

typedef struct {
    //Eye scan data:
    u16 error_count;     //Error count for each pixel/eye location
    u16 sample_count;     //Sample count for each pixel/eye location
    s16 h_offset;        //offset of each pixel.
    s16 v_offset;        //offset of each pixel.
    s16 ut_sign;            //UT sign of each pixel
    u8 prescale;   //prescale of each pixel
    u8 center_error; // Eyescan (0,0) error count from frame checker
} eye_scan_pixel;

typedef struct {
    u8 enable; //enable this lane
    u8 initialized; // have we been initialized?
  
    //Scan parameters defined by user:
    u8  lpm_mode;           //Equalizer mode: 1 for LPM. 0 for DFE
    u8  horz_step_size;     //Horizontal scan step size
    u8  vert_step_size;     //Vertical scan step size
    u8  max_prescale;       //Maximum prescale value (for dynamic prescaling)
    u16 max_horz_offset;    //Maximum horizontal offset value. Depends on rate mode (e.g. full, half, etc)
    u16 data_width;         //Data width

    //Eye scan data:
    eye_scan_pixel * pixels; // array of pointers, allocate memory for each pixel as required
    s16 horz_offset;        //Horizontal offset for current pixel
    s16 vert_offset;        //Vertical offset for current pixel
    s16 ut_sign;            //UT sign for current pixel
    u8  prescale;           //Prescale for current pixel

    //Status indicators:
    u8 p_upload_rdy;        //Flag to indicate PC should upload data from BRAM
    u16 state;              //State of es_simple_eye_acq
    u16 pixel_count;        //Count of number of pixels currently stored in the struct

    //Names:
    u8  lane_number;
} eye_scan;

eye_scan * get_eye_scan_lane( int lane );

//void write_es_data (eye_scan* p_lane, XIOModule* p_io_mod, u16 lane_offset);
int configure_eye_scan(eye_scan* p_lane, u8 lane_offset);
int init_eye_scan(eye_scan* p_lane, u8 lane_offset);
void global_reset_eye_scan();
void global_run_eye_scan();
u8 global_upload_ready();

void global_stop_eye_scan();
void global_upload_unrdy();

void eyescan_lock();
void eyescan_unlock();

void eyescan_global_debug( char * dbgstr );
void eyescan_debugging( int lane , char * dbgstr );
void eyescan_debug_addr( int lane , u32 drp_addr , char * dbgstr );
