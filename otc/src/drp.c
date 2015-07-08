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
#include "drp.h"
#include "xaxi_eyescan.h"

u8 two_digit_strtoi(u8* str){
    return (str[0] - '0') * 10 + (str[1] = '0');
}

u16 mask_drp_rddata (u16 value, u8 start_bit, u8 end_bit) {
    u8 left_shift = 15 - end_bit;
    u16 new_val = ( ( value << left_shift ) & 0xFFFF ) >> (start_bit + left_shift);

    return new_val;
}

u16 get_mask( u8 start_bit , u8 end_bit ) {
    u16 value = 0xFFFF;
    u8 left_shift = 15 - end_bit;
    u16 new_val = ( ( value << left_shift ) & 0xFFFF ) >> (start_bit + left_shift);
    new_val = ( new_val << start_bit ) & 0xFFFF;
    new_val = ~new_val;
    return new_val;
}


u16 drp_write (u16 value, u8 attr_name, u8 lane_num) {
    u16 curr_value = xaxi_eyescan_read_channel_drp( lane_num , drp_addr[attr_name] );
    //printf( "drp_write value before 0x%x 0x%x\n" , value , curr_value );
    value = ( ( curr_value & drp_mask[attr_name] ) | ( ( value << drp_start_bit[attr_name] ) & 0xFFFF ) );
    //printf( "drp_write value after 0x%x\n" , value );
    xaxi_eyescan_write_channel_drp(lane_num, drp_addr[attr_name], value);
    u16 new_value = (u16) xaxi_eyescan_read_channel_drp(lane_num, drp_addr[attr_name]);
    if( new_value != value ) {
        printf( "drp_write to 0x%x expected 0x%x observed 0x%x lane %d\n" , drp_addr[attr_name] , value , new_value , lane_num );
    }
    return (u16) xaxi_eyescan_read_channel_drp(lane_num, drp_addr[attr_name]);
}

u16 drp_read  (u8 attr_name, u8 lane_num){
    u16 val = (u16) xaxi_eyescan_read_channel_drp(lane_num, drp_addr[attr_name]);
    if( attr_name == ES_PRESCALE )
        val = mask_drp_rddata( val , 11 , 15 );
    if( attr_name == ALIGN_COMMA_WORD )
        val = mask_drp_rddata( val , 13 , 15 );
    if( attr_name == TX_INT_DATAWIDTH )
        val = mask_drp_rddata( val , 4 , 4 );
    if( attr_name == TX_DATA_WIDTH )
        val = mask_drp_rddata( val , 0 , 2 );
    if( attr_name == RX_INT_DATAWIDTH )
        val = mask_drp_rddata( val , 14 , 14 );
    if( attr_name == RX_DATA_WIDTH )
        val = mask_drp_rddata( val , 11 , 13 );

    return val;
}

u16 drp_write_raw( u16 value , u16 drp_address , u8 start_bit , u8 end_bit , u8 lane_num ) {
    if( start_bit > end_bit ) {
        printf( "You really don't want to do this, start_bit should be less than end_bit!!!!\n" );
        exit(1);
    }
    u16 mask = get_mask( start_bit , end_bit );
    u16 curr_value = xaxi_eyescan_read_channel_drp( lane_num , drp_address );
    //printf( "drp_write value before 0x%x 0x%x\n" , value , curr_value );
    value = ( ( curr_value & mask) | ( ( value << start_bit ) & 0xFFFF ) );
    //printf( "drp_write value after 0x%x\n" , value );
    xaxi_eyescan_write_channel_drp(lane_num, drp_address, value);
    u16 new_value = (u16) xaxi_eyescan_read_channel_drp(lane_num, drp_address);
    if( new_value != value ) {
        printf( "drp_write to 0x%x expected 0x%x observed 0x%x\n" , drp_address , value , new_value );
    }
    return (u16) xaxi_eyescan_read_channel_drp(lane_num, drp_address);
}

u16 drp_read_raw( u16 drp_address , u8 start_bit , u8 end_bit , u8 lane_num ) {
    u16 val = (u16) xaxi_eyescan_read_channel_drp(lane_num, drp_address);
    if( start_bit != 0 || end_bit != 15 )
        val = mask_drp_rddata( val , start_bit , end_bit );
    return val;
}
