#ifndef __DELAY
#define __DELAY

#include "stm32f407xx.h"

#define SYS_CORE_FREQUENSY 168000000

static volatile uint32_t TimingDelay;

uint32_t delayInit(void);
void delay_ms(volatile uint32_t nTime);
void delay_us(volatile uint32_t nTime);
void timingDelayDecrement(void);



#endif
