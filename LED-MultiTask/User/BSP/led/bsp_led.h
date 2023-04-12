#ifndef __LED_H
#define __LED_H

#include "stm32f10x.h"

#define LED0_OFF 	GPIO_SetBits  	(GPIOB, GPIO_Pin_5)
#define LED0_ON 	GPIO_ResetBits	(GPIOB, GPIO_Pin_5)
#define LED1_OFF 	GPIO_SetBits	(GPIOE, GPIO_Pin_5)
#define LED1_ON 	GPIO_ResetBits	(GPIOE, GPIO_Pin_5)

#define LED0_REVERSE	GPIO_ReadOutputDataBit (GPIOB, GPIO_Pin_5) ? \
                        GPIO_ResetBits (GPIOB, GPIO_Pin_5) : GPIO_SetBits (GPIOB, GPIO_Pin_5)
#define LED1_REVERSE	GPIO_ReadOutputDataBit (GPIOE, GPIO_Pin_5) ? \
                        GPIO_ResetBits (GPIOE, GPIO_Pin_5) : GPIO_SetBits (GPIOE, GPIO_Pin_5)

void LED_Init (void);

#endif /* __LED_H */
