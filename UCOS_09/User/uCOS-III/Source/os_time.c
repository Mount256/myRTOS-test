#include "os.h"

void OSTimeTick (void)
{
	OS_PRIO i;
	
	CPU_SR_ALLOC();
	OS_CRITICAL_ENTER();	/* �����ٽ�� */
	
	/* �������������б� */
	for ( i = 0u; i < OS_CFG_PRIO_MAX; i++)
	{
		if ( OSRdyList[i].HeadPtr->TaskDelayTicks > 0u )	/* �����ʱδ��ʱ����� 1 */
		{
			OSRdyList[i].HeadPtr->TaskDelayTicks --;
			if (OSRdyList[i].HeadPtr->TaskDelayTicks == 0u)	/* �����ʱʱ���ѵ������������ */
			{
				OS_PrioInsert (i);
			}
		}
	}
	
	OS_CRITICAL_EXIT();		/* �˳��ٽ�� */
	
	/* ������� */
	OSSched();  
}

/* ������ʱ */
void OSTimeDly (OS_TICK dly)
{
	CPU_SR_ALLOC();
	OS_CRITICAL_ENTER();	/* �����ٽ�� */
	
	/* ��ʱʱ�� */
	OSTCBCurPtr->TaskDelayTicks = dly;
	OS_PrioRemove (OSTCBCurPtr->Prio);
	
	OS_CRITICAL_EXIT();		/* �˳��ٽ�� */
	
	/* �����л� */
	OSSched();
}
