#ifndef __ARCH_ARM_MACH_OMAP2_AM335X_GENERIC_H
#define __ARCH_ARM_MACH_OMAP2_AM335X_GENERIC_H

extern void __iomem *am33xx_emif_base;
extern void __iomem * __init am33xx_get_mem_ctlr(void);
extern void __iomem *am33xx_get_ram_base(void);

extern void __iomem *am33xx_gpio0_base;
extern void __iomem *am33xx_get_gpio0_base(void);

void am335x_internal_rtc_init(void);

#endif
