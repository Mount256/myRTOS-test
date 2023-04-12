#include "os.h"

/* OS ϵͳ��ʼ�������ڳ�ʼ��ȫ�ֱ��� */
void OSInit (OS_ERR *p_err)
{
	/* ϵͳ��һ��ȫ�ֱ��� OSRunning ��ָʾϵͳ������״̬��ϵͳ��ʼ��ʱ��Ĭ��Ϊֹͣ״̬���� OS_STATE_OS_STOPPED */
	OSRunning = OS_STATE_OS_STOPPED;
	
	OSTCBCurPtr 	= (OS_TCB *) 0; /* ָ��ǰ�������е������ TCB ָ�� */
	OSTCBHighRdyPtr = (OS_TCB *) 0; /* ָ��������������ȼ���ߵ������ TCB */
	
	OS_RdyListInit();  /* ��ʼ�������б� */
	
	*p_err = OS_ERR_NONE;  /* ����ִ�е������ʾû�д��� */
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

void OSSched (void)
{
	if( OSTCBCurPtr == OSRdyList[0].HeadPtr )
	{
		OSTCBHighRdyPtr = OSRdyList[1].HeadPtr;
	}
	else
	{
		OSTCBHighRdyPtr = OSRdyList[0].HeadPtr;
	}
	
	OS_TASK_SW();
}
