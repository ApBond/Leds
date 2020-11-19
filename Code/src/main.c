#include "main.h"


#define RANGE_MEASURE_COUNT 5
extern uint16_t range;
uint16_t rangeBuf[RANGE_MEASURE_COUNT];
uint8_t rangePointer=0;
uint16_t scanTimer=0;
uint8_t flag=1;

void TIM4_IRQHandler(void)
{
	if((TIM4->SR &TIM_SR_UIF)!=0)
	{
		if(flag==2) 
		{
			EXTI->IMR&=~EXTI_IMR_IM0;
			flag=1;
		}
		else
		{
			TIM4->SR&=~TIM_SR_UIF;//Сброс флага прерывания
			GPIOA->BSRR=GPIO_BSRR_BR1;//Срез измерительно импульса
			EXTI->IMR|=EXTI_IMR_IM0;//Разрешение EXTI0
		}
	}
}

void EXTI0_IRQHandler(void)
{
	uint8_t i;
	EXTI->PR|=EXTI_PR_PR0;//Сброс флага прерывания
	if((GPIOA->IDR & GPIO_IDR_ID0) != 0)//Если прерывание по фронту
	{
		TIM4->ARR=0xFFFF;//Считаем до максимума
		TIM4->CNT=0;//Считаем с нуля
		TIM4->CR1|=TIM_CR1_CEN;//Запустить счет
		flag=2;
	}
	else//Если срез - счет окончен
	{
		TIM4->CR1&=~TIM_CR1_CEN;//Выключаем таймер
		if(rangePointer==RANGE_MEASURE_COUNT-1)
		{
			range=0;
			for(i=0;i<RANGE_MEASURE_COUNT;i++)
			{
				range+=rangeBuf[i];
			}
			range/=RANGE_MEASURE_COUNT;
			rangePointer=0;
		}
		else
		{
			rangeBuf[rangePointer]=TIM4->CNT/58;
			rangePointer++;
		}
		EXTI->IMR&=~EXTI_IMR_IM0;//Запрет EXTI0
		flag=1;
	}
}

void TIM8_UP_TIM13_IRQHandler(void)
{
	if((TIM13->SR &TIM_SR_UIF)!=0)
	{
		TIM13->SR&=~TIM_SR_UIF;//Сброс флага прерывания
		scanTimer++;
	}
}

void timer4Init(void)
{
	RCC->APB1ENR|=RCC_APB1ENR_TIM4EN;//Тактирование таймера 9
	TIM4->PSC=84;//Предделитель таймера
	TIM4->ARR=0xFFFF;
	TIM4->CR1|=TIM_CR1_CEN;
	TIM4->CR1|=TIM_CR1_OPM;//Режим одиночного счета
	TIM4->DIER|=TIM_DIER_UIE;//Включить прерывания по обновлению
	NVIC_EnableIRQ(TIM4_IRQn);
}

void scan(void)
{
	uint8_t i=0;
	if(flag!=1) return;
	TIM4->DIER|=TIM_DIER_UIE;
	TIM4->CNT=0;
	TIM4->ARR=10;
	GPIOA->BSRR=GPIO_BSRR_BS1;//Фронт измерительного импульса
	TIM4->CR1|=TIM_CR1_CEN;//Запустить счет
	/*for(i=0;i<60;i++)
	{
		if(flag==1) break;
		delay_ms(1);
	}*/
}

void triggerInit(void)
{
	SYSCFG->EXTICR[0]|=SYSCFG_EXTICR1_EXTI0_PA;//Выбор прерывания EXTI0 на PA0
	EXTI->FTSR|=EXTI_FTSR_TR0;//Разрешить прерывание по срезу
	EXTI->RTSR|=EXTI_RTSR_TR0;//Разрешить прерывание по фронту
	EXTI->IMR|=EXTI_IMR_IM0;//Разрешение EXTI0
	NVIC_EnableIRQ(EXTI0_IRQn);
}

int main()
{
 	RccClockInit();
	delayInit();
	ledInit();
	animationInit();
	triggerInit();
	timer4Init();
	GPIOA->MODER|=GPIO_MODER_MODE1_0;//PA1 в режив выходы
	GPIOA->OSPEEDR|=GPIO_OSPEEDER_OSPEEDR1;//Максимальная скоростьь на PA1
	GPIOA->PUPDR|=GPIO_PUPDR_PUPD0_1;//Pull down
	RCC->APB1ENR|=RCC_APB1ENR_TIM13EN;
	TIM13->PSC|=8400;//Предделитель на 10кГц
	TIM13->ARR=9;//Делитель на 1кГц
	TIM13->DIER|=TIM_DIER_UIE;//Разрешить прерывание по переполнению
	NVIC_EnableIRQ(TIM8_UP_TIM13_IRQn);
	TIM13->CR1|=TIM_CR1_CEN;//Запустить таймер
	while(1)
	{
		if(scanTimer>5)
		{
			scan();
			scanTimer=0;
		}
		animationLoop();
	}
}

