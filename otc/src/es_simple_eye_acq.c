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

/*
 * Please refer to Matlab code for code comments and description.
 */

/* Include files */
#include "es_controller.h"
//#include "xiomodule.h"
#include "drp.h"
#include "es_simple_eye_acq.h"
#include "xaxi_eyescan.h"
#include <math.h>

#define DEBUG FALSE
#define USE_DRP_RX_PRBS_ERR_CNT TRUE

double ** pixel_ber_tables;
int * central_err_cnts;
double * central_samp_cnts;

//Find largest multiple of vert_step_size that is less than 127
u16 get_max_vert_offset(u16 vert_step_size) {
	u16 max_vert_offset = 0;
	while(max_vert_offset < 127){
		max_vert_offset += vert_step_size;
	}
	max_vert_offset -= vert_step_size;
	return max_vert_offset;
}

void init_monitor_tables(eye_scan * eye_struct, u16 max_horz_offset, u16 max_vert_offset) {
	// Create ber tables for webpage if necessary
	if (pixel_ber_tables == NULL) {
		pixel_ber_tables = malloc(sizeof(double*) * MAX_NUMBER_OF_LANES);
		central_err_cnts = malloc(sizeof(int) * MAX_NUMBER_OF_LANES);
		central_samp_cnts = malloc(sizeof(double) * MAX_NUMBER_OF_LANES);

		int rowsize = 2 * max_horz_offset / eye_struct->horz_step_size + 1;
		int colsize = 2 * max_vert_offset / eye_struct->vert_step_size + 1;
		int i, j;
		for (i = 0; i < MAX_NUMBER_OF_LANES; ++i) {
			pixel_ber_tables[i] = malloc(sizeof(double) * rowsize * colsize);
			central_err_cnts[i] = 0;
			central_samp_cnts[i] = 0;
			for(j = 0; j < rowsize * colsize; ++j)
				pixel_ber_tables[i][j] = NAN;
		}
	} else {
		xil_printf("Warning: called init_monitor_tables() with tables already initialized\n");
	}
}

void clear_monitor_tables() {
	if (pixel_ber_tables != NULL) {
		int i;
		for(i = 0; i < MAX_NUMBER_OF_LANES; ++i) {
			free(pixel_ber_tables[i]);
			pixel_ber_tables[i] = NULL;
		}
		free(pixel_ber_tables);
		pixel_ber_tables = NULL;
	}
	if (central_err_cnts != NULL) {
		free(central_err_cnts);
		central_err_cnts = NULL;
	}
	if(central_samp_cnts != NULL) {
		free(central_samp_cnts);
		central_samp_cnts = NULL;
	}
}

void es_simple_eye_acq(eye_scan *eye_struct)
{
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% Does nothing but return.
	//%%%%%%%%%%%%%%%%%%  WAIT State (es_simple_eye_acq)  %%%%%%%%%%%%%%%%%%% Waits for RESET state
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% to be written externally.
	if(eye_struct->state == WAIT_STATE || eye_struct->state == DONE_STATE) { //(WAIT x0000 = 0, DONE = 1)
		if( DEBUG ) printf( "did we get here?\n" );
		return;
	}

	if( DEBUG ) printf( "start es_simple_eye_acq\n");

	u16 vert_step_size = eye_struct->vert_step_size;
	u16 max_vert_offset = 0;
	u16 max_horz_offset = eye_struct->max_horz_offset;
	u16 data_width = eye_struct->data_width;
	//lpm_mode: Zero for LPM & 1 for DFE
	u8  max_ut_sign = 1 - eye_struct->lpm_mode;

	//Gear-shifting-related
	u16 min_error_count = 3;
	u8  max_prescale = eye_struct->max_prescale;
	u8  step_prescale = 3;
	s16 next_prescale = 0;

	u16 es_status = 0;
	u16 error_count = 0;
	u16 sample_count = 0;
	u16 es_state = 0;
	u16 es_done = 0;
	u16 prescale = 0;
	u32 current_center_error = 0;
	u8 previous_center_error = 0;

	u16 horz_value = 0;
	u16 vert_value = 0;
	u16 phase_unification = 0;

	//Find largest multiple of vert_step_size that is less than 127
	max_vert_offset = get_max_vert_offset(vert_step_size);

	//Read DONE bit & state of Eye Scan State Machine
	if( DEBUG ) printf( "es_simple_eye_acq read es_control_status lane %d\n" , eye_struct->lane_number );
	es_status = drp_read(ES_CONTROL_STATUS, eye_struct->lane_number);
	if( DEBUG ) printf( "lane %d es_status bit 0x%x\n" , eye_struct->lane_number , es_status );
	es_state = es_status >> 1;
	es_done = es_status & 1;
	if( DEBUG ) printf( "es_state 0x%x es_done 0x%x\n" , es_state , es_done );

	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% Initializes parameters for
	//%%%%%%%%%%%%%%%%%%  RESET State (es_simple_eye_acq)  %%%%%%%%%%%%%%%%%% statistical eye scan
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% and sets ES_PRESCALE
	// Called with RESET state when eye acquisition is to be started.
	if(eye_struct->state == RESET_STATE) {  //(RESET x0010 = 16)
		//Eye Scan SM must be in WAIT state with RUN & ARM deasserted
		if(es_state != 0) {
			return;
		}
		if(eye_struct->lpm_mode == 0) {
			eye_struct->ut_sign = 1;
		} else {
			eye_struct->ut_sign = 0;
		}

		// Monitor tables
		clear_monitor_tables();
		init_monitor_tables(eye_struct, max_horz_offset, max_vert_offset);

		//Starts in center of eye
		eye_struct->horz_offset = 0;
		eye_struct->vert_offset = -max_vert_offset - vert_step_size; //Incremented to -max_vert_offset in SETUP state

		//Gear Shifting start
		drp_write(eye_struct->prescale, ES_PRESCALE, eye_struct->lane_number);
		//Gear Shifting end
		eye_struct->state = SETUP_STATE; //SETUP

	}

	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% Increments vert_offset,
	//%%%%%%%%%%%%%%%%%%  SETUP State (es_simple_eye_acq)  %%%%%%%%%%%%%%%%%% horz_offset and ut_sign
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	if(eye_struct->state == SETUP_STATE) {  //(SETUP x0020 = 32)
		if( eye_struct->pixel_count >= NUM_PIXELS_TOTAL ) {
			eye_struct->state = DONE_STATE;
			return;
		}
		//Advance vert_offset (-max to max)
		eye_struct->vert_offset = eye_struct->vert_step_size + eye_struct->vert_offset;

		//If done scanning pixel column
		if(eye_struct->vert_offset > max_vert_offset) {
			//Reset to -max_vert_offset
			eye_struct->vert_offset = -max_vert_offset;

			//Advance(decrement) ut_sign
			eye_struct->ut_sign = eye_struct->ut_sign - 1;

			//If completed 1 & 0 for DFE (or just 0 for LPM), reset to max
			if(eye_struct->ut_sign < 0){
				eye_struct->ut_sign = max_ut_sign;

				//Advance horz_offset
				if(eye_struct->horz_offset < 0){
					eye_struct->horz_offset = -eye_struct->horz_offset;
				} else {
					//Increments thru sequence: 0, -step, step, -2*step, 2*step, -3*step, 3*step, ...
					eye_struct->horz_offset = -(eye_struct->horz_step_size + eye_struct->horz_offset);
				}

				//If incremented thru all pixel columns, reset to 0 and set to DONE state, which acts like WAIT state in this function
				if(eye_struct->horz_offset < -max_horz_offset){
					eye_struct->horz_offset = 0;
					eye_struct->state = DONE_STATE;
					return;
				}
			}
		}
		//Convert to 'phase_unification' + two's complement
		if(eye_struct->horz_offset < 0) {
			horz_value = eye_struct->horz_offset + 2048; //Generate 11-bit 2's complement number
			phase_unification = 2048; //12th bit i.e. bit[11]
		} else {
			horz_value = eye_struct->horz_offset;
			phase_unification = 0;
		}
		horz_value  = phase_unification + horz_value;
		//Write horizontal offset
		drp_write(horz_value, ES_HORZ_OFFSET,eye_struct->lane_number);

		//ES_VERT_OFFSET[8] is UT_sign
		vert_value = abs(eye_struct->vert_offset) + eye_struct->ut_sign * 256; //256 = 2^8 ut_sign is bit[8]
		if(eye_struct->vert_offset < 0) {
			vert_value = vert_value + 128;//128=2^7 sign is bit [7]
		}

		//Write vertical offset
		drp_write(vert_value, ES_VERT_OFFSET, eye_struct->lane_number);

		//xaxi_eyescan_write_channel_reg(eye_struct->lane_number, XAXI_EYESCAN_RESET, (1<<14|1<<7) );

		//printf("PRESS RETURN %d\n",eye_struct->lane_number);
		//char empty_buffer[1024];
		//scanf("%s",empty_buffer);

		//xaxi_eyescan_write_channel_reg(eye_struct->lane_number, XAXI_EYESCAN_RESET, 0);

		//         xaxi_eyescan_write_channel_reg(eye_struct->lane_number, XAXI_EYESCAN_RESET, 0x80 );
		//         xaxi_eyescan_write_channel_reg(eye_struct->lane_number, XAXI_EYESCAN_RESET, 0);


		//Assert RUN bit to start error and sample counters
		drp_write(1, ES_CONTROL, eye_struct->lane_number);

		//Transition to 'COUNT' state, allow other operations while errors & samples accumulate
		eye_struct->state = COUNT_STATE;

		//if( eye_struct->pixel_count == 10 && eye_struct->lane_number == 5 ) {
		//	sleep(10);
		//	xaxi_eyescan_write_channel_reg(eye_struct->lane_number, XAXI_EYESCAN_TXCFG, 1 | ( 1 << 8 ) | ( 1 << 11 ) );
		//}

		//xaxi_eyescan_write_channel_reg(eye_struct->lane_number, XAXI_EYESCAN_RESET, 0x80 );
		//xaxi_eyescan_write_channel_reg(eye_struct->lane_number, XAXI_EYESCAN_RESET, 0);

		return;
	}

	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% When acquisition finished, reads
	//%%%%%%%%%%%%%%%%%%  COUNT State (es_simple_eye_acq)  %%%%%%%%%%%%%%%%%% error & sample counts and prescale.
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% Adjusts prescale as needed.

	if(eye_struct->state == COUNT_STATE) { // (COUNT x0030 = 48)
		//If acquisition is not finished, return
		if(es_done == 0){
#if USE_DRP_RX_PRBS_ERR_CNT
			current_center_error = drp_read_raw( DRP_RX_PRBS_ERR_CNT , 0 , 15 , eye_struct->lane_number );
#else
			current_center_error = xaxi_eyescan_read_channel_reg(eye_struct->lane_number,XAXI_EYESCAN_MONITOR);
			//if( current_center_error >> 14 & 1 ) printf( "monitor register 0 %d 0x%08lx\n" , eye_struct->lane_number , current_center_error );
			current_center_error = current_center_error & 0xFF;
#endif
			return;
		}
		//If DONE, deassert RUN bit to transition to Eye Scan WAIT state
		drp_write(0, ES_CONTROL, eye_struct->lane_number);

		//Read error, sample counts & prescale used for acquisition
		error_count = drp_read(ES_ERROR_COUNT, eye_struct->lane_number);
		sample_count = drp_read(ES_SAMPLE_COUNT, eye_struct->lane_number);
		prescale = drp_read(ES_PRESCALE, eye_struct->lane_number);
#if USE_DRP_RX_PRBS_ERR_CNT
		current_center_error = drp_read_raw( DRP_RX_PRBS_ERR_CNT , 0 , 15 , eye_struct->lane_number );
#else
		current_center_error = xaxi_eyescan_read_channel_reg(eye_struct->lane_number,XAXI_EYESCAN_MONITOR);
		//if( current_center_error >> 14 & 1 ) printf( "monitor register 0 %d 0x%08lx\n" , eye_struct->lane_number , current_center_error );
		current_center_error = current_center_error & 0xFF;
#endif
		if( DEBUG ) printf( "current prescale read from drp %d\n" , prescale );
		if(error_count < (10*min_error_count) || error_count > (1000*min_error_count)){
			if (prescale < max_prescale && error_count < 10*min_error_count){
				next_prescale = prescale+step_prescale;
				if(next_prescale > max_prescale) {
					next_prescale = max_prescale;
				}

				//Restart measurement if too few errors counted
				if (error_count < min_error_count){
					drp_write(next_prescale, ES_PRESCALE, eye_struct->lane_number);

					//Assert RUN bit to restart sample & error counters. Remain in COUNT state.
					drp_write(1, ES_CONTROL, eye_struct->lane_number);
					return;
				}
			}
			else if(prescale > 0 && error_count > 10*min_error_count){
				if (error_count > 1000*min_error_count){
					next_prescale = prescale - 2*step_prescale;
				} else {
					next_prescale = prescale - step_prescale;
				}

				if(next_prescale < 0){
					next_prescale = 0;
				}
			}else{
				next_prescale   = prescale;
			}
			drp_write(next_prescale,ES_PRESCALE, eye_struct->lane_number);
		}

		//sleep(100);


		if( DEBUG ) printf( "lane %d pixel_count %d error_count %d sample_count %d prescale %d\n" , eye_struct->lane_number , eye_struct->pixel_count , error_count , sample_count , prescale );
		eye_struct->prescale = prescale;

		// Store information about current pixel
		eye_scan_pixel * current_pixel = ( eye_struct->pixels + eye_struct->pixel_count );
		current_pixel->error_count = error_count;
		current_pixel->sample_count = sample_count;
		current_pixel->h_offset = eye_struct->horz_offset;
		current_pixel->v_offset = eye_struct->vert_offset;
		current_pixel->ut_sign = eye_struct->ut_sign;
		current_pixel->prescale = eye_struct->prescale;

		if( eye_struct->pixel_count != 0 ) {
			previous_center_error = ( eye_struct->pixels + (eye_struct->pixel_count-1) )->center_error;
		}
		//current_pixel->center_error = current_center_error - previous_center_error;
		current_pixel->center_error = current_center_error;

		// Add one pixel into the table
		int vert_idx = (current_pixel->v_offset + max_vert_offset) / eye_struct->vert_step_size;
		int horz_idx = (current_pixel->h_offset + eye_struct->max_horz_offset) / eye_struct->horz_step_size;
		int rowsize = 2 * (eye_struct->max_horz_offset / eye_struct->horz_step_size) + 1;
		int tot_samples = current_pixel->sample_count * 32 << (1 + (prescale & 0x001F));
		double errcnt = (current_pixel->error_count == 0 ? 1 : current_pixel->error_count);
		double this_ber = errcnt / (double) tot_samples;
		double prev_ber = pixel_ber_tables[eye_struct->lane_number][vert_idx * rowsize + horz_idx];
		if(prev_ber != prev_ber) // means "if current_ber is NAN" ie if this is the first scan of this pixel
			pixel_ber_tables[eye_struct->lane_number][vert_idx * rowsize + horz_idx] = this_ber;
		else
			pixel_ber_tables[eye_struct->lane_number][vert_idx * rowsize + horz_idx] = (prev_ber + this_ber) / 2;
		central_err_cnts[eye_struct->lane_number] = current_pixel->center_error; //DEBUG: IT SHOULD BE +=. DO NOT COMMIT THIS
		central_samp_cnts[eye_struct->lane_number] += tot_samples;

#if USE_DRP_RX_PRBS_ERR_CNT
		current_center_error = drp_read_raw( DRP_RX_PRBS_ERR_CNT , 0 , 15 , eye_struct->lane_number );
#else
		current_center_error = xaxi_eyescan_read_channel_reg(eye_struct->lane_number,XAXI_EYESCAN_MONITOR);
		//if( ( ( current_center_error >> 13 ) & 3 ) != 0x3 ) printf( "%d monitor register 2 %d 0x%08lx %d\n" , eye_struct->pixel_count , eye_struct->lane_number , current_center_error , ( current_center_error >> 13 ) & 3 );
		current_center_error = current_center_error & 0xFF;
#endif

		eye_struct->pixel_count++;
		if( eye_struct->pixel_count % 10 == 0 ) {
			if( DEBUG ) printf( "lane %d at pixel %d\n" , eye_struct->lane_number , eye_struct->pixel_count );
		}

		//Transition to SETUP state
		eye_struct->state = SETUP_STATE;
		return;
	}
}
