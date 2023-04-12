#include "os.h"

/* ��ʼ������ TCB */
void OS_TaskInitTCB (OS_TCB *p_tcb)
{
	p_tcb->StkPtr 			= (CPU_STK   *)	0;
	p_tcb->StkSize 			= (CPU_STK_SIZE)0;
	
//	p_tcb->TaskDelayTicks 	= (OS_TICK    )	0;		
	
	p_tcb->Prio 			= (OS_PRIO    )	OS_PRIO_INIT;				
	
	p_tcb->NextPtr 			= (OS_TCB    *)	0;			
	p_tcb->PrevPtr 			= (OS_TCB    *)	0;			
}

/* ���񴴽����� */
void OSTaskCreate( 	OS_TCB 			*p_tcb,  		/* TCBָ�� */
					OS_TASK_PTR 	p_task,  		/* �������� */
					void 			*p_arg,  		/* ������β� */
					OS_PRIO			prio,			/* �������ȼ� */
					CPU_STK 		*p_stk_base, 	/* ����ջ����ʼ��ַ */
					CPU_STK_SIZE 	stk_size,		/* ����ջ��С */
					OS_ERR 			*p_err )		/* ������ */
{
	CPU_STK		*p_sp;
	CPU_SR_ALLOC();
	
	OS_TaskInitTCB (p_tcb);
	
	p_sp = OSTaskStkInit ( 	p_task,
							p_arg,
							p_stk_base,
							stk_size );  /* ����ջ��ʼ������ */
	p_tcb->Prio		= prio;		/* �������ȼ������� TCB �� prio ��*/
	p_tcb->StkPtr 	= p_sp;    	/* ʣ��ջ��ջ��ָ�� p_sp ���浽������ƿ� TCB �� StkPtr �� */
	p_tcb->StkSize 	= stk_size; /* ������ջ�Ĵ�С���浽������ƿ� TCB �ĳ�Ա StkSize �� */
	
	OS_CRITICAL_ENTER();	/* �����ٽ�� */
	
	/* ��������ӵ������б� */
	OS_PrioInsert (p_tcb->Prio);
	OS_RdyListInsertTail (p_tcb);
	
	OS_CRITICAL_EXIT();		/* �˳��ٽ�� */
	
	*p_err = OS_ERR_NONE;		/* ����ִ�е������ʾû�д��� */
}
