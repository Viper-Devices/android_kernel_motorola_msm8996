/*
 *    Bitmaps for set_bit, clear_bit, test_and_set_bit, ...
 *    See include/asm/{bitops.h|posix_types.h} for details
 *
 *    Copyright IBM Corp. 1999,2009
 *    Author(s): Martin Schwidefsky <schwidefsky@de.ibm.com>,
 */

#include <linux/bitops.h>

const char _oi_bitmap[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };

const char _ni_bitmap[] = { 0xfe, 0xfd, 0xfb, 0xf7, 0xef, 0xdf, 0xbf, 0x7f };

const char _zb_findmap[] = {
	0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,
	0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,
	0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,
	0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,6,
	0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,
	0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,
	0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,
	0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,7,
	0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,
	0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,
	0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,
	0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,6,
	0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,
	0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,
	0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,
	0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,8 };

const char _sb_findmap[] = {
	8,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
	4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
	5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
	4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
	6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
	4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
	5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
	4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
	7,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
	4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
	5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
	4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
	6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
	4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
	5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
	4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0 };
