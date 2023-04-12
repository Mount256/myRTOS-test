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

void Task1 (void *p_arg);
void Task2 (void *p_arg);
void delay(uint32_t count);

int main (void)
{
	OS_ERR err;
	
	/* ��ʼ����ص�ȫ�ֱ��� */
	OSInit(&err);
	
	/* �������� */
	OSTaskCreate ((OS_TCB*)      &Task1TCB, 
	              (OS_TASK_PTR ) Task1, 
	              (void *)       0,
	              (CPU_STK*)     &Task1Stk[0],
	              (CPU_STK_SIZE) TASK1_STK_SIZE,
	              (OS_ERR *)     &err);

	OSTaskCreate ((OS_TCB*)      &Task2TCB, 
	              (OS_TASK_PTR ) Task2, 
	              (void *)       0,
	              (CPU_STK*)     &Task2Stk[0],
	              (CPU_STK_SIZE) TASK2_STK_SIZE,
	              (OS_ERR *)     &err);
				  
	/* ��������뵽�����б� */
	OSRdyList[0].HeadPtr = &Task1TCB;
	OSRdyList[1].HeadPtr = &Task2TCB;
	
	/* ����OS�������ٷ��� */				
	OSStart(&err);
}

void delay (uint32_t count)
{
	for(; count!=0; count--);
}


void Task1 (void *p_arg)
{
	for( ;; )
	{
		flag1 = 1;
		delay( 100 );		
		flag1 = 0;
		delay( 100 );
		
		/* �����л����������ֶ��л� */		
		OSSched();
	}
}

void Task2 (void *p_arg)
{
	for( ;; )
	{
		flag2 = 1;
		delay( 100 );		
		flag2 = 0;
		delay( 100 );
		
		/* �����л����������ֶ��л� */
		OSSched();
	}
}