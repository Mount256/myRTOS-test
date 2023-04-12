#include "os.h"

void OSTimeTick (void)
{
	OS_PRIO i;
	
	/* 遍历整个就绪列表，如果延时未到时，则减 1 */
	for ( i = 0u; i < OS_CFG_PRIO_MAX; i++)
	{
		if ( OSRdyList[i].HeadPtr->TaskDelayTicks > 0u )
		{
			OSRdyList[i].HeadPtr->TaskDelayTicks --;
		}
	}
	
	/* 任务调度 */
	OSSched();  
}

/* 阻塞延时 */
void OSTimeDly (OS_TICK dly)
{
	/* 延时时间 */
	OSTCBCurPtr->TaskDelayTicks = dly;
	/* 任务切换 */
	OSSched();
}
