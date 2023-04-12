#include "os.h"

void OSTimeTick (void)
{
	OS_PRIO i;
	
	CPU_SR_ALLOC();
	OS_CRITICAL_ENTER();	/* 进入临界段 */
	
	/* 遍历整个就绪列表 */
	for ( i = 0u; i < OS_CFG_PRIO_MAX; i++)
	{
		if ( OSRdyList[i].HeadPtr->TaskDelayTicks > 0u )	/* 如果延时未到时，则减 1 */
		{
			OSRdyList[i].HeadPtr->TaskDelayTicks --;
			if (OSRdyList[i].HeadPtr->TaskDelayTicks == 0u)	/* 如果延时时间已到，让任务就绪 */
			{
				OS_PrioInsert (i);
			}
		}
	}
	
	OS_CRITICAL_EXIT();		/* 退出临界段 */
	
	/* 任务调度 */
	OSSched();  
}

/* 阻塞延时 */
void OSTimeDly (OS_TICK dly)
{
	CPU_SR_ALLOC();
	OS_CRITICAL_ENTER();	/* 进入临界段 */
	
	/* 延时时间 */
	OSTCBCurPtr->TaskDelayTicks = dly;
	OS_PrioRemove (OSTCBCurPtr->Prio);
	
	OS_CRITICAL_EXIT();		/* 退出临界段 */
	
	/* 任务切换 */
	OSSched();
}
