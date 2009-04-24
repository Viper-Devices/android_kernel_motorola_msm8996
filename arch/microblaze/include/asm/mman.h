/*
 * Copyright (C) 2006 Atmark Techno, Inc.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#ifndef _ASM_MICROBLAZE_MMAN_H
#define _ASM_MICROBLAZE_MMAN_H

#include <asm-generic/mman.h>

#define MAP_GROWSDOWN	0x0100 /* stack-like segment */
#define MAP_DENYWRITE	0x0800 /* ETXTBSY */
#define MAP_EXECUTABLE	0x1000 /* mark it as an executable */
#define MAP_LOCKED	0x2000 /* pages are locked */
#define MAP_NORESERVE	0x4000 /* don't check for reservations */
#define MAP_POPULATE	0x8000 /* populate (prefault) pagetables */
#define MAP_NONBLOCK	0x10000 /* do not block on IO */

#define MCL_CURRENT	1 /* lock all current mappings */
#define MCL_FUTURE	2 /* lock all future mappings */

#endif /* _ASM_MICROBLAZE_MMAN_H */
