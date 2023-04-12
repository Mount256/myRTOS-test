#include "os.h"

/****************************************�����б�**************************************************/
/* ��ʼ�������б� */
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

/* ����ͷ������һ�� TCB */
void OS_RdyListInsertHead (OS_TCB *p_tcb)
{
	OS_RDY_LIST	*p_rdy_list;
	OS_TCB		*p_tcb2;
	
	p_rdy_list = &OSRdyList[p_tcb->Prio];
	
	if (p_rdy_list->NbrEntries == (OS_OBJ_QTY)0)	/* ������Ϊ�� */
	{
		p_rdy_list->NbrEntries = (OS_OBJ_QTY)1;
		p_rdy_list->HeadPtr = p_tcb;
		p_rdy_list->TailPtr = p_tcb;
		p_tcb->NextPtr = (OS_TCB *)0;
		p_tcb->PrevPtr = (OS_TCB *)0;
	}
	else		/* ������Ϊ�� */
	{
		p_rdy_list->NbrEntries++;
		p_tcb2 = p_rdy_list->HeadPtr;		/* p_tcb2 = ԭ�� TCB �����ͷ��� */
		
		p_tcb->PrevPtr = (OS_TCB *)0;		/* �µ� p_tcb ��ǰָ��Ϊ 0 */
		p_tcb->NextPtr = p_rdy_list->HeadPtr;
		p_tcb2->PrevPtr = p_tcb;			/* p_tcb2 ��ǰָ��ָ�� p_tcb�����µ� p_tcb ���� TCB �����У���Ϊ�µ�ͷ��� */
		
		p_rdy_list->HeadPtr = p_tcb;		/* �¼���һ�� TCB �󣬾����б�� HeadPtrָ���µ�ͷ��� */
	}
}

/* ����β������һ�� TCB */
void OS_RdyListInsertTail (OS_TCB *p_tcb)
{
	OS_RDY_LIST	*p_rdy_list;
	OS_TCB		*p_tcb2;
	
	p_rdy_list = &OSRdyList[p_tcb->Prio];
	
	if (p_rdy_list->NbrEntries == (OS_OBJ_QTY)0)	/* ������Ϊ�� */
	{
		p_rdy_list->NbrEntries = (OS_OBJ_QTY)1;
		p_rdy_list->HeadPtr = p_tcb;
		p_rdy_list->TailPtr = p_tcb;
		p_tcb->NextPtr = (OS_TCB *)0;
		p_tcb->PrevPtr = (OS_TCB *)0;
	}
	else		/* ������Ϊ�� */
	{
		p_rdy_list->NbrEntries++;
		p_tcb2 = p_rdy_list->TailPtr;		/* p_tcb2 = ԭ�� TCB �����β��� */
		
		p_tcb->PrevPtr = p_tcb2;			/* �µ� p_tcb ��ǰָ��ָ�� p_tcb2 */
		p_tcb->NextPtr = (OS_TCB *)0;		/* �µ� p_tcb �ĺ�ָ��Ϊ 0 */
		p_tcb2->NextPtr = p_tcb;			/* p_tcb2 �ĺ�ָ��ָ�� p_tcb�����µ� p_tcb ���� TCB �����У���Ϊ�µ�β��� */
		
		p_rdy_list->TailPtr = p_tcb;		/* �¼���һ�� TCB �󣬾����б�� HeadPtrָ���µ�β��� */
	}
}

/* �ھ����б��в���һ�� TCB */
void OS_RdyListInsert (OS_TCB *p_tcb)
{	
	OS_PrioInsert (p_tcb->Prio);  /* �����ȼ����뵱ǰ���ȼ��� */
	
	if (p_tcb->Prio == OSPrioCur)
	{
		OS_RdyListInsertTail (p_tcb);  /* ������ȼ����ڵ�ǰ���ȼ����򽫵�ǰ�����������β�� */
	}
	else
	{
		OS_RdyListInsertHead (p_tcb);	/* ���򽫵�ǰ�����������ͷ�� */
	}
}

/* �� TCB ������ͷ���Ƶ�����β�� */
void OS_RdyListMoveHeadToTail (OS_RDY_LIST *p_rdy_list)
{
	OS_TCB	*p_tcb1;
	OS_TCB	*p_tcb2;
	OS_TCB	*p_tcb3;
	
	switch (p_rdy_list->NbrEntries)
	{
		case 0:		/* ����Ϊ�� */
		case 1:		/* ����ֻ��һ���ڵ� */
			break;
		
		case 2:		/* ����ֻ�������ڵ� */
			p_tcb1 = p_rdy_list->HeadPtr;
			p_tcb2 = p_rdy_list->TailPtr;
			p_tcb1->PrevPtr = p_tcb2;
			p_tcb1->NextPtr = (OS_TCB *)0;
			p_tcb2->PrevPtr = (OS_TCB *)0;
			p_tcb2->NextPtr = p_tcb1;
			p_rdy_list->HeadPtr = p_tcb2;
			p_rdy_list->TailPtr = p_tcb1;
			break;
		
		default:	/* �������������ϵĽڵ� */
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

/* �����Ƴ�һ�� TCB */
void OS_RdyListRemove (OS_TCB *p_tcb)
{
	OS_RDY_LIST	*p_rdy_list;
	OS_TCB		*p_tcb1;
	OS_TCB		*p_tcb2;
	
	/* ����Ҫɾ���� TCB �ڵ��ǰһ���ͺ�һ���ڵ� */
	p_tcb1 = p_tcb->PrevPtr;
	p_tcb2 = p_tcb->NextPtr;
	p_rdy_list = &OSRdyList[p_tcb->Prio];
	
	if (( p_tcb1 == (OS_TCB *)0 ) && ( p_tcb2 == (OS_TCB *)0 ))			/* �������ֻ��һ���ڵ� */
	{
		p_rdy_list->HeadPtr = (OS_TCB *) 0;
		p_rdy_list->TailPtr = (OS_TCB *) 0;
		p_rdy_list->NbrEntries = (OS_OBJ_QTY)0;
		OS_PrioRemove (p_tcb->Prio);
	}
	else if (( p_tcb1 == (OS_TCB *)0 ) && ( p_tcb2 != (OS_TCB *)0 ))	/* ���ɾ�����������ͷ�ڵ�(p_tcb1 = 0) */
	{
		p_tcb2->PrevPtr = (OS_TCB *) 0;
		p_rdy_list->HeadPtr = p_tcb2;
		p_rdy_list->NbrEntries--;
	}
	else if (( p_tcb1 != (OS_TCB *)0 ) && ( p_tcb2 == (OS_TCB *)0 ))	/* ���ɾ�����������β�ڵ�(p_tcb2 = 0) */
	{
		p_tcb1->NextPtr = (OS_TCB *) 0;
		p_rdy_list->TailPtr = p_tcb1;
		p_rdy_list->NbrEntries--;
	}
	else																/* ���ɾ�������������ͨ�ڵ� */
	{
		p_tcb1->NextPtr = p_tcb2;
		p_tcb2->PrevPtr = p_tcb1;
		p_rdy_list->NbrEntries--;
	}
	
	/* ��λ�Ӿ����б���ɾ���� TCB �� PrevPtr �� NextPtr ������ָ�� */
	p_tcb->PrevPtr = (OS_TCB *)0;
	p_tcb->NextPtr = (OS_TCB *)0;
}

/************************************************************************************************/

/* OS ϵͳ��ʼ�������ڳ�ʼ��ȫ�ֱ��� */
void OSInit (OS_ERR *p_err)
{
	/* ϵͳ��һ��ȫ�ֱ��� OSRunning ��ָʾϵͳ������״̬��ϵͳ��ʼ��ʱ��Ĭ��Ϊֹͣ״̬���� OS_STATE_OS_STOPPED */
	OSRunning = OS_STATE_OS_STOPPED;
	
	OSTCBCurPtr 	= (OS_TCB *) 0; /* ָ��ǰ�������е������ TCB ָ�� */
	OSTCBHighRdyPtr = (OS_TCB *) 0; /* ָ��������������ȼ���ߵ������ TCB */
	
	OSPrioCur 		= (OS_PRIO)0;	/* ��ʼ����ǰ���ȼ� */
	OSPrioHighRdy 	= (OS_PRIO)0;	/* ��ʼ��������ȼ� */
	
	OS_PrioInit();	    /* ��ʼ�����ȼ��� */
	
	OS_RdyListInit();   /* ��ʼ�������б� */
	
	OS_TickListInit();  /* ��ʼ��ʱ���б� */
	
	OS_IdleTaskInit(p_err); /* ��ʼ���������� */
	
	if (*p_err != OS_ERR_NONE) {
		return;
	}
}

/* ϵͳ�������� */
void OSStart (OS_ERR *p_err)
{
	if ( OSRunning == OS_STATE_OS_STOPPED )
	{
		OSPrioHighRdy = OS_PrioGetHighest();
		OSPrioCur = OSPrioHighRdy;
		
		OSTCBHighRdyPtr = OSRdyList[OSPrioHighRdy].HeadPtr; 
		OSTCBCurPtr = OSTCBHighRdyPtr;
		
		OSStartHighRdy(); 						/* ���������л������᷵�� */
		*p_err = OS_ERR_FATAL_RETURN;			/* �������˴���˵���������������� */
	}
	else{
		*p_err = OS_STATE_OS_RUNNING;
	}
}

/* ������� */
void OSSched (void)
{	
	CPU_SR_ALLOC();
	OS_CRITICAL_ENTER();	/* �����ٽ�� */
	
	OSPrioHighRdy   = OS_PrioGetHighest();	
	OSTCBHighRdyPtr = OSRdyList[OSPrioHighRdy].HeadPtr; 
	
	/* ���������ȼ��������ǵ�ǰ������ֱ�ӷ��أ������������л� */
	if (OSTCBHighRdyPtr == OSTCBCurPtr)	
	{
		OS_CRITICAL_EXIT();	/* �˳��ٽ�� */
		return;
	}
	
	OS_CRITICAL_EXIT();		/* �˳��ٽ�� */
	OS_TASK_SW();			/* �����л�   */
}

/* �������� */	
void OS_IdleTask (void *p_arg)
{
	p_arg = p_arg;
	
	/* ��������ʲô��������ֻ��ȫ�ֱ���OSIdleTaskCtr ++ ���� */
	for (;;)
	{
		OSIdleTaskCtr++;
	}
}

/* ���������ʼ������ */ 
void OS_IdleTaskInit (OS_ERR *p_err)
{
	OSIdleTaskCtr = (OS_IDLE_CTR) 0;		/* ���������� */
	
	OSTaskCreate ((OS_TCB*)      &OSIdleTaskTCB, 
	              (OS_TASK_PTR)  OS_IdleTask, 
	              (void *)       0,
				  (OS_PRIO)		(OS_CFG_PRIO_MAX - 1u),
	              (CPU_STK *)    OSCfg_IdleTaskStkBasePtr,
	              (CPU_STK_SIZE) OSCfg_IdleTaskStkSize,
				  (OS_TICK)		 0,
	              (OS_ERR *)     &p_err);		/* ������������ */
}

/* ������� */
void OS_TaskRdy (OS_TCB *p_tcb)
{
	OS_TickListRemove (p_tcb);		/* ��ʱ���б���ɾ�� */
	OS_RdyListInsert  (p_tcb);		/* ��������б�     */
}

/* ʱ��Ƭ���Ⱥ��� */
#if OS_CFG_SCHED_ROUND_ROBIN_EN > 0u
void OS_SchedRoundRobin (OS_RDY_LIST *p_rdy_list)
{
	OS_TCB *p_tcb;
	
	CPU_SR_ALLOC();
	CPU_CRITICAL_ENTER();		/* �����ٽ�� */
	
	p_tcb = p_rdy_list->HeadPtr;
	
	/* �������Ϊ�գ����ǿ����������˳� */
	if ((p_tcb == (OS_TCB *)0) || (p_tcb == &OSIdleTaskTCB))
	{
		CPU_CRITICAL_EXIT();	/* �˳��ٽ�� */
		return;
	}
	
	/* ���ʱ��Ƭδ���꣬ʱ��Ƭ������һ */
	if (p_tcb->TimeQuantaCtr > (OS_TICK)0)
	{
		p_tcb->TimeQuantaCtr--;
	}
	
	/* �����һ��ʱ��Ƭ��δ���꣬���˳� */
	if (p_tcb->TimeQuantaCtr > (OS_TICK)0)
	{
		CPU_CRITICAL_EXIT();	/* �˳��ٽ�� */
		return;
	}
	
	/* �������ֻ��һ���ڵ㣬���˳� */
	if (p_rdy_list->NbrEntries < (OS_OBJ_QTY)2)
	{
		CPU_CRITICAL_EXIT();	/* �˳��ٽ�� */
		return;
	}
	
	/* ���е��˴�ʱ����ζ�ŵ�ǰ�����Ѿ�������ʱ��Ƭ��������ŵ�������� */
	OS_RdyListMoveHeadToTail (p_rdy_list);
	
	/* ������һ�������ʱ��Ƭ���� */
	p_tcb = p_rdy_list->HeadPtr;
	p_tcb->TimeQuantaCtr = p_tcb->TimeQuanta;

	CPU_CRITICAL_EXIT();		/* �˳��ٽ�� */
}
#endif
