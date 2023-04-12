#include "os.h"

/****************************************就绪列表**************************************************/
/* 初始化就绪列表 */
void OS_RdyListInit (void)
{
	OS_PRIO		i;
	OS_RDY_LIST	*p_rdy_list;
	
	for ( i = 0u; i < OS_CFG_PRIO_MAX; i++ )
	{
		p_rdy_list = &OSRdyList[i];
		p_rdy_list->HeadPtr = (OS_TCB *) 0;
		p_rdy_list->TailPtr = (OS_TCB *) 0;
		p_rdy_list->NbrEntries = (OS_OBJ_QTY)0;
	}
}

/* 链表头部插入一个 TCB */
void OS_RdyListInsertHead (OS_TCB *p_tcb)
{
	OS_RDY_LIST	*p_rdy_list;
	OS_TCB		*p_tcb2;
	
	p_rdy_list = &OSRdyList[p_tcb->Prio];
	
	if (p_rdy_list->NbrEntries == (OS_OBJ_QTY)0)	/* 若链表为空 */
	{
		p_rdy_list->NbrEntries = (OS_OBJ_QTY)1;
		p_rdy_list->HeadPtr = p_tcb;
		p_rdy_list->TailPtr = p_tcb;
		p_tcb->NextPtr = (OS_TCB *)0;
		p_tcb->PrevPtr = (OS_TCB *)0;
	}
	else		/* 若链表不为空 */
	{
		p_rdy_list->NbrEntries++;
		p_tcb2 = p_rdy_list->HeadPtr;		/* p_tcb2 = 原先 TCB 链表的头结点 */
		
		p_tcb->PrevPtr = (OS_TCB *)0;		/* 新的 p_tcb 的前指针为 0 */
		p_tcb->NextPtr = p_rdy_list->HeadPtr;
		p_tcb2->PrevPtr = p_tcb;			/* p_tcb2 的前指针指向 p_tcb，即新的 p_tcb 加入 TCB 链表中，成为新的头结点 */
		
		p_rdy_list->HeadPtr = p_tcb;		/* 新加入一个 TCB 后，就绪列表的 HeadPtr指向新的头结点 */
	}
}

/* 链表尾部插入一个 TCB */
void OS_RdyListInsertTail (OS_TCB *p_tcb)
{
	OS_RDY_LIST	*p_rdy_list;
	OS_TCB		*p_tcb2;
	
	p_rdy_list = &OSRdyList[p_tcb->Prio];
	
	if (p_rdy_list->NbrEntries == (OS_OBJ_QTY)0)	/* 若链表为空 */
	{
		p_rdy_list->NbrEntries = (OS_OBJ_QTY)1;
		p_rdy_list->HeadPtr = p_tcb;
		p_rdy_list->TailPtr = p_tcb;
		p_tcb->NextPtr = (OS_TCB *)0;
		p_tcb->PrevPtr = (OS_TCB *)0;
	}
	else		/* 若链表不为空 */
	{
		p_rdy_list->NbrEntries++;
		p_tcb2 = p_rdy_list->TailPtr;		/* p_tcb2 = 原先 TCB 链表的尾结点 */
		
		p_tcb->PrevPtr = p_tcb2;			/* 新的 p_tcb 的前指针指向 p_tcb2 */
		p_tcb->NextPtr = (OS_TCB *)0;		/* 新的 p_tcb 的后指针为 0 */
		p_tcb2->NextPtr = p_tcb;			/* p_tcb2 的后指针指向 p_tcb，即新的 p_tcb 加入 TCB 链表中，成为新的尾结点 */
		
		p_rdy_list->TailPtr = p_tcb;		/* 新加入一个 TCB 后，就绪列表的 HeadPtr指向新的尾结点 */
	}
}

/* 在就绪列表中插入一个 TCB */
void OS_RdyListInsert (OS_TCB *p_tcb)
{	
	OS_PrioInsert (p_tcb->Prio);  /* 将优先级插入当前优先级中 */
	
	if (p_tcb->Prio == OSPrioCur)
	{
		OS_RdyListInsertTail (p_tcb);  /* 如果优先级等于当前优先级，则将当前任务插入链表尾部 */
	}
	else
	{
		OS_RdyListInsertHead (p_tcb);	/* 否则将当前任务插入链表头部 */
	}
}

/* 将 TCB 从链表头部移到链表尾部 */
void OS_RdyListMoveHeadToTail (OS_RDY_LIST *p_rdy_list)
{
	OS_TCB	*p_tcb1;
	OS_TCB	*p_tcb2;
	OS_TCB	*p_tcb3;
	
	switch (p_rdy_list->NbrEntries)
	{
		case 0:		/* 链表为空 */
		case 1:		/* 链表只有一个节点 */
			break;
		
		case 2:		/* 链表只有两个节点 */
			p_tcb1 = p_rdy_list->HeadPtr;
			p_tcb2 = p_rdy_list->TailPtr;
			p_tcb1->PrevPtr = p_tcb2;
			p_tcb1->NextPtr = (OS_TCB *)0;
			p_tcb2->PrevPtr = (OS_TCB *)0;
			p_tcb2->NextPtr = p_tcb1;
			p_rdy_list->HeadPtr = p_tcb2;
			p_rdy_list->TailPtr = p_tcb1;
			break;
		
		default:	/* 链表有两个以上的节点 */
			p_tcb1 = p_rdy_list->HeadPtr;
			p_tcb2 = p_rdy_list->TailPtr;
			p_tcb3 = p_tcb1->NextPtr;
			
			p_tcb1->PrevPtr = p_tcb2;
			p_tcb1->NextPtr = (OS_TCB *)0;
			p_tcb3->PrevPtr = (OS_TCB *)0;
			p_tcb2->NextPtr = p_tcb1;
		
			p_rdy_list->HeadPtr = p_tcb3;
			p_rdy_list->TailPtr = p_tcb1;
			break;
	}
}

/* 链表移除一个 TCB */
void OS_RdyListRemove (OS_TCB *p_tcb)
{
	OS_RDY_LIST	*p_rdy_list;
	OS_TCB		*p_tcb1;
	OS_TCB		*p_tcb2;
	
	/* 保存要删除的 TCB 节点的前一个和后一个节点 */
	p_tcb1 = p_tcb->PrevPtr;
	p_tcb2 = p_tcb->NextPtr;
	p_rdy_list = &OSRdyList[p_tcb->Prio];
	
	if (( p_tcb1 == (OS_TCB *)0 ) && ( p_tcb2 == (OS_TCB *)0 ))			/* 如果链表只有一个节点 */
	{
		p_rdy_list->HeadPtr = (OS_TCB *) 0;
		p_rdy_list->TailPtr = (OS_TCB *) 0;
		p_rdy_list->NbrEntries = (OS_OBJ_QTY)0;
		OS_PrioRemove (p_tcb->Prio);
	}
	else if (( p_tcb1 == (OS_TCB *)0 ) && ( p_tcb2 != (OS_TCB *)0 ))	/* 如果删除的是链表的头节点(p_tcb1 = 0) */
	{
		p_tcb2->PrevPtr = (OS_TCB *) 0;
		p_rdy_list->HeadPtr = p_tcb2;
		p_rdy_list->NbrEntries--;
	}
	else if (( p_tcb1 != (OS_TCB *)0 ) && ( p_tcb2 == (OS_TCB *)0 ))	/* 如果删除的是链表的尾节点(p_tcb2 = 0) */
	{
		p_tcb1->NextPtr = (OS_TCB *) 0;
		p_rdy_list->TailPtr = p_tcb1;
		p_rdy_list->NbrEntries--;
	}
	else																/* 如果删除的是链表的普通节点 */
	{
		p_tcb1->NextPtr = p_tcb2;
		p_tcb2->PrevPtr = p_tcb1;
		p_rdy_list->NbrEntries--;
	}
	
	/* 复位从就绪列表中删除的 TCB 的 PrevPtr 和 NextPtr 这两个指针 */
	p_tcb->PrevPtr = (OS_TCB *)0;
	p_tcb->NextPtr = (OS_TCB *)0;
}

/************************************************************************************************/

/* OS 系统初始化，用于初始化全局变量 */
void OSInit (OS_ERR *p_err)
{
	/* 系统用一个全局变量 OSRunning 来指示系统的运行状态。系统初始化时，默认为停止状态，即 OS_STATE_OS_STOPPED */
	OSRunning = OS_STATE_OS_STOPPED;
	
	OSTCBCurPtr 	= (OS_TCB *) 0; /* 指向当前正在运行的任务的 TCB 指针 */
	OSTCBHighRdyPtr = (OS_TCB *) 0; /* 指向就绪任务中优先级最高的任务的 TCB */
	
	OSPrioCur 		= (OS_PRIO)0;	/* 初始化当前优先级 */
	OSPrioHighRdy 	= (OS_PRIO)0;	/* 初始化最高优先级 */
	
	OS_PrioInit();	    /* 初始化优先级表 */
	
	OS_RdyListInit();   /* 初始化就绪列表 */
	
	OS_TickListInit();  /* 初始化时基列表 */
	
	OS_IdleTaskInit(p_err); /* 初始化空闲任务 */
	
	if (*p_err != OS_ERR_NONE) {
		return;
	}
}

/* 系统启动函数 */
void OSStart (OS_ERR *p_err)
{
	if ( OSRunning == OS_STATE_OS_STOPPED )
	{
		OSPrioHighRdy = OS_PrioGetHighest();
		OSPrioCur = OSPrioHighRdy;
		
		OSTCBHighRdyPtr = OSRdyList[OSPrioHighRdy].HeadPtr; 
		OSTCBCurPtr = OSTCBHighRdyPtr;
		
		OSStartHighRdy(); 						/* 启动任务切换，不会返回 */
		*p_err = OS_ERR_FATAL_RETURN;			/* 运行至此处，说明发生了致命错误 */
	}
	else{
		*p_err = OS_STATE_OS_RUNNING;
	}
}

/* 任务调度 */
void OSSched (void)
{	
	CPU_SR_ALLOC();
	OS_CRITICAL_ENTER();	/* 进入临界段 */
	
	OSPrioHighRdy   = OS_PrioGetHighest();	
	OSTCBHighRdyPtr = OSRdyList[OSPrioHighRdy].HeadPtr; 
	
	/* 如果最高优先级的任务是当前任务则直接返回，不进行任务切换 */
	if (OSTCBHighRdyPtr == OSTCBCurPtr)	
	{
		OS_CRITICAL_EXIT();	/* 退出临界段 */
		return;
	}
	
	OS_CRITICAL_EXIT();		/* 退出临界段 */
	OS_TASK_SW();			/* 任务切换   */
}

/* 空闲任务 */	
void OS_IdleTask (void *p_arg)
{
	p_arg = p_arg;
	
	/* 空闲任务什么都不做，只对全局变量OSIdleTaskCtr ++ 操作 */
	for (;;)
	{
		OSIdleTaskCtr++;
	}
}

/* 空闲任务初始化函数 */ 
void OS_IdleTaskInit (OS_ERR *p_err)
{
	OSIdleTaskCtr = (OS_IDLE_CTR) 0;		/* 计数器清零 */
	
	OSTaskCreate ((OS_TCB*)      &OSIdleTaskTCB, 
	              (OS_TASK_PTR)  OS_IdleTask, 
	              (void *)       0,
				  (OS_PRIO)		(OS_CFG_PRIO_MAX - 1u),
	              (CPU_STK *)    OSCfg_IdleTaskStkBasePtr,
	              (CPU_STK_SIZE) OSCfg_IdleTaskStkSize,
				  (OS_TICK)		 0,
	              (OS_ERR *)     &p_err);		/* 创建空闲任务 */
}

/* 任务就绪 */
void OS_TaskRdy (OS_TCB *p_tcb)
{
	OS_TickListRemove (p_tcb);		/* 从时基列表中删除 */
	OS_RdyListInsert  (p_tcb);		/* 插入就绪列表     */
}

/* 时间片调度函数 */
#if OS_CFG_SCHED_ROUND_ROBIN_EN > 0u
void OS_SchedRoundRobin (OS_RDY_LIST *p_rdy_list)
{
	OS_TCB *p_tcb;
	
	CPU_SR_ALLOC();
	CPU_CRITICAL_ENTER();		/* 进入临界段 */
	
	p_tcb = p_rdy_list->HeadPtr;
	
	/* 如果链表为空，或是空闲任务，则退出 */
	if ((p_tcb == (OS_TCB *)0) || (p_tcb == &OSIdleTaskTCB))
	{
		CPU_CRITICAL_EXIT();	/* 退出临界段 */
		return;
	}
	
	/* 如果时间片未用完，时间片个数减一 */
	if (p_tcb->TimeQuantaCtr > (OS_TICK)0)
	{
		p_tcb->TimeQuantaCtr--;
	}
	
	/* 如果减一后，时间片仍未用完，则退出 */
	if (p_tcb->TimeQuantaCtr > (OS_TICK)0)
	{
		CPU_CRITICAL_EXIT();	/* 退出临界段 */
		return;
	}
	
	/* 如果链表只有一个节点，则退出 */
	if (p_rdy_list->NbrEntries < (OS_OBJ_QTY)2)
	{
		CPU_CRITICAL_EXIT();	/* 退出临界段 */
		return;
	}
	
	/* 运行到此处时，意味着当前任务已经用完了时间片，将任务放到链表最后 */
	OS_RdyListMoveHeadToTail (p_rdy_list);
	
	/* 重设下一个任务的时间片计数 */
	p_tcb = p_rdy_list->HeadPtr;
	p_tcb->TimeQuantaCtr = p_tcb->TimeQuanta;

	CPU_CRITICAL_EXIT();		/* 退出临界段 */
}
#endif
