/*
 * safe_printf.h
 *
 *  Created on: Apr 28, 2014
 *      Author: ddboline
 */

#ifndef SAFE_PRINTF_H_
#define SAFE_PRINTF_H_

#define REDEFINE_XIL_PRINTF_IN_SAFE_PRINTF 1

#include "xil_types.h"
#include <semaphore.h>

#if REDEFINE_XIL_PRINTF_IN_SAFE_PRINTF
#define safe_printf(fmt,args...) { xil_printf(fmt, ##args); }
#else
#define safe_printf(fmt,args...) { lock_printf(); xil_printf(fmt, ##args); unlock_printf(); }
#endif

#define safe_sprintf(ostr,fmt,args...) { \
    char temp_safe_printf_string[2048]; \
    memset( temp_safe_printf_string , 0 , 2048 ); \
    sprintf( temp_safe_printf_string , fmt , ##args ); \
    sprintf( ostr , "%s" , temp_safe_printf_string ); \
    }

void lock_printf(void);
void unlock_printf(void);

#endif /* SAFE_PRINTF_H_ */
