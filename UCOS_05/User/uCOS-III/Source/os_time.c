#include "os.h"

void OSTimeTick (void)
{
	OS_PRIO i;
	
	/* �������������б������ʱδ��ʱ����� 1 */
	for ( i = 0u; i < OS_CFG_PRIO_MAX; i++)
	{
		if ( OSRdyList[i].HeadPtr->TaskDelayTicks > 0u )
		{
			OSRdyList[i].HeadPtr->TaskDelayTicks --;
		}
	}
	
	/* ������� */
	OSSched();  
}

/* ������ʱ */
void OSTimeDly (OS_TICK dly)
{
	/* ��ʱʱ�� */
	OSTCBCurPtr->TaskDelayTicks = dly;
	/* �����л� */
	OSSched();
}
