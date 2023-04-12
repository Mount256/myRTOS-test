#include "os.h"

void OSTimeTick (void)
{
	/* ����ʱ���б� */
	OS_TickListUpdate();
	
	/* ʱ��Ƭ���� */
#if OS_CFG_SCHED_ROUND_ROBIN_EN > 0u
	OS_SchedRoundRobin (&OSRdyList[OSPrioCur]);
#endif
	
	/* ������� */
	OSSched();  
}

/* ������ʱ */
void OSTimeDly (OS_TICK dly)
{
	CPU_SR_ALLOC();
	OS_CRITICAL_ENTER();	
	
	/* ����ʱ���б� */
	OS_TickListInsert (OSTCBCurPtr, dly);
	
	/* �Ӿ����б��Ƴ� */
	OS_RdyListRemove (OSTCBCurPtr);
	
	OS_CRITICAL_EXIT();		
	
	/* �����л� */
	OSSched();
}
