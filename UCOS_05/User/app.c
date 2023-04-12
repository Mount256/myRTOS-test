#include "ARMCM3.h"
#include "os.h"

#define  TASK1_STK_SIZE       20
#define  TASK2_STK_SIZE       20

static   CPU_STK   Task1Stk[TASK1_STK_SIZE];
static   CPU_STK   Task2Stk[TASK2_STK_SIZE];

static   OS_TCB    Task1TCB;
static   OS_TCB    Task2TCB;

uint32_t flag1;
uint32_t flag2;

uint32_t TimeStart;
uint32_t TimeEnd;
uint32_t TimeUse;

void Task1 (void *p_arg);
void Task2 (void *p_arg);
void delay(uint32_t count);

int main (void)
{
	OS_ERR err;
	
	/* 初始化相关的全局变量，创建空闲任务 */
	OSInit(&err);
	
	/* CPU 初始化：初始化时间戳 */
	CPU_Init();
	
	/* 关中断，因为此时 OS 未启动，若开启中断，那么 SysTick 将会引发中断 */
	CPU_IntDis();
	
	/* 初始化 SysTick，配置 SysTick 为 10ms 中断一次，Tick = 10ms */
	OS_CPU_SysTickInit(10);
	
	/* 创建任务 */
	OSTaskCreate ((OS_TCB*)      &Task1TCB, 
	              (OS_TASK_PTR) Task1, 
	              (void *)       0,
	              (CPU_STK*)     &Task1Stk[0],
	              (CPU_STK_SIZE) TASK1_STK_SIZE,
	              (OS_ERR *)     &err);

	OSTaskCreate ((OS_TCB*)      &Task2TCB, 
	              (OS_TASK_PTR) Task2, 
	              (void *)       0,
	              (CPU_STK*)     &Task2Stk[0],
	              (CPU_STK_SIZE) TASK2_STK_SIZE,
	              (OS_ERR *)     &err);
				  
	/* 将任务加入到就绪列表 */
	OSRdyList[0].HeadPtr = &Task1TCB;
	OSRdyList[1].HeadPtr = &Task2TCB;
	
	/* 启动OS，将不再返回 */				
	OSStart(&err);
}

void Task1 (void *p_arg)
{
	for (;;)
	{
		flag1 = 1;
		TimeStart = OS_TS_GET();
		OSTimeDly (20);	
		TimeEnd = OS_TS_GET();
		TimeUse = TimeEnd - TimeStart;
		flag1 = 0;
		OSTimeDly (5);
	}
}

void Task2 (void *p_arg)
{
	for (;;)
	{
		flag2 = 1;
		OSTimeDly (5);		
		flag2 = 0;
		OSTimeDly (5);
	}
}
