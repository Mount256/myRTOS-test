#include "os.h"
#include "ARMCM3.h"

/* ����ջ��ʼ������ */
CPU_STK *OSTaskStkInit ( OS_TASK_PTR 	p_task,  		/* ��������ָʾ���������ڵ�ַ */
						 void			*p_arg,  		/* ������β� */
						 CPU_STK		*p_stk_base, 	/* ����ջ����ʼ��ַ */
						 CPU_STK_SIZE	stk_size ) 		/* ����ջ�Ĵ�С */
{
	CPU_STK		*p_stk;
	
	p_stk = &p_stk_base[stk_size];		/* ��ȡ����ջ��ջ����ַ */
	
	/* �����һ������ʱ��CPU�Ĵ�����ҪԤ������ */
	/* �������쳣����ʱ�Զ������ 8 ���Ĵ��� */
	/* R14��R12��R3��R2 �� R1 Ϊ�˵��Է��㣬��������Ĵ��������Ӧ�� 16 ������ */
	*--p_stk = (CPU_STK) 0x01000000u;		/* xPSR �� bit24 ������ 1 		*/
	*--p_stk = (CPU_STK) p_task;			/* R15(PC) �������ڵ�ַ 		*/
	*--p_stk = (CPU_STK) 0x14141414u;		/* R14(LR)						*/
	*--p_stk = (CPU_STK) 0x12121212u;		/* R12							*/
	*--p_stk = (CPU_STK) 0x03030303u;		/* R3							*/
	*--p_stk = (CPU_STK) 0x02020202u;		/* R2							*/
	*--p_stk = (CPU_STK) 0x01010101u;		/* R1							*/
	*--p_stk = (CPU_STK) p_arg;				/* R0 : �����β�  				*/
	/* ʣ�µ��� 8 ����Ҫ�ֶ����ص� CPU �Ĵ����Ĳ�����Ϊ�˵��Է���������Ĵ��������Ӧ�� 16 ������ */
	*--p_stk = (CPU_STK) 0x11111111u;		/* R11							*/
	*--p_stk = (CPU_STK) 0x10101010u;		/* R10							*/
	*--p_stk = (CPU_STK) 0x09090909u;		/* R9							*/
	*--p_stk = (CPU_STK) 0x08080808u;		/* R8							*/
	*--p_stk = (CPU_STK) 0x07070707u;		/* R7							*/
	*--p_stk = (CPU_STK) 0x06060606u;		/* R6							*/
	*--p_stk = (CPU_STK) 0x05050505u;		/* R5							*/
	*--p_stk = (CPU_STK) 0x04040404u;		/* R4							*/
	
	return p_stk;	/* ��ʱ p_stk ָ��ʣ��ջ��ջ�� */
}

/* ��ʼ��SysTick */
void OS_CPU_SysTickInit (CPU_INT32U ms)
{ 
	SysTick->LOAD  = ms * SystemCoreClock / 1000 - 1;    			/* set reload register */
	
	/* set Priority for Cortex-M0 System Interrupts */
	NVIC_SetPriority (SysTick_IRQn, (1<<__NVIC_PRIO_BITS) - 1); 	/* ���ȼ�Ϊ 15( = (1<<3) - 1 ) */
	
	SysTick->VAL   = 0;                                          	/* Load the SysTick Counter Value */
	
	/* Enable SysTick IRQ and SysTick Timer */
	SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk | 					/* ѡ��ʱ��ԴΪ SystemCoreClock */
					SysTick_CTRL_TICKINT_Msk   | 					/* �����ж� */
					SysTick_CTRL_ENABLE_Msk;                 		/* ����ʹ�� */   	
}

void SysTick_Handler (void)
{
	OSTimeTick();
}

