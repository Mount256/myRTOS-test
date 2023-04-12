#include "os.h"

/* OS 系统初始化，用于初始化全局变量 */
void OSInit (OS_ERR *p_err)
{
	/* 系统用一个全局变量 OSRunning 来指示系统的运行状态。系统初始化时，默认为停止状态，即 OS_STATE_OS_STOPPED */
	OSRunning = OS_STATE_OS_STOPPED;
	
	OSTCBCurPtr 	= (OS_TCB *) 0; /* 指向当前正在运行的任务的 TCB 指针 */
	OSTCBHighRdyPtr = (OS_TCB *) 0; /* 指向就绪任务中优先级最高的任务的 TCB */
	
	OS_RdyListInit();  /* 初始化就绪列表 */
	
	*p_err = OS_ERR_NONE;  /* 函数执行到这里表示没有错误 */
}

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
	}
}

/* 系统启动函数 */
void OSStart (OS_ERR *p_err)
{
	if ( OSRunning == OS_STATE_OS_STOPPED )
	{
		OSTCBHighRdyPtr = OSRdyList[0].HeadPtr; /* 手动配置任务 1 先运行 */
		OSStartHighRdy(); 						/* 启动任务切换，不会返回 */
		*p_err = OS_ERR_FATAL_RETURN;			/* 运行至此处，说明发生了致命错误 */
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
