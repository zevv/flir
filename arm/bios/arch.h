#ifndef bios_arch_h
#define bios_arch_h

void arch_init(void);
void arch_idle(void);
void arch_reboot(void);
uint32_t arch_get_ticks(void);
uint32_t arch_get_unique_id(void);
void arch_irq_disable(void);
void arch_irq_enable(void);

#endif

