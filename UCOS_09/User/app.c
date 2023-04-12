#include "ARMCM3.h"
#include "os.h"

#define  TASK1_STK_SIZE       128
#define  TASK2_STK_SIZE       128
#define  TASK3_STK_SIZE       128

static   CPU_STK   Task1Stk[TASK1_STK_SIZE];
static   CPU_STK   Task2Stk[TASK2_STK_SIZE];
static   CPU_STK   Task3Stk[TASK3_STK_SIZE];

static   OS_TCB    Task1TCB;
static   OS_TCB    Task2TCB;
static   OS_TCB    Task3TCB;

uint32_t flag1;
uint32_t flag2;
uint32_t flag3;

void Task1 (void *p_arg);
void Task2 (void *p_arg);
void Task3 (void *p_arg);

int main (void)
{
	OS_ERR err;
	
	/* ��ʼ����ص�ȫ�ֱ����������������� */
	OSInit(&err);
	
	/* CPU ��ʼ������ʼ��ʱ��� */
	CPU_Init();
	
	/* ���жϣ���Ϊ��ʱ OS δ�������������жϣ���ô SysTick ���������ж� */
	CPU_IntDis();
	
	/* ��ʼ�� SysTick������ SysTick Ϊ 10ms �ж�һ�Σ�Tick = 10ms */
	OS_CPU_SysTickInit(10);
	
	/* �������� */
	OSTaskCreate ((OS_TCB*)      &Task1TCB, 
	              (OS_TASK_PTR)  Task1, 
	              (void *)       0,
				  (OS_PRIO)		 3,
	              (CPU_STK*)     &Task1Stk[0],
	              (CPU_STK_SIZE) TASK1_STK_SIZE,
	              (OS_ERR *)     &err);

	OSTaskCreate ((OS_TCB*)      &Task2TCB, 
	              (OS_TASK_PTR)  Task2, 
	              (void *)       0,
				  (OS_PRIO)		 2,
	              (CPU_STK*)     &Task2Stk[0],
	              (CPU_STK_SIZE) TASK2_STK_SIZE,
	              (OS_ERR *)     &err);
				  
	OSTaskCreate ((OS_TCB*)      &Task3TCB, 
	              (OS_TASK_PTR)  Task3, 
	              (void *)       0,
				  (OS_PRIO)		 1,
	              (CPU_STK*)     &Task3Stk[0],
	              (CPU_STK_SIZE) TASK3_STK_SIZE,
	              (OS_ERR *)     &err);
	
	/* ����OS�������ٷ��� */				
	OSStart(&err);
}

void Task1 (void *p_arg)
{
	for (;;)
	{
		flag1 = 1;
		OSTimeDly (2);	
		flag1 = 0;
		OSTimeDly (2);
	}
}

void Task2 (void *p_arg)
{
	for (;;)
	{
		flag2 = 1;
		OSTimeDly (2);		
		flag2 = 0;
		OSTimeDly (2);
	}
}

void Task3 (void *p_arg)
{
	for (;;)
	{
		flag3 = 1;
		OSTimeDly (2);		
		flag3 = 0;
		OSTimeDly (2);
	}
}
