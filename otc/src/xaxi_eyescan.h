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

#include "xbasic_types.h"
#include "xparameters.h"

/* Constants which separate the four address zones: global regs, common drp, channel regs, channel DRP */
#define XAXI_EYESCAN_GLOBAL_REG     0x00000
#define XAXI_EYESCAN_COMMON_DRP     0x40000
#define XAXI_EYESCAN_CHANNEL_REG    0x80000
#define XAXI_EYESCAN_CHANNEL_DRP    0x80800

#define XAXI_EYESCAN_QUAD_SHIFT     11
#define XAXI_EYESCAN_QUAD_SIZE       4
#define XAXI_EYESCAN_CHANNEL_SHIFT  13
#define XAXI_EYESCAN_CHANNEL_SIZE    6

/* Base register offsets and masks to get fields. */
#define XAXI_EYESCAN_NGTX   0
#define XAXI_EYESCAN_NLEFT  1
#define XAXI_EYESCAN_NRIGHT 2
#define XAXI_EYESCAN_QPLL_LOCK 3
#define XAXI_EYESCAN_QPLL_LOST 4
#define XAXI_EYESCAN_GLOBAL_RESET 5
#define XAXI_EYESCAN_BASEFREQ_COUNT 6
#define XAXI_EYESCAN_FREQ0_COUNT 7
#define XAXI_EYESCAN_FREQ1_COUNT 8
#define XAXI_EYESCAN_FREQ2_COUNT 9
#define XAXI_EYESCAN_FREQ3_COUNT 10
#define XAXI_EYESCAN_QPLL_BASEFREQ_COUNT 11
#define XAXI_EYESCAN_QPLL_FREQ0_COUNT 12
#define XAXI_EYESCAN_QPLL_FREQ1_COUNT 13
#define XAXI_EYESCAN_QPLL_FREQ2_COUNT 14
#define XAXI_EYESCAN_QPLL_FREQ3_COUNT 15

#define XAXI_EYESCAN_GTX_MASK   0x00FF
#define XAXI_EYESCAN_QUAD_MASK  0xFF00

/* Channel register offsets */
#define XAXI_EYESCAN_RESET      0
#define XAXI_EYESCAN_TXCFG      1
#define XAXI_EYESCAN_RXCFG      2
#define XAXI_EYESCAN_CLKCFG     3
#define XAXI_EYESCAN_MONITOR    4
#define XAXI_EYESCAN_CURSOR     5

/* I/O access to specific regions of the eyescan memory space:
 *   Global Registers
 *   Quad-specific DRP  (GTX common DRP)
 *   GTX channel specific registers
 *   GTX channel specific DRP
 */

u32 xaxi_eyescan_read_global(u32 offset);
void xaxi_eyescan_write_global(u32 offset, u32 value);  /* NB: There are no writable global registers */

u32 xaxi_eyescan_read_common_drp(u32 quadIdx, u32 drpOffset);
void xaxi_eyescan_write_common_drp(u32 quadIdx, u32 drpOffset, u32 value);

u32 xaxi_eyescan_read_channel_reg(u32 chanIdx, u32 chanRegOffset);
void xaxi_eyescan_write_channel_reg(u32 chanIdx, u32 chanRegOffset, u32 value);

u32 xaxi_eyescan_read_channel_drp(u32 chanIdx, u32 drpOffset);
void xaxi_eyescan_write_channel_drp(u32 chanIdx, u32 drpOffset, u32 value);

/* Non-IO routies */
int xaxi_eyescan_channel_tx_active(u32 chanIdx);
int xaxi_eyescan_channel_rx_active(u32 chanIdx);
int xaxi_eyescan_channel_active(u32 chanIdx);
void xaxi_eyescan_reset_channel(u32 chanIdx);
void xaxi_eyescan_reset_channel_after_rx_powerdown(u32 chanIdx);
void xaxi_eyescan_enable_channel(u32 chanIdx);
void xaxi_eyescan_disable_channel(u32 chanIdx);
void xaxi_eyescan_error_inject(u32 chanIdx);

/* Low level I/O routines.  Ultimately these always end up being called. */
u32 xaxi_eyescan_read(u32 *address);
void xaxi_eyescan_write(u32 *address, u32 value);
