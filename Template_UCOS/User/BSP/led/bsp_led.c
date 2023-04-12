#include "bsp_led.h"   

void LED_Init (void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd (RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOE, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin 	= GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed 	= GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_Out_PP; 
	
	/* Definition: void GPIO_Init (GPIO_TypeDef* GPIOx, GPIO_InitTypeDef* GPIO_InitStruct) */
	GPIO_Init (GPIOB, &GPIO_InitStructure);
	GPIO_Init (GPIOE, &GPIO_InitStructure);
	
	LED0_OFF;
	LED1_OFF;
}


