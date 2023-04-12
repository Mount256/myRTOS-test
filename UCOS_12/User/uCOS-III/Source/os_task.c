#include "os.h"

/* 初始化任务 TCB */
void OS_TaskInitTCB (OS_TCB *p_tcb)
{
	p_tcb->StkPtr 			= (CPU_STK   *)	0;
	p_tcb->StkSize 			= (CPU_STK_SIZE)0;	
	
	p_tcb->Prio 			= (OS_PRIO    )	OS_PRIO_INIT;				
	
	p_tcb->NextPtr 			= (OS_TCB    *)	0;			
	p_tcb->PrevPtr 			= (OS_TCB    *)	0;			
}

/* 任务创建函数 */
void OSTaskCreate( 	OS_TCB 			*p_tcb,  		/* TCB指针 */
					OS_TASK_PTR 	p_task,  		/* 任务函数名 */
					void 			*p_arg,  		/* 任务的形参 */
					OS_PRIO			prio,			/* 任务优先级 */
					CPU_STK 		*p_stk_base, 	/* 任务栈的起始地址 */
					CPU_STK_SIZE 	stk_size,		/* 任务栈大小 */
					OS_TICK			time_quanta,	/* 时间片个数 */
					OS_ERR 			*p_err )		/* 错误码 */
{
	CPU_STK		*p_sp;
	CPU_SR_ALLOC();
	
	OS_TaskInitTCB (p_tcb);
	
	p_sp = OSTaskStkInit ( 	p_task,
							p_arg,
							p_stk_base,
							stk_size );  /* 任务栈初始化函数 */
	p_tcb->Prio		= prio;		/* 任务优先级保存在 TCB 的 prio 中 */
	p_tcb->StkPtr 	= p_sp;    	/* 剩余栈的栈顶指针 p_sp 保存到任务控制块 TCB 的 StkPtr 中 */
	p_tcb->StkSize 	= stk_size; /* 将任务栈的大小保存到任务控制块 TCB 的成员 StkSize 中 */
	
	p_tcb->TimeQuanta = time_quanta;
	
#if OS_CFG_SCHED_ROUND_ROBIN_EN > 0u
	p_tcb->TimeQuantaCtr = time_quanta;
#endif
	
	OS_CRITICAL_ENTER();	/* 进入临界段 */
	
	/* 将任务添加到就绪列表 */
	OS_PrioInsert (p_tcb->Prio);
	OS_RdyListInsertTail (p_tcb);
	
	OS_CRITICAL_EXIT();		/* 退出临界段 */
	
	*p_err = OS_ERR_NONE;		/* 函数执行到这里表示没有错误 */
}

/* 任务挂起函数 */
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
		if (OSSchedLockNestingCtr > (OS_NESTING_CTR)0)	/* 如果调度器锁住则不能挂起自己 */
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
	
	OSSched();	/* 任务切换 */
	
	CPU_CRITICAL_EXIT();
}
#endif

/* 任务恢复函数 */
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
	
	OSSched();	/* 任务切换 */
	
	CPU_CRITICAL_EXIT();
}
#endif

/* 任务删除函数 */
#if OS_CFG_TASK_DEL_EN > 0u
void OSTaskDel (OS_TCB *p_tcb, OS_ERR *p_err)
{
	CPU_SR_ALLOC();
	
	if (p_tcb == &OSIdleTaskTCB)	/* 不能删除空闲任务 */
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
	
	OSSched();	/* 任务切换 */
	
	*p_err = OS_ERR_NONE;
}
#endif
