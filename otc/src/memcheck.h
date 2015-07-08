/*
 * $Id: $
 *-
 *-   Purpose and Methods: Make a (destructive) memory check. Used to check DDR3
 *-    on Xilinx VC707 eval board and the SBU AMT OpticalTestCard
 *-
 *-   Inputs  : address - starting address of the range to check
 *-             nwords  - number of 32 bit words to test.
 *-   Outputs :
 *-   Controls:
 *-
 *-   Created  21-JAN-2014   John D. Hobbs
 *- $Revision: 1.3 $
 *-
*/

#ifndef MEMCHECK_H_
#define MEMCHECK_H_

int memcheck(u32 *address, u32 nwords);

#endif /* MEMCHECK_H_ */
