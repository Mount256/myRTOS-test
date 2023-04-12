#include "os.h"

/* 任务栈初始化函数 */
CPU_STK *OSTaskStkInit ( OS_TASK_PTR 	p_task,  		/* 任务名，指示着任务的入口地址 */
						 void			*p_arg,  		/* 任务的形参 */
						 CPU_STK		*p_stk_base, 	/* 任务栈的起始地址 */
						 CPU_STK_SIZE	stk_size ) 		/* 任务栈的大小 */
{
	CPU_STK		*p_stk;
	
	p_stk = &p_stk_base[stk_size];		/* 获取任务栈的栈顶地址 */
	
	/* 任务第一次运行时，CPU寄存器需要预设数据 */
	/* 首先是异常发生时自动保存的 8 个寄存器 */
	/* R14、R12、R3、R2 和 R1 为了调试方便，需填入与寄存器号相对应的 16 进制数 */
	*--p_stk = (CPU_STK) 0x01000000u;		/* xPSR 的 bit24 必须置 1 		*/
	*--p_stk = (CPU_STK) p_task;			/* R15(PC) 任务的入口地址 		*/
	*--p_stk = (CPU_STK) 0x14141414u;		/* R14(LR)						*/
	*--p_stk = (CPU_STK) 0x12121212u;		/* R12							*/
	*--p_stk = (CPU_STK) 0x03030303u;		/* R3							*/
	*--p_stk = (CPU_STK) 0x02020202u;		/* R2							*/
	*--p_stk = (CPU_STK) 0x01010101u;		/* R1							*/
	*--p_stk = (CPU_STK) p_arg;				/* R0 : 任务形参  				*/
	/* 剩下的是 8 个需要手动加载到 CPU 寄存器的参数，为了调试方便填入与寄存器号相对应的 16 进制数 */
	*--p_stk = (CPU_STK) 0x11111111u;		/* R11							*/
	*--p_stk = (CPU_STK) 0x10101010u;		/* R10							*/
	*--p_stk = (CPU_STK) 0x09090909u;		/* R9							*/
	*--p_stk = (CPU_STK) 0x08080808u;		/* R8							*/
	*--p_stk = (CPU_STK) 0x07070707u;		/* R7							*/
	*--p_stk = (CPU_STK) 0x06060606u;		/* R6							*/
	*--p_stk = (CPU_STK) 0x05050505u;		/* R5							*/
	*--p_stk = (CPU_STK) 0x04040404u;		/* R4							*/
	
	return p_stk;	/* 此时 p_stk 指向剩余栈的栈顶 */
}
