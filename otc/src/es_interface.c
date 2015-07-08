/*
 * es_interface.c
 *
 *  Created on: Jul 15, 2014
 *      Author: ddboline
 */

#include <stdio.h>
#include <string.h>

#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwipopts.h"

#include "safe_printf.h"
#include "es_controller.h"
#include "SysStatus.h"

#include "xaxi_eyescan.h"

#ifdef IS_OTC_BOARD
#include "otcLib/uPod.h"
#endif

#define NTELNETCOMMANDS 15
#define NTELNETTOKENS 20
#define RECV_BUF_SIZE 2048

u32_t u32_test_array[4] = { 0x12345678 , 0x9abcdef0 , 0x46e87ccd , 0x238e1f29 };
u8_t u8_test_array[4] = { 0x12 , 0x34 , 0x56 , 0x78 };
u16_t u16_test_array[4] = { 0x1234 , 0x5678 , 0x9abc , 0xdef0 };

int safe_send( int s, const char * ostr) {
    int retval = 0;
    retval = lwip_send(s, ostr, strlen(ostr), 0);
    return retval;
}

int es_interface(int s, const void *data, size_t size) {
    int idx = 0 , retval = 0;

    char input_buf[RECV_BUF_SIZE+1];

    // Copy the data into 'input_buf'
    memset( input_buf , 0 , RECV_BUF_SIZE+1 );
    strncpy( input_buf , data , size );

    char *commands[NTELNETCOMMANDS] = { "esinit" , "esread" , "esdone" , "esdisable" , "mwr" , "mrd" , "debug" , \
            "dbgeyescan" , "initclk" , "readclk" , "printupod" , "iicr" , "iicw" , "printtemp" , "globalinit"
    };

    if( !strncmp( input_buf , "h" , 1 ) || !strncmp( input_buf , "H" , 1 ) ) {
        memset( input_buf , 0 , RECV_BUF_SIZE+1 );
        safe_sprintf( input_buf , "commands :" );
        for( idx = 0 ; idx < NTELNETCOMMANDS ; idx++ ) {
            safe_sprintf( input_buf , "%s %s" , input_buf , commands[idx] );
        }
        safe_sprintf( input_buf , "%s\r\n" , input_buf );
        return safe_send(s, input_buf);
    }

    char * temp_str , ** pEnd = NULL;
    typedef enum { ESINIT = 0 , ESREAD = 1 , ESDONE = 2 , ESDISABLE = 3 , MWR = 4 , MRD = 5 , DEBUG = 6 , \
        DBGEYESCAN = 7 , INITCLK = 8 , READCLK = 9 , PRINTUPOD = 10 , IICR = 11 , IICW = 12 , PRINTTEMP = 13 , GLOBALINIT = 14
    } command_type_t;
    command_type_t command_type = MWR;

    char tokens[NTELNETTOKENS][20] = {};
    for( idx = 0 ; idx < NTELNETTOKENS ; idx++ ) {
        memset( tokens[idx] , 0 , 20 );
    }

    // tokenize (strtok) the input and store in tokens[]
    temp_str = strtok( input_buf , " " );
    int number_tokens = 0;
    while( temp_str != NULL ) {
        strncpy( tokens[number_tokens] , temp_str , strlen( temp_str ) );
        ++number_tokens;
        temp_str = strtok( NULL , " {},\r\n");
    }

    // identify the command
    for( idx = 0 ; idx < NTELNETCOMMANDS ; ++idx ) {
        if( !strncmp( commands[idx] , tokens[0] , strlen(commands[idx]) ) )
            command_type = idx;
    }

    if( command_type == ESINIT ) {
        if( number_tokens == 2 ) {
            if( !strncmp( "run" , tokens[1] , 3 ) ) {
                global_run_eye_scan();
                return safe_send( s , "1\r\n" );
            }
        }
        if( number_tokens != 8 ) {
            memset( input_buf , 0 , RECV_BUF_SIZE+1 );
            safe_sprintf( input_buf , "Syntax: esinit <lane> <max_prescale> <horz_step> <data_width> <vert_step> <lpm_mode> <rate>\r\n");
            return safe_send( s , input_buf );
        }

        int curr_lane = strtoul( tokens[1] , pEnd , 0);
        xaxi_eyescan_enable_channel(curr_lane);
        eyescan_lock();
        eye_scan * curr_eyescan = get_eye_scan_lane( curr_lane );
        if( curr_eyescan == NULL ) {
            return safe_send( s , "error, no lane found\r\n" );
        }
        curr_eyescan->pixel_count = 0;
        curr_eyescan->state = WAIT_STATE;
        curr_eyescan->p_upload_rdy = 0;

        // Read values in
        curr_eyescan->max_prescale = strtoul( tokens[2] , pEnd , 0);
        curr_eyescan->horz_step_size = strtoul( tokens[3] , pEnd , 0);
        curr_eyescan->data_width = strtoul( tokens[4] , pEnd , 0);
        curr_eyescan->vert_step_size = strtoul( tokens[5] , pEnd , 0);
        curr_eyescan->lpm_mode = strtoul( tokens[6] , pEnd , 0);
        curr_eyescan->max_horz_offset = strtoul( tokens[7] , pEnd , 0); // same as rate?

        //retval = configure_eye_scan( curr_eyescan , curr_lane );
        
        curr_eyescan->enable = TRUE; // enable the lane
        curr_eyescan->initialized = FALSE; // need to reinitialize lane

        eyescan_unlock();
        return 0;
    }

    if( command_type == ESREAD ) {
        memset( input_buf , 0 , RECV_BUF_SIZE+1 );
        if( number_tokens != 3 && number_tokens != 2 ) {
            safe_sprintf( input_buf , "Syntax: esread <lane> <pixel>\r\n");
            return safe_send( s , input_buf );
        }
        if( !strncmp( "all" , tokens[1] , 3 ) ) {
            if( !global_upload_ready() )
                return 0;
            int curr_lane = 0;
            for( curr_lane = 0 ; curr_lane < MAX_NUMBER_OF_LANES ; curr_lane++ ) {
                eye_scan * curr_eyescan = get_eye_scan_lane( curr_lane );
                if( curr_eyescan == NULL || curr_eyescan->enable == FALSE || curr_eyescan->p_upload_rdy == FALSE )
                    continue;
                for( idx = 0 ; idx < curr_eyescan->pixel_count ; idx++ ) {
                    eye_scan_pixel * current_pixel = ( curr_eyescan->pixels + idx );
                    safe_sprintf( input_buf , "%s%d %d %d %d: %d %d %d %d %ld\r\n" , input_buf, curr_lane , idx , \
                        current_pixel->h_offset , current_pixel->v_offset , \
                        current_pixel->error_count , current_pixel->sample_count , \
                        current_pixel->prescale & 0x001F , current_pixel->ut_sign , current_pixel->center_error );
                    if( strlen(input_buf) > 1900 ) {
                        retval = safe_send(s, input_buf);
                        memset( input_buf , 0 , RECV_BUF_SIZE+1 );
                    }
                }
            }
            if( strlen(input_buf) > 0 ) {
                retval = safe_send(s, input_buf);
            }
            return retval;
        }
        int curr_lane = strtoul( tokens[1] , pEnd , 0 );
        eye_scan * curr_eyescan = get_eye_scan_lane( curr_lane );
        if( curr_eyescan == NULL ) {
            return safe_send( s , "error, no lane found\r\n" );
        }
        if( number_tokens == 2 ) {
            safe_sprintf( input_buf , "%d\r\n" , curr_eyescan->pixel_count );
            retval = safe_send(s, input_buf);
            return retval;
        }
        else {
            int curr_pixel = strtoul( tokens[2] , pEnd , 0 );

            int begin_pixel = curr_pixel;
            int end_pixel = curr_pixel + 1;
            
            if( curr_pixel == curr_eyescan->pixel_count ) {
                begin_pixel = 0;
                end_pixel = curr_eyescan->pixel_count;
            }
            else if( curr_pixel > curr_eyescan->pixel_count ) {
                return 0;
            }
            
            for( idx = begin_pixel ; idx <= end_pixel ; idx++ ) {
                eye_scan_pixel * current_pixel = ( curr_eyescan->pixels + idx );
                safe_sprintf( input_buf , "%s%d %d %d %d: %d %d %d %d %ld\r\n" , input_buf, curr_lane , idx , \
                    current_pixel->h_offset , current_pixel->v_offset , \
                    current_pixel->error_count , current_pixel->sample_count , \
                    current_pixel->prescale & 0x001F , current_pixel->ut_sign , current_pixel->center_error );
                if( strlen(input_buf) > 1900 ) {
                    retval = safe_send(s, input_buf);
                    memset( input_buf , 0 , RECV_BUF_SIZE+1 );
                }
            }
            if( strlen(input_buf) > 0 ) {
                retval = safe_send(s, input_buf);
            }
            return retval;
        }
    }

    if( command_type == ESDONE ) {
        if( number_tokens != 2 ) {
            memset( input_buf , 0 , RECV_BUF_SIZE+1 );
            safe_sprintf( input_buf , "Syntax: esdone <lane> \r\n");
            return safe_send( s , input_buf );
        }
        if( !strncmp( "all" , tokens[1] , 3 ) ) {
            memset( input_buf , 0 , RECV_BUF_SIZE+1 );
            safe_sprintf( input_buf , "%d\r\n" , global_upload_ready() );
            return safe_send(s, input_buf);
        }
        int curr_lane = strtoul( tokens[1] , pEnd , 0);
        int is_ready = FALSE;
        eye_scan * curr_eyescan = get_eye_scan_lane( curr_lane );
        if( curr_eyescan == NULL ) {
            return safe_send( s , "error, no lane found\r\n" );
        }
        is_ready = curr_eyescan->p_upload_rdy;

        memset( input_buf , 0 , RECV_BUF_SIZE+1 );
        safe_sprintf( input_buf , "%d: %d\r\n" , curr_lane , is_ready );
        return safe_send(s, input_buf);
    }

    if( command_type == ESDISABLE ) {
        if( number_tokens != 2 ) {
            memset( input_buf , 0 , RECV_BUF_SIZE+1 );
            safe_sprintf( input_buf , "Syntax: esdisable <lane> \r\n");
            return safe_send( s , input_buf );
        }
        if( !strncmp( "all" , tokens[1] , 3 ) ) {
            global_stop_eye_scan();
            global_upload_unrdy();
            return 0;
        }

        // Disable the eyescan
        int curr_lane = strtoul( tokens[1] , pEnd , 0);
        eye_scan * curr_eyescan = get_eye_scan_lane( curr_lane );
        curr_eyescan->enable = FALSE;

        // Turn off the GTX for this channel
        xaxi_eyescan_disable_channel(curr_lane);

        return 0;
    }

    typedef enum { N=-1 , W = 0 , H = 1 , B = 2 } wtype_t;
    wtype_t wtype = N;
    char *wtypes[3] = { "w" , "h" , "b" };
    uint wtype_sizes[3] = { 8 , 4 , 2 };
    if( command_type == MWR || command_type == MRD ) {
        for( idx = 0 ; idx < 3 ; idx++ ){
            if( !strncmp( wtypes[idx] , tokens[number_tokens-1] , 1 ) )
                wtype = idx;
        }
    }

    u16_t number_of_words = 1;
    if( command_type == MRD ) {
        if( number_tokens == 2 )
            number_of_words = 0;
        else if( number_tokens > 2 ) {
            if( wtype == N ) {
                number_of_words = strtoul( tokens[number_tokens-1] , pEnd , 0 );
            }
            else {
                number_of_words = strtoul( tokens[number_tokens-2] , pEnd , 0 );
            }
        }
    }
    else if( command_type == MWR ) {
        if( number_tokens == 3 )
            number_of_words = 1;
        else if( number_tokens > 3 ) {
            if( wtype == N ) {
                number_of_words = strtoul( tokens[number_tokens-1] , pEnd , 0 );
            }
            else {
                number_of_words = strtoul( tokens[number_tokens-2] , pEnd , 0 );
            }
        }
    }

    if( command_type == MRD && number_tokens == 2 )
        number_of_words = 1;

    u32_t address = 0;
    u32_t addresses[NTELNETTOKENS] = {0};
    u32_t values[NTELNETTOKENS] = {0};

    if( command_type == MWR || command_type == MRD ) {
        if( number_tokens > 1 ) {
            address = strtoul( tokens[1] , pEnd , 0 );
        }
    }

    if( command_type == MWR ) {
        for( idx = 0 ; idx < number_of_words ; idx++ ) {
            values[idx] = strtoul( tokens[idx+2] , pEnd , 0 );
            if( wtype == N || wtype == W ) { /* WORD, u32_t */
                u32_t * tval = NULL;
                tval = ( (u32_t*)address + idx );
                *tval = (u32_t)values[idx];
            }
            else if( wtype == H ) { /* HALF, u16_t */
                u16_t * tval = NULL;
                tval = ( (u16_t*)address + idx );
                *tval = (u16_t)values[idx];
            }
            else if( wtype == B ) { /* BYTE, u8_t */
                u8_t * tval = NULL;
                tval = ( (u8_t*)address + idx );
                *tval = (u8_t)values[idx];
            }
        }
    }

    if( command_type == MRD ) {
        for( idx = 0 ; idx < number_of_words ; idx++ ) {
            if( wtype == N || wtype == W ) { /* WORD, u32_t */
                u32_t * tval = NULL;
                tval = ( (u32_t*)address + idx );
                addresses[idx] = (u32_t)tval;
                values[idx] = *tval;
            }
            else if( wtype == H ) { /* HALF, u16_t */
                u16_t * tval = NULL;
                tval = ( (u16_t*)address + idx );
                addresses[idx] = (u32_t)tval;
                values[idx] = *tval;
            }
            else if( wtype == B ) { /* BYTE, u8_t */
                u8_t * tval = NULL;
                tval = ( (u8_t*)address + idx );
                addresses[idx] = (u32_t)tval;
                values[idx] = *tval;
            }
        }
        return retval;
    }

    if( command_type == MRD ) {
        char format_string[20];
        memset( format_string , 0 , 20 );
        if( wtype >= 0 )
            sprintf( format_string , "%%p: 0x%%0%dlx\r\n" , wtype_sizes[wtype] );
        else
            sprintf( format_string , "%%p: 0x%%0%dlx\r\n" , wtype_sizes[0] );
        for( idx = 0 ; idx < number_of_words ; idx++ ) {
            memset( input_buf , 0 , RECV_BUF_SIZE+1);
            safe_sprintf( input_buf , format_string , addresses[idx] , values[idx] );
            retval = safe_send(s, input_buf);
        }
        return retval;
    }

    if( command_type == DEBUG ) {
        srand( time(NULL) );

        safe_sprintf( input_buf , "echo mrd %p 4 b | nc 192.168.1.99 7\r\n" , u8_test_array );
        retval = safe_send(s, input_buf);


        safe_sprintf( input_buf , "echo mwr %p {0x%02x 0x%02x 0x%02x 0x%02x} 4 b | nc 192.168.1.99 7\r\n" ,
        u8_test_array , (u8_t)rand() , (u8_t)rand() , (u8_t)rand() , (u8_t)rand() );
        retval = safe_send(s, input_buf);


        safe_sprintf( input_buf , "echo mrd %p 4 b | nc 192.168.1.99 7\r\n" , u8_test_array );
        retval = safe_send(s, input_buf);


        safe_sprintf( input_buf , "echo mrd %p 4 h | nc 192.168.1.99 7\r\n" , u16_test_array );
        retval = safe_send(s, input_buf);


        safe_sprintf( input_buf , "echo mwr %p {0x%04x 0x%04x 0x%04x 0x%04x} 4 h | nc 192.168.1.99 7\r\n" ,
        u16_test_array , (u16_t)rand() , (u16_t)rand() , (u16_t)rand() , (u16_t)rand() );
        retval = safe_send(s, input_buf);


        safe_sprintf( input_buf , "echo mrd %p 4 h | nc 192.168.1.99 7\r\n" , u16_test_array );
        retval = safe_send(s, input_buf);

        safe_sprintf( input_buf , "echo mrd %p 4 w | nc 192.168.1.99 7\r\n" , u32_test_array );
        retval = safe_send(s, input_buf);

        safe_sprintf( input_buf , "echo mwr %p {0x%08lx 0x%08lx 0x%08lx 0x%08lx} 4 w | nc 192.168.1.99 7\r\n" ,
        u32_test_array , (u32_t)rand() , (u32_t)rand() , (u32_t)rand() , (u32_t)rand() );
        retval = safe_send(s, input_buf);

        safe_sprintf( input_buf , "echo mrd %p 4 w | nc 192.168.1.99 7\r\n" , u32_test_array );
        retval = safe_send(s, input_buf);

        return retval;
    }

    if( command_type == DBGEYESCAN ) {
        int curr_lane = -1;

        memset( input_buf , 0 , RECV_BUF_SIZE+1 );

        if( number_tokens == 2 ) {
            curr_lane = strtoul( tokens[1] , pEnd , 0 );
        }

        eyescan_debugging( curr_lane , input_buf );
        if( curr_lane == -1 ) {
            retval = safe_send( s , input_buf );
            return retval;
        }

        u32 drp_addresses[146] = { \
              0x000, 0x00D, 0x00E, 0x00F, 0x011, 0x012, 0x013, 0x014, 0x015, 0x016, 0x018, 0x019, 0x01A, 0x01B, 0x01C, 0x01D, 0x01E, 0x01F, 0x020, \
              0x021, 0x022, 0x023, 0x024, 0x025, 0x026, 0x027, 0x028, 0x029, 0x02A, 0x02B, 0x02C, 0x02D, 0x02E, 0x02F, 0x030, 0x031, 0x032, 0x033, \
              0x034, 0x035, 0x036, 0x037, 0x038, 0x039, 0x03A, 0x03B, 0x03C, 0x03D, 0x03E, 0x03F, 0x040, 0x041, 0x044, 0x045, 0x046, 0x047, 0x048, \
              0x049, 0x04A, 0x04B, 0x04C, 0x04D, 0x04E, 0x04F, 0x050, 0x051, 0x052, 0x053, 0x054, 0x055, 0x056, 0x057, 0x059, 0x05B, 0x05C, 0x05D, \
              0x05E, 0x05F, 0x060, 0x061, 0x062, 0x063, 0x064, 0x065, 0x066, 0x068, 0x069, 0x06A, 0x06B, 0x06F, 0x070, 0x071, 0x074, 0x075, 0x076, \
              0x077, 0x078, 0x079, 0x07A, 0x07C, 0x07D, 0x07F, 0x082, 0x083, 0x086, 0x087, 0x088, 0x08C, 0x091, 0x092, 0x097, 0x098, 0x099, 0x09A, \
              0x09B, 0x09C, 0x09D, 0x09F, 0x0A0, 0x0A1, 0x0A2, 0x0A3, 0x0A4, 0x0A5, 0x0A6, 0x0A7, 0x0A8, 0x0A9, 0x0AA, 0x0AB, 0x0AC ,\
              0x14F, 0x150, 0x151, 0x152, 0x153, 0x154, 0x155, 0x156, 0x157, 0x158, 0x159, 0x15A, 0x15B , 0x15C, 0x15D \
        };

        for( idx = 0 ; idx < 146 ; idx++ ) {
            eyescan_debug_addr( curr_lane , drp_addresses[idx] , input_buf );
            if( strlen(input_buf) > 1900 ) {
                retval = safe_send( s , input_buf );
                memset( input_buf , 0 , RECV_BUF_SIZE+1 );
            }
        }
        if( strlen(input_buf) > 0 ) {
            retval = safe_send( s , input_buf );
            memset( input_buf , 0 , RECV_BUF_SIZE+1 );
        }

        return retval;
    }

    if( command_type == INITCLK ) {
#ifdef IS_OTC_BOARD
        SetClockDevID(0);
        retval = 0;
        if( number_tokens == 1 ) {
            retval = InitClockRegisters();
            return retval;
        }
        else if( number_tokens == 2 ) {
            char * freqs[4] = { "125.000 MHz" , "148.778 MHz" , "200.395 MHz" , "299.000 MHz" };
            u16 regvals[4][21] = {
                { 0x01b9,  0x24c4,  0x74fa,  0x04fa,  0x306f,  0x0023,  0x0003,  0x0023,  0x0003,  0x00c3,  0x0030,  0x0000,  0x00c3,  0x0030,  0x0000,  0x00c3,  0x0030,  0x0000,  0x00c3,  0x0030,  0x0000 } ,
                { 0x01b9,  0x11e9,  0x5c3c,  0x04f0,  0x306f,  0x0023,  0x0004,  0x0023,  0x0004,  0x00c3,  0x0040,  0x0000,  0x00c3,  0x0040,  0x0000,  0x00c3,  0x0040,  0x0000,  0x00c3,  0x0040,  0x0000 } ,
                { 0x01b9,  0x000c,  0x003b,  0x04f5,  0x306f,  0x0023,  0x0002,  0x0023,  0x0002,  0x00c3,  0x0020,  0x0000,  0x00c3,  0x0020,  0x0000,  0x00c3,  0x0020,  0x0000,  0x00c3,  0x0020,  0x0000 } ,
                { 0x01b1,  0x21f5,  0xc846,  0x04f5,  0x306f,  0x0023,  0x0001,  0x0023,  0x0001,  0x00c3,  0x0010,  0x0000,  0x00c3,  0x0010,  0x0000,  0x00c3,  0x0010,  0x0000,  0x00c3,  0x0010,  0x0000 }
            };
            int fidx = strtoul( tokens[1] , pEnd , 0);
            if( fidx >= 0 && fidx < 4 ) {
                u16 regval[21];
                for( idx = 0 ; idx<21 ; idx++ ) {
                    regval[idx] = regvals[fidx][idx];
                }
                memset( input_buf , 0 , RECV_BUF_SIZE+1 );
                safe_sprintf( input_buf , "Using clock frequency of %s\n" , freqs[fidx] );
                retval = safe_send( s , input_buf );
                retval = InitClockRegisters( regval );
                return retval;
            }
            else {
                safe_printf( "Can't init clock\n");
                return 0;
            }
        }
        else if( number_tokens == 22 ) {
            u16 regval[21];
            for( idx = 1 ; idx<22 ; idx++ ) {
                regval[idx-1] = strtoul( tokens[idx] , pEnd , 0);
            }
            retval = InitClockRegisters( regval );
            return retval;
        }
        else {
            safe_printf( "Can't init clock\n");
            return 0;
        }
#endif
        return retval;
    }

    if( command_type == READCLK ) {
#ifdef IS_OTC_BOARD
        u16 *cfgdata = GetClockConfig();

        memset( input_buf , 0 , RECV_BUF_SIZE+1 );
        for( idx = 0 ; idx < 21 ; idx++ ) {
            safe_sprintf( input_buf , "%s  %04d " , input_buf , idx );
        }
        safe_sprintf( input_buf , "%s\r\n" , input_buf );
        retval = safe_send(s, input_buf);
        memset( input_buf , 0 , RECV_BUF_SIZE+1 );
        for( idx = 0 ; idx < 21 ; idx++ ) {
            safe_sprintf( input_buf , "%s0x%04x " , input_buf , cfgdata[idx] );
        }
        safe_sprintf( input_buf , "%s\r\n" , input_buf );
        retval = safe_send(s, input_buf);
        free(cfgdata);
#endif
        return retval;
    }

    if( command_type == PRINTUPOD ) {
#ifdef IS_OTC_BOARD
        u8_t i2c_addresses[8];
        for( idx=0 ; idx<8; idx++ )
            i2c_addresses[idx] = idx;
        for( idx=1 ; idx < number_tokens ; idx++ )
            i2c_addresses[idx-1] = strtoul( tokens[idx] , pEnd , 0 );
        for( idx=0;idx<8;idx++ ) {
            if( number_tokens > 1 && idx>=(number_tokens-1) )
                continue;
            u8_t i2c_addr = i2c_addresses[idx];
            if( i2c_addr < 8 ) {
                i2c_addr = upod_address(i2c_addr);
            }
            SetUPodI2CAddress( i2c_addr );
            uPodMonitorData *mondata = GetUPodStatus();

            memset( input_buf , 0 , RECV_BUF_SIZE+1 );
            char * temp_format_string = "Addr %p , status 0x%02x , temp %d.%03dC , 3.3V %duV , 2.5V %duV\n";
            safe_sprintf( input_buf , temp_format_string , i2c_addr , mondata->status, \
                    mondata->tempWhole, mondata->tempFrac, \
                    100*mondata->v33, 100*mondata->v25);
            free(mondata);
            retval = safe_send(s, input_buf);
        }
#endif
        return retval;
    }

    if( command_type == IICR ) {
#ifdef IS_OTC_BOARD
        int devid = 0;
        u8_t i2c_addr;
        u8_t regaddr;
        u8_t * data;
        u16_t nbytes = 1;

        devid = strtoul( tokens[1] , pEnd , 0 );
        i2c_addr = strtoul( tokens[2] , pEnd , 0 );
        regaddr = strtoul( tokens[3] , pEnd , 0 );

        if( number_tokens == 5 )
            nbytes = strtoul( tokens[4] , pEnd , 0 );

        data = malloc( sizeof(u8_t) * nbytes );

        /* Set the register address */
        retval = IICMasterWrite(devid,i2c_addr,1,&regaddr);
        if( retval != XST_SUCCESS ) return retval;

        /* and do the read */
        retval = IICMasterRead(devid,i2c_addr,nbytes,data);

        memset( input_buf , 0 , RECV_BUF_SIZE+1 );
        for( idx = 0 ; idx < nbytes ; idx++ ) {
            safe_sprintf( input_buf , "%s 0x%02x" , input_buf , data[idx] );
        }
        safe_sprintf( input_buf , "%s\r\n" , input_buf );
        retval = safe_send(s, input_buf);
        free(data);
#endif
        return retval;
    }

    if( command_type == IICW ) {
#ifdef IS_OTC_BOARD
        int devid = 0;
        u8_t i2c_addr;
        u8_t regaddr;
        u8_t * buffer;
        u16_t nbytes = ( number_tokens - 4 );

        devid = strtoul( tokens[1] , pEnd , 0 );
        i2c_addr = strtoul( tokens[2] , pEnd , 0 );
        regaddr = strtoul( tokens[3] , pEnd , 0 );

        buffer = malloc( sizeof(u8_t) * ( nbytes + 1 ) );

        buffer[0] = regaddr;
        for( idx = 4 ; idx < number_tokens ; idx++ ) {
            buffer[idx-3] = strtoul( tokens[idx] , pEnd , 0 );
        }

        retval = IICMasterWrite(devid,i2c_addr,nbytes+1,buffer);
        free(buffer);
#endif
        return retval;
    }

    if( command_type == PRINTTEMP ) { // We now update this in monitor, don't bother doing it twice...
        /* now write the web page data in two steps.  FIrst the Xilinx temp/voltages */
        char *pagefmt = "uptime %d , temp %0.1fC , intv %0.1fV , auxv %0.1fV , bramv %0.1fV\r\n";
        safe_sprintf(input_buf,pagefmt,procStatus.uptime,procStatus.v7temp,procStatus.v7vCCINT,procStatus.v7vCCAUX,procStatus.v7vBRAM);
        int n=strlen(input_buf);
        int w;
        if ((w = safe_send(s, input_buf)) < 0 ) {
            safe_printf("error writing web page data (part 1) to socket\r\n");
            safe_printf("attempted to lwip_write %d bytes, actual bytes written = %d\r\n", n, w);
            return -2;
        }
        return w;
    }
    
    if( command_type == GLOBALINIT ) {
        global_reset_eye_scan();
    }

    return retval;

}
