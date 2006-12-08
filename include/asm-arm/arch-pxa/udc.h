/*
 * linux/include/asm-arm/arch-pxa/udc.h
 *
 * This supports machine-specific differences in how the PXA2xx
 * USB Device Controller (UDC) is wired.
 *
 */
#include <asm/mach/udc_pxa2xx.h>

extern void pxa_set_udc_info(struct pxa2xx_udc_mach_info *info);

