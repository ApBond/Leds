#include "animation.h"

uint16_t blinkTimer=0;
uint16_t blinkTimer1=0;
uint16_t updateTimer = 0;
uint16_t range=0;

void TIM8_TRG_COM_TIM14_IRQHandler(void)
{
	if((TIM14->SR &TIM_SR_UIF)!=0)
	{
		TIM14->SR&=~TIM_SR_UIF;//Сброс флага прерывания
		blinkTimer++;
		blinkTimer1++;
		updateTimer++;
	}
}

void animationInit(void)
{
	RCC->APB1ENR|=RCC_APB1ENR_TIM14EN;//Тактирование таймера 14
	TIM14->PSC|=16800;//Предделитель на 10кГц
	TIM14->ARR=9;//Делитель на 1кГц
	TIM14->DIER|=TIM_DIER_UIE;//Разрешить прерывание по переполнению
	NVIC_EnableIRQ(TIM8_TRG_COM_TIM14_IRQn);
	TIM14->CR1|=TIM_CR1_CEN;//Запустить таймер
}

void animationLoop(void)
{
	static uint8_t blinkState=0;
	static uint8_t blinkState1=0;
	static uint8_t redBlink=0;
	if(blinkTimer1>500)
	{
		if(blinkState1==0)
			{
				ledSetBlockHSV(8,5,12,255,255);
				blinkState1=1;
			}
			else
			{
				ledSetBlockHSV(8,5,255,255,0);
				blinkState1=0;
			}
			blinkTimer1=0;
	}
	if(redBlink && blinkTimer>200)
	{
		if(blinkState==0)
		{
			ledSetBlockHSV(0,7,0,255,255);
			blinkState=1;
		}
		else
		{
			ledSetBlockHSV(0,7,255,255,0);
			blinkState=0;
		}
		blinkTimer=0;
	}
	if(updateTimer>100)
	{
		if(range>=5)
		{
			redBlink=0;
			rangeIndication(0,7,range,200,5);
		}
		else
		{
			redBlink=1;
		}			
		updateTimer=0;
		ledUpdate();
	}
}

static void rangeIndication(uint16_t ledStart,uint16_t ledNum, uint16_t range,uint16_t rangeMax,uint16_t rangeMin)
{
	uint16_t hue=(((range-rangeMin)*(80)/(rangeMax-rangeMin)));
	if(range>rangeMax) hue=80;
	if(range<rangeMin) hue=0;
	ledSetBlockHSV(ledStart,ledNum,hue,255,255);
}