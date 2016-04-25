
#include "arch/lpc/arch.h"

#include "config.h"
#include "bios/bios.h"
#include "bios/button.h"
#include "bios/arch.h"
#include "bios/evq.h"
#include "bios/cmd.h"
#include "bios/printd.h"

#include "arch/lpc/inc/chip.h"

uint32_t SystemCoreClock = 8000000;

/*lint -esym(9003,_srom,_erom,_sdata,_edata,_sbss,_ebss,_estack) */

extern uint8_t _stext;
extern uint8_t _etext;
extern uint8_t _srom;
extern uint8_t _erom;
extern uint8_t _sdata;
extern uint8_t _edata;
extern uint8_t _sbss;
extern uint8_t _ebss;
extern uint8_t _estack;


typedef int mainret;
mainret main(void);


void _entry_rom(void);
void systick_irq(void);
void uart_irq(void);
void usb_irq(void);
void ssp_irq(void);

static volatile uint32_t jiffies = 0;


void dump_stack(void)
{
	uint32_t j;
	uint32_t *sp = &j;

	printd("sp: %08x\n\n", sp);

	uint32_t *p = sp;
	while(p < (uint32_t *)&_estack) {
		if(*p >= (uint32_t)&_stext && *p < (uint32_t)&_etext) {
			printd("%08x\n", *p);
		}
		p++;
	}

	printd("\n");

	for(j=0; j<32; j++) {
		printd("%08x ", *(sp+j));
		if((j % 8) == 7) printd("\n");
	}
}


static void hardfault_irq(void)
{
	printd("\n\n** HARD FAULT**\n");
	dump_stack();
	for(;;);
}



/*
 * Early entry point. Data and BSS is not yet initialized and the stack pointer
 * can not be trusted. Use registers only!
 */

void _entry_rom(void)
{
	register uint8_t *src, *dst;

	/* Copy .data from flash to RAM */

	src = &_erom;
	dst = &_sdata;
	while(dst < &_edata) *dst++ = *src++;
	
	/* Clear .bss */

	dst = &_sbss;
	while(dst < &_ebss) *dst++ = 0;

	/* Run main */

	main();

	for(;;);
}


void systick_irq(void)
{
	static uint8_t t1 = 0;
	static uint8_t t2 = 0;
	static uint8_t t3 = 0;
        event_t ev;

	jiffies ++;

	t1 ++;
	if(t1 == 10) {

		t1 = 0;
		ev.type = EV_TICK_100HZ;
		evq_push(&ev);

		t2 ++;
		if(t2 == 10) {
			t2 = 0;
			ev.type = EV_TICK_10HZ;
			evq_push(&ev);
		
			t3++;
			if(t3 == 10) {
				t3 = 0;
				ev.type = EV_TICK_1HZ;
				evq_push(&ev);
			}
		}
	}

}


/*
 * This is the ARM vactor table with pointers to the stack pointer, entry point
 * and exception/interrupt handlers
 */


static void *vectors[83] 
__attribute__((section(".vectors"))) 
__attribute__(( __used__ ))
= {
	[0] = &_sdata,
	[1] = _entry_rom,
	[3] = hardfault_irq,
	[15] = systick_irq, 

	[UART0_IRQn + 16] = uart_irq,
	[USB0_IRQn + 16] = usb_irq,
	[SSP0_IRQn + 16] = ssp_irq,

};


void __assert_func(const char *_file, int _line, const char *_func, const char *_expr )
{
	printd("Assert: %s:%d %s(): %s\n", _file, _line, _func, _expr);
	dump_stack();
	for(;;);
}


void assert_failed(uint8_t* file, uint32_t line)
{
	printd("STM cube assert: %s:%d\n", file, line);
	dump_stack();
	for(;;);
}


void arch_init(void)
{
	volatile int i;

	Chip_SYSCTL_PowerUp(SYSCTL_POWERDOWN_SYSOSC_PD);

	for (i = 0; i < 0x200; i++) {}

	Chip_Clock_SetSystemPLLSource(SYSCTL_PLLCLKSRC_SYSOSC);
	Chip_SYSCTL_PowerDown(SYSCTL_POWERDOWN_SYSPLL_PD);
	Chip_Clock_SetupSystemPLL(3, 1);
	Chip_SYSCTL_PowerUp(SYSCTL_POWERDOWN_SYSPLL_PD);
	while (!Chip_Clock_IsSystemPLLLocked()) {}
	Chip_Clock_SetSysClockDiv(1);
	Chip_FMC_SetFLASHAccess(FLASHTIM_72MHZ_CPU);
	Chip_Clock_SetMainClockSource(SYSCTL_MAINCLKSRC_PLLOUT);

	SystemCoreClockUpdate();
	SysTick_Config(SystemCoreClock / 1000);
}


uint32_t HAL_GetTick(void)
{
	return jiffies;
}


void arch_idle(void)
{
	__WFI();
}


void arch_reboot(void)
{
}


uint32_t arch_get_ticks(void)
{
	return jiffies;
}


uint32_t arch_get_unique_id(void)
{
	return 0;
}


void arch_irq_disable(void)
{
	__asm__ volatile( "cpsid i" ::: "memory" );
}


void arch_irq_enable(void)
{
	__asm__ volatile( "cpsie i" ::: "memory" );
}


static rv on_cmd_lpc(uint8_t argc, char **argv)
{
	rv r = RV_EINVAL;

	if(argc >= 1u) {
		char cmd = argv[0][0];

		if(cmd == 'f') {
			printd("clk:   %.1f\n", SystemCoreClock / 1.0e6);
			r = RV_OK;
		}

		if(cmd == 's') {
			dump_stack();
			r = RV_OK;
		}
		
		if(cmd == 'h') {
			uint8_t *a = (void *)0x60000000;
			*a = 0xff;
			r = RV_OK;
		}
	}

	return r;
}

CMD_REGISTER(lpc, on_cmd_lpc, "f[req]");

/*
 * End
 */
