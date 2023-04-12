#include "os.h"

/* ���񴴽����� */
void OSTaskCreate( 	OS_TCB 			*p_tcb,  		/* TCBָ�� */
					OS_TASK_PTR 	p_task,  		/* �������� */
					void 			*p_arg,  		/* ������β� */
					CPU_STK 		*p_stk_base, 	/* ����ջ����ʼ��ַ */
					CPU_STK_SIZE 	stk_size,		/* ����ջ��С */
					OS_ERR 			*p_err )		/* ������ */
{
	CPU_STK		*p_sp;
	
	p_sp = OSTaskStkInit ( 	p_task,
							p_arg,
							p_stk_base,
							stk_size );  /* ����ջ��ʼ������ */
	p_tcb->StkPtr 	= p_sp;    	/* ʣ��ջ��ջ��ָ�� p_sp ���浽������ƿ� TCB �ĵ�һ����Ա StkPtr �� */
	p_tcb->StkSize 	= stk_size; /* ������ջ�Ĵ�С���浽������ƿ� TCB �ĳ�Ա StkSize �� */
	
	*p_err = OS_ERR_NONE;		/* ����ִ�е������ʾû�д��� */
}
