#include "os.h"

/* ��ʼ������ TCB */
void OS_TaskInitTCB (OS_TCB *p_tcb)
{
	p_tcb->StkPtr 			= (CPU_STK   *)	0;
	p_tcb->StkSize 			= (CPU_STK_SIZE)0;	
	
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
					OS_TICK			time_quanta,	/* ʱ��Ƭ���� */
					OS_ERR 			*p_err )		/* ������ */
{
	CPU_STK		*p_sp;
	CPU_SR_ALLOC();
	
	OS_TaskInitTCB (p_tcb);
	
	p_sp = OSTaskStkInit ( 	p_task,
							p_arg,
							p_stk_base,
							stk_size );  /* ����ջ��ʼ������ */
	p_tcb->Prio		= prio;		/* �������ȼ������� TCB �� prio �� */
	p_tcb->StkPtr 	= p_sp;    	/* ʣ��ջ��ջ��ָ�� p_sp ���浽������ƿ� TCB �� StkPtr �� */
	p_tcb->StkSize 	= stk_size; /* ������ջ�Ĵ�С���浽������ƿ� TCB �ĳ�Ա StkSize �� */
	
	p_tcb->TimeQuanta = time_quanta;
	
#if OS_CFG_SCHED_ROUND_ROBIN_EN > 0u
	p_tcb->TimeQuantaCtr = time_quanta;
#endif
	
	OS_CRITICAL_ENTER();	/* �����ٽ�� */
	
	/* ��������ӵ������б� */
	OS_PrioInsert (p_tcb->Prio);
	OS_RdyListInsertTail (p_tcb);
	
	OS_CRITICAL_EXIT();		/* �˳��ٽ�� */
	
	*p_err = OS_ERR_NONE;		/* ����ִ�е������ʾû�д��� */
}

/* ��������� */
#if OS_CFG_TASK_SUSPENDED_EN > 0u
void OSTaskSuspend (OS_TCB *p_tcb, OS_ERR *p_err)
{
	CPU_SR_ALLOC();	
	CPU_CRITICAL_ENTER();
	
	if (p_tcb == (OS_TCB *)0)
	{
		p_tcb = OSTCBCurPtr;
	}
	
	if (p_tcb == OSTCBCurPtr)
	{
		if (OSSchedLockNestingCtr > (OS_NESTING_CTR)0)	/* �����������ס���ܹ����Լ� */
		{
			CPU_CRITICAL_EXIT();
			*p_err = OS_ERR_SCHED_LOCKED;
			return;
		}
	}
	
	*p_err = OS_ERR_NONE;
	
	switch (p_tcb->TaskState)
	{
		case OS_TASK_STATE_RDY:
			OS_CRITICAL_ENTER_CPU_CRITICAL_EXIT();
			p_tcb->TaskState  = OS_TASK_STATE_SUSPENDED;
			p_tcb->SuspendCtr = (OS_NESTING_CTR)1;
			OS_RdyListRemove (p_tcb);
			OS_CRITICAL_EXIT_NO_SCHED();
			break;
		
		case OS_TASK_STATE_DLY:
			p_tcb->TaskState  = OS_TASK_STATE_DLY_SUSPENDED;
			p_tcb->SuspendCtr = (OS_NESTING_CTR)1;
			CPU_CRITICAL_EXIT();
			break;
		
		case OS_TASK_STATE_PEND:
			p_tcb->TaskState  = OS_TASK_STATE_PEND_SUSPENDED;
			p_tcb->SuspendCtr = (OS_NESTING_CTR)1;
			CPU_CRITICAL_EXIT();
			break;
		
		case OS_TASK_STATE_PEND_TIMEOUT:
			p_tcb->TaskState  = OS_TASK_STATE_PEND_TIMEOUT_SUSPENDED;
			p_tcb->SuspendCtr = (OS_NESTING_CTR)1;
			CPU_CRITICAL_EXIT();
			break;
		
		case OS_TASK_STATE_SUSPENDED:
		case OS_TASK_STATE_DLY_SUSPENDED:
		case OS_TASK_STATE_PEND_SUSPENDED:
		case OS_TASK_STATE_PEND_TIMEOUT_SUSPENDED:
			p_tcb->SuspendCtr++;
			CPU_CRITICAL_EXIT();
			break;
		
		default:
			CPU_CRITICAL_EXIT();
			*p_err = OS_ERR_STATE_INVALID;
			return;
	}
	
	OSSched();	/* �����л� */
	
	CPU_CRITICAL_EXIT();
}
#endif

/* ����ָ����� */
#if OS_CFG_TASK_SUSPENDED_EN > 0u
void OSTaskResume (OS_TCB *p_tcb, OS_ERR *p_err)
{
	CPU_SR_ALLOC();
	CPU_CRITICAL_ENTER();
	
	*p_err = OS_ERR_NONE;
	
	switch (p_tcb->TaskState)
	{
		case OS_TASK_STATE_RDY:
		case OS_TASK_STATE_DLY:
		case OS_TASK_STATE_PEND:
		case OS_TASK_STATE_PEND_TIMEOUT:
			CPU_CRITICAL_EXIT();
			*p_err = OS_ERR_TASK_NOT_SUSPENDED;
			break;
		
		case OS_TASK_STATE_SUSPENDED:
			OS_CRITICAL_ENTER_CPU_CRITICAL_EXIT();
			p_tcb->SuspendCtr--;
			if (p_tcb->SuspendCtr == (OS_NESTING_CTR)0)
			{
				p_tcb->TaskState = OS_TASK_STATE_RDY;
				OS_TaskRdy (p_tcb);
			}
			OS_CRITICAL_EXIT_NO_SCHED();
			break;
		
		case OS_TASK_STATE_DLY_SUSPENDED:
			p_tcb->SuspendCtr--;
			if (p_tcb->SuspendCtr == (OS_NESTING_CTR)0)
			{
				p_tcb->TaskState = OS_TASK_STATE_DLY;
			}
			CPU_CRITICAL_EXIT();
			break;
			
		case OS_TASK_STATE_PEND_SUSPENDED:
			p_tcb->SuspendCtr--;
			if (p_tcb->SuspendCtr == (OS_NESTING_CTR)0)
			{
				p_tcb->TaskState = OS_TASK_STATE_PEND;
			}
			CPU_CRITICAL_EXIT();
			break;
			
		case OS_TASK_STATE_PEND_TIMEOUT_SUSPENDED:
			p_tcb->SuspendCtr--;
			if (p_tcb->SuspendCtr == (OS_NESTING_CTR)0)
			{
				p_tcb->TaskState = OS_TASK_STATE_PEND_TIMEOUT;
			}
			CPU_CRITICAL_EXIT();
			break;
			
		default:
			CPU_CRITICAL_EXIT();
			*p_err = OS_ERR_STATE_INVALID;
			return;
	}
	
	OSSched();	/* �����л� */
	
	CPU_CRITICAL_EXIT();
}
#endif

/* ����ɾ������ */
#if OS_CFG_TASK_DEL_EN > 0u
void OSTaskDel (OS_TCB *p_tcb, OS_ERR *p_err)
{
	CPU_SR_ALLOC();
	
	if (p_tcb == &OSIdleTaskTCB)	/* ����ɾ���������� */
	{
		*p_err = OS_ERR_TASK_DEL_IDLE;
		return;
	}
	
	if (p_tcb == (OS_TCB *)0)
	{
		CPU_CRITICAL_ENTER();
		p_tcb = OSTCBCurPtr;
		CPU_CRITICAL_EXIT();
	}
	
	OS_CRITICAL_ENTER();
	
	switch (p_tcb->TaskState)
	{
		case OS_TASK_STATE_RDY:
			OS_RdyListRemove (p_tcb);
			break;
		
		case OS_TASK_STATE_SUSPENDED:
			break;
		
		case OS_TASK_STATE_DLY:
		case OS_TASK_STATE_DLY_SUSPENDED:
			OS_TickListRemove (p_tcb);
			break;
		
		case OS_TASK_STATE_PEND:
		case OS_TASK_STATE_PEND_TIMEOUT:
		case OS_TASK_STATE_PEND_SUSPENDED:
		case OS_TASK_STATE_PEND_TIMEOUT_SUSPENDED:
			OS_TickListRemove (p_tcb);
		
		default:
			OS_CRITICAL_EXIT();
			*p_err = OS_ERR_STATE_INVALID;
			return;
	}
	
	OS_TaskInitTCB (p_tcb);
	p_tcb->TaskState = OS_TASK_STATE_DEL;
	
	OS_CRITICAL_ENTER_CPU_CRITICAL_EXIT();
	
	OSSched();	/* �����л� */
	
	*p_err = OS_ERR_NONE;
}
#endif
