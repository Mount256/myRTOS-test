#include "os.h"

void OSTimeTick (void)
{
	/* 更新时基列表 */
	OS_TickListUpdate();
	
	/* 任务调度 */
	OSSched();  
}

/* 阻塞延时 */
void OSTimeDly (OS_TICK dly)
{
	CPU_SR_ALLOC();
	OS_CRITICAL_ENTER();	
	
	/* 插入时基列表 */
	OS_TickListInsert (OSTCBCurPtr, dly);
	
	/* 从就绪列表移除 */
	OS_RdyListRemove (OSTCBCurPtr);
	
	OS_CRITICAL_EXIT();		
	
	/* 任务切换 */
	OSSched();
}
