#include "os.h"

/* OS ϵͳ��ʼ�������ڳ�ʼ��ȫ�ֱ��� */
void OSInit (OS_ERR *p_err)
{
	/* ϵͳ��һ��ȫ�ֱ��� OSRunning ��ָʾϵͳ������״̬��ϵͳ��ʼ��ʱ��Ĭ��Ϊֹͣ״̬���� OS_STATE_OS_STOPPED */
	OSRunning = OS_STATE_OS_STOPPED;
	
	OSTCBCurPtr 	= (OS_TCB *) 0; /* ָ��ǰ�������е������ TCB ָ�� */
	OSTCBHighRdyPtr = (OS_TCB *) 0; /* ָ��������������ȼ���ߵ������ TCB */
	
	OS_RdyListInit();  /* ��ʼ�������б� */
	
	OS_IdleTaskInit(p_err); /* ��ʼ���������� */
	
	if (*p_err != OS_ERR_NONE) {
		return;
	}
}

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
	}
}

/* ϵͳ�������� */
void OSStart (OS_ERR *p_err)
{
	if ( OSRunning == OS_STATE_OS_STOPPED )
	{
		OSTCBHighRdyPtr = OSRdyList[0].HeadPtr; /* �ֶ��������� 1 ������ */
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
	/*
	if( OSTCBCurPtr == OSRdyList[0].HeadPtr )
	{
		OSTCBHighRdyPtr = OSRdyList[1].HeadPtr;
	}
	else
	{
		OSTCBHighRdyPtr = OSRdyList[0].HeadPtr;
	}
	*/
	
	if ( OSTCBCurPtr == &OSIdleTaskTCB )	/* (a) �������ڵ������ǿ������� */
	{
		if ( OSRdyList[0].HeadPtr->TaskDelayTicks == 0 )	/* ������� 1 ��ʱ���� */
		{
			OSTCBHighRdyPtr = OSRdyList[0].HeadPtr;
		}
		else if ( OSRdyList[1].HeadPtr->TaskDelayTicks == 0 )	/* ������� 2 ��ʱ���� */
		{
			OSTCBHighRdyPtr = OSRdyList[1].HeadPtr;
		}
		else
		{
			return;
		}
	}
	else if ( OSTCBCurPtr == OSRdyList[0].HeadPtr )	/* (b) �������ڵ����������� 1 */
	{
		if ( OSRdyList[1].HeadPtr->TaskDelayTicks == 0 )	/* ������� 2 ��ʱ���� */
		{
			OSTCBHighRdyPtr = OSRdyList[1].HeadPtr;
		}
		else if ( OSTCBCurPtr->TaskDelayTicks != 0 )	/* ������� 1 �Լ�������ʱ */
		{
			OSTCBHighRdyPtr = &OSIdleTaskTCB;
		}
		else
		{
			return;
		}
	}
	else if ( OSTCBCurPtr == OSRdyList[1].HeadPtr )	/* (c) �������ڵ����������� 2 */
	{
		if ( OSRdyList[0].HeadPtr->TaskDelayTicks == 0 )	/* ������� 1 ��ʱ���� */
		{
			OSTCBHighRdyPtr = OSRdyList[0].HeadPtr;
		}
		else if ( OSTCBCurPtr->TaskDelayTicks != 0 )	/* ������� 2 �Լ�������ʱ */
		{
			OSTCBHighRdyPtr = &OSIdleTaskTCB;
		}
		else
		{
			return;
		}
	}
	
	OS_TASK_SW();
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
	              (CPU_STK *)    OSCfg_IdleTaskStkBasePtr,
	              (CPU_STK_SIZE) OSCfg_IdleTaskStkSize,
	              (OS_ERR *)     &p_err);		/* ������������ */
}
