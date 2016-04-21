
#include "arch/stm32/arch.h"

#include "config.h"
#include "bios/arch.h"
#include "bios/evq.h"
#include "bios/cmd.h"
#include "bios/printd.h"

#include "stm32f1xx_hal.h"

uint32_t SystemCoreClock = 8000000;

/*lint -esym(9003,_srom,_erom,_sdata,_edata,_sbss,_ebss,_estack) */

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
void usart1_irq(void);
void adc1_2_irq(void);
void can1_irq(void);
void can2_irq(void);

static volatile uint32_t jiffies = 0;

rv hal_status_to_rv(HAL_StatusTypeDef status)
{
	rv r;

	switch(status) {
		case HAL_OK:
			r = RV_OK;
			break;
		case HAL_ERROR:
			r = RV_EIO;
			break;
		case HAL_BUSY:
			r = RV_EBUSY;
			break;
		case HAL_TIMEOUT:
			r = RV_ETIMEOUT;
			break;
		default:
			r = RV_EIMPL;
			break;
	}

	return r;
}


void dump_stack(void)
{
	uint32_t j;
	uint32_t *sp = &j;

	printd("sp: %08x\n\n", sp);

	uint32_t *p = sp;
	while(p < (uint32_t *)&_estack) {
		if(*p >= (uint32_t)&_srom && *p < (uint32_t)&_erom) {
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
#ifdef DRV_ADC_STM32
	[34] = adc1_2_irq,
#endif
#ifdef DRV_CAN_STM32
	[CAN1_RX0_IRQn + 16] = can1_irq,
	[CAN1_RX1_IRQn + 16] = can1_irq,
	[CAN1_TX_IRQn + 16] = can1_irq,
	[CAN2_RX0_IRQn + 16] = can2_irq,
	[CAN2_RX1_IRQn + 16] = can2_irq,
	[CAN2_TX_IRQn + 16] = can2_irq,
#endif
	[53] = usart1_irq,
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


static void setup_clock(void)
{
	RCC_ClkInitTypeDef clkinitstruct = {0};
	RCC_OscInitTypeDef oscinitstruct = {0};

	/* Enable HSE Oscillator and activate PLL with HSE as source */

	oscinitstruct.OscillatorType  = RCC_OSCILLATORTYPE_HSE;
	oscinitstruct.HSEState        = RCC_HSE_ON;
	oscinitstruct.HSEPredivValue  = RCC_HSE_PREDIV_DIV1;
	oscinitstruct.PLL.PLLState    = RCC_PLL_ON;
	oscinitstruct.PLL.PLLSource   = RCC_PLLSOURCE_HSE;
	oscinitstruct.PLL.PLLMUL      = RCC_PLL_MUL9;

	if(HAL_RCC_OscConfig(&oscinitstruct)!= HAL_OK)
	{
		/* Initialization Error */
		while(1);
	}

	/* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
	   clocks dividers */

	clkinitstruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | 
				   RCC_CLOCKTYPE_PCLK1  | RCC_CLOCKTYPE_PCLK2);
	clkinitstruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	clkinitstruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	clkinitstruct.APB2CLKDivider = RCC_HCLK_DIV1;
	clkinitstruct.APB1CLKDivider = RCC_HCLK_DIV2;

	if(HAL_RCC_ClockConfig(&clkinitstruct, FLASH_LATENCY_2)!= HAL_OK)
	{
		/* Initialization Error */
		while(1);
	}
}


void arch_init(void)
{
	HAL_Init();
	setup_clock();
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
	HAL_NVIC_SystemReset();;
}


uint32_t arch_get_ticks(void)
{
	return jiffies;
}

/*
 * FNV-hash all random-id bits of the ARM processor to generate an unique
 * device serial number. The MSB is always set to indicate this is a random
 * serial number, not a official production serial number
 */

#define FNV_BASIS 2166136261
#define FNV_PRIME 16777619

uint32_t arch_get_unique_id(void)
{
    uint32_t id = FNV_BASIS;
    int i;

    uint8_t *p = (void *)0x1FFFF7E8;
    for(i=0; i<12; i++) {
        id *= FNV_PRIME;
        id ^= *p++;
    }

    return id;
}


void arch_irq_disable(void)
{
	__asm__ volatile( "cpsid i" ::: "memory" );
}


void arch_irq_enable(void)
{
	__asm__ volatile( "cpsie i" ::: "memory" );
}


static rv on_cmd_stm32(uint8_t argc, char **argv)
{
	rv r = RV_EINVAL;

	if(argc >= 1u) {
		char cmd = argv[0][0];

		if(cmd == 'f') {
			printd("sys:   %.1f\n", HAL_RCC_GetSysClockFreq() / 1.0e6);
			printd("hclk:  %.1f\n", HAL_RCC_GetHCLKFreq() / 1.0e6);
			printd("pclk1: %.1f\n", HAL_RCC_GetPCLK1Freq() / 1.0e6);
			printd("pclk2: %.1f\n", HAL_RCC_GetPCLK1Freq() / 1.0e6);
			r = RV_OK;
		}
	}

	return r;
}

CMD_REGISTER(stm32, on_cmd_stm32, "f[req]");

/*
 * End
 */
