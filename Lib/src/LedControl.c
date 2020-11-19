#include "LedControl.h"

const uint8_t dim_curve[256] = {
  0, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4,
  4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6,
  6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8,
  8, 8, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 11, 11, 11,
  11, 11, 12, 12, 12, 12, 12, 13, 13, 13, 13, 14, 14, 14, 14, 15,
  15, 15, 16, 16, 16, 16, 17, 17, 17, 18, 18, 18, 19, 19, 19, 20,
  20, 20, 21, 21, 22, 22, 22, 23, 23, 24, 24, 25, 25, 25, 26, 26,
  27, 27, 28, 28, 29, 29, 30, 30, 31, 32, 32, 33, 33, 34, 35, 35,
  36, 36, 37, 38, 38, 39, 40, 40, 41, 42, 43, 43, 44, 45, 46, 47,
  48, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62,
  63, 64, 65, 66, 68, 69, 70, 71, 73, 74, 75, 76, 78, 79, 81, 82,
  83, 85, 86, 88, 90, 91, 93, 94, 96, 98, 99, 101, 103, 105, 107, 109,
  110, 112, 114, 116, 118, 121, 123, 125, 127, 129, 132, 134, 136, 139, 141, 144,
  146, 149, 151, 154, 157, 159, 162, 165, 168, 171, 174, 177, 180, 183, 186, 190,
  193, 196, 200, 203, 207, 211, 214, 218, 222, 226, 230, 234, 238, 242, 248, 255,
};

uint16_t ledsBuff[LEDS_COUNT*24+2];
uint8_t idleFlag=0;

void DMA1_Stream4_IRQHandler(void)
{
	DMA1_Stream4->CR&=~(DMA_SxCR_EN);//Выключить ДМА
	DMA1->HIFCR=(DMA_HISR_TCIF4 | DMA_HISR_HTIF4 | DMA_HISR_FEIF4);
	TIM3->CR1 &= ~(TIM_CR1_CEN); //останавливаем таймер
	TIM3->CCR1=0;//Сбрасываем регистр сравнения
	TIM10->DIER |= TIM_DIER_UIE; //прерывание по обновлению
	TIM10->CR1|=TIM_CR1_CEN;//Включить таймер
}

void TIM1_UP_TIM10_IRQHandler(void)
{
	if((TIM10->SR &TIM_SR_UIF)!=0)
	{
		TIM10->SR&=~TIM_SR_UIF;//Сброс флага прерывания
		TIM10->CR1 &= ~(TIM_CR1_CEN); //останавливаем таймер
		TIM10->DIER &= ~(TIM_DIER_UIE); //запрещаем прерывание таймера
		idleFlag=0;
	}
}

void ledInit(void)
{
	RCC->APB1ENR|=RCC_APB1ENR_TIM3EN;//Тактирование таймера 3
	RCC->AHB1ENR|=RCC_AHB1ENR_GPIOAEN;//Тактирование порта A
	RCC->AHB1ENR|=RCC_AHB1ENR_DMA1EN;//Тактирование DMA1
	RCC->APB2ENR|=RCC_APB2ENR_TIM10EN;//Тактирование таймера 10
	//GPIO setting
	GPIOA->MODER|=GPIO_MODER_MODE6_1;//PA6в режим альтернативной функции
	GPIOA->OSPEEDR|=GPIO_OSPEEDER_OSPEEDR6;//Высокая скорость порта PA6
	GPIOA->AFR[0]|=GPIO_AFRL_AFRL6_1;//Альтернативная функция AF2 на PA6
	//Timer setting
	TIM3->ARR=TIMER_ARR;//Регистр автоперезагрузки
	TIM3->PSC=0;//Настройка делителя
	TIM3->CR1&=~TIM_CR1_DIR;//Счет вверх
	TIM3->CR1 &= ~TIM_CR1_CMS;//Режим быстрого ШИМ
	TIM3->CCMR1|=TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2 ;//CH1 - прямой ШИМ.
	TIM3->CCER|=TIM_CCER_CC1E;//Настройка каналa 1 на выход. Активный уровень - высокий.
  TIM3->DIER |= TIM_DIER_CC1DE; //Разрешить запрос DMA
	TIM3->CR2|=TIM_CR2_CCDS;
	//Настройка таймера 10
	TIM10->PSC=168;
	TIM10->ARR=WAIT_TIME_US;
	TIM10->CR1|=TIM_CR1_OPM;
	TIM10->DIER|=TIM_DIER_UIE;
	//DMA settings
	DMA1_Stream4->CR|=5<<DMA_SxCR_CHSEL_Pos//Выбор 5го канала ДМА 
	| DMA_SxCR_PL//Высокий приоретет 
	| DMA_SxCR_MINC//Инкремент памяти
	| DMA_SxCR_PSIZE_0//Размер периферии 16бит
	| DMA_SxCR_TCIE//Разрешить прерывание по завершению передачи
	| DMA_SxCR_DIR_0;//Из памяти в переферию
	DMA1_Stream4->CR&=~(DMA_SxCR_EN);//Выключить ДМА
	DMA1_Stream4->PAR=(uint32_t)(&TIM3->CCR1);//Адрес регистра сравнения TIM2
	DMA1_Stream4->M0AR=(uint32_t)&ledsBuff;//Адрес буфера
	//Разрешаем обработку прерываний
  NVIC_EnableIRQ(DMA1_Stream4_IRQn); 
	NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);
	__enable_irq();
	buffClear();
}

uint8_t ledUpdate(void)
{
	if(!idleFlag)
	{
		DMA1_Stream4->CR&=~(DMA_SxCR_EN);//Выключить ДМА
		DMA1_Stream4->NDTR =sizeof(ledsBuff)/2;//Кол-во данных для передачи
		DMA1_Stream4->CR|=DMA_SxCR_EN;//Включить ДМА
		TIM3->CR1|=TIM_CR1_CEN;//Включить таймер
		idleFlag=1;
		return 1;
	}
	return 0;
}

uint8_t getIdleFlag(void)
{
	return idleFlag;
}

uint8_t ledSetRGB(uint16_t led,uint8_t r,uint8_t g, uint8_t b)
{
	uint8_t i;
	if(led>LEDS_COUNT-1 || idleFlag==1) return 0;
	//Зелёный цвет
  for(i=0; i<8; i++)
  {
    if(g & (1<<i))
      ledsBuff[led*24 + (7-i)] = PWM_HIGH;
    else
      ledsBuff[led*24 + (7-i)] = PWM_LOW;
  }
	//Красный цвет
  for(i=0; i<8; i++)
  {
    if(r & (1<<i))
      ledsBuff[led*24 + (7-i) + 8] = PWM_HIGH;
    else
      ledsBuff[led*24 + (7-i) + 8] = PWM_LOW;
  }
	//Синий цвет
  for(i=0; i<8; i++)
  {
    if(b & (1<<i))
      ledsBuff[led*24 + (7-i) + 16] = PWM_HIGH;
    else
      ledsBuff[led*24 + (7-i) + 16] = PWM_LOW;
  }
	return 1;
}

void buffClear(void)
{
	uint16_t i;
	for(i=0;i<LEDS_COUNT*24;i++)
	{
		ledsBuff[i]=PWM_LOW;
	}
}

uint8_t ledSetHSV(uint16_t led,uint16_t hue, uint16_t sat, uint16_t val) 
{
  //hue        : 0..360
  //saturation : 0..255
  //value      : 0..255
  uint16_t r;
  uint16_t g;
  uint16_t b;
  uint16_t base;
  //uint32_t rgb;
  val = dim_curve[val];
  sat = 255 - dim_curve[255 - sat];
	if(led>LEDS_COUNT-1 || idleFlag==1) return 0;
  if ( sat == 0 ) // Acromatic color (gray). Hue doesn't mind.
  {
    r = val;
    g = val;
    b = val;
  }
  else
  {
    base = ((255 - sat) * val) >> 8;
    switch (hue / 60)
    {
    case 0:
      r = val;
      g = (((val - base) * hue) / 60) + base;
      b = base;
      break;
    case 1:
      r = (((val - base) * (60 - (hue % 60))) / 60) + base;
      g = val;
      b = base;
      break;
    case 2:
      r = base;
      g = val;
      b = (((val - base) * (hue % 60)) / 60) + base;
      break;
    case 3:
      r = base;
      g = (((val - base) * (60 - (hue % 60))) / 60) + base;
      b = val;
      break;
    case 4:
      r = (((val - base) * (hue % 60)) / 60) + base;
      g = base;
      b = val;
      break;
    case 5:
      r = val;
      g = base;
      b = (((val - base) * (60 - (hue % 60))) / 60) + base;
      break;
    }
		return ledSetRGB(led,r & 0xFF,g & 0xFF,b & 0xFF);
  }
}

uint8_t ledSetBlockRGB(uint16_t ledStart,uint16_t ledCount,uint8_t r,uint8_t g, uint8_t b)
{
	uint16_t i;
	if(ledStart+ledCount>LEDS_COUNT-1 || idleFlag==1) return 0;
	for(i=ledStart;i<ledCount+ledStart;i++)
	{
		if(!ledSetRGB(i,r,g,b)) return 0;
	}
	return 1;
}

uint8_t ledSetBlockHSV(uint16_t ledStart,uint16_t ledCount,uint16_t hue, uint16_t sat, uint16_t val)
{
	uint16_t i;
	if(ledStart+ledCount>LEDS_COUNT-1 || idleFlag==1) return 0;
 
	for(i=ledStart;i<ledCount+ledStart;i++)
	{
		if(!ledSetHSV(i,hue,sat,val)) return 0;
	}
	return 1;
}
