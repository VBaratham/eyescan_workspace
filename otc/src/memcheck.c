/*
 * $Id: $
 *-
 *- memcheck.c
 *-
 *-   Purpose and Methods: (Destructively) Check a range or memory
 *-
 *-   Inputs  :
 *-   Outputs :
 *-   Controls:
 *-
 *-   Created  21-JAN-2014   John D. Hobbs
 *- $Revision: 1.3 $
 *-
*/

#include "xil_types.h"
#include "memcheck.h"

static int valueCheck( u32 *address, u32 nwords, u32 value) {
  u32 iw=0, status=0;
  u32 *curword = address;

  for( iw=0 ; iw<nwords ; iw++ ) *curword++ = value;
  curword = address;
  for( iw=0 ; iw<nwords ; iw++ ) if( *curword++ != value ) status += 1;
  return status;
}

int memcheck(u32 *address, u32 nwords) {
  u32 ibit=0,iw=0,status=0;
  u32 *curword = address;

  xil_printf("Running memcheck for %d words with starting address = 0x%08x\n",nwords,address);

  /* Write all zeros and check it */
  if( valueCheck(address,nwords,0) != 0 ) status |= 1;

  /* Write all ones and check it */
  if( valueCheck(address,nwords,0) != 0 ) status |= 2;

  /* Write on to each bit one after the other and check them*/
  for( ibit=0 ; ibit<32 ; ibit++ )
    if( valueCheck(address,nwords,ibit) != 0 ) status |= 4;

  /* Write a count and check it */
  for( iw=0 ; iw<nwords ; iw++ ) *curword++ = iw;
  curword = address;
  for( iw=0 ; iw<nwords ; iw++ ) if( *curword++ != iw ) status |= 8;

  return status;
}

