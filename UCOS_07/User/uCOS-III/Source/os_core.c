#include "os.h"

/* OS 系统初始化，用于初始化全局变量 */
void OSInit (OS_ERR *p_err)
{
	/* 系统用一个全局变量 OSRunning 来指示系统的运行状态。系统初始化时，默认为停止状态，即 OS_STATE_OS_STOPPED */
	OSRunning = OS_STATE_OS_STOPPED;
	
	OSTCBCurPtr 	= (OS_TCB *) 0; /* 指向当前正在运行的任务的 TCB 指针 */
	OSTCBHighRdyPtr = (OS_TCB *) 0; /* 指向就绪任务中优先级最高的任务的 TCB */
	
	OS_RdyListInit();  /* 初始化就绪列表 */
	
	OS_IdleTaskInit(p_err); /* 初始化空闲任务 */
	
	if (*p_err != OS_ERR_NONE) {
		return;
	}
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

/* 任务调度 */
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
	
	if ( OSTCBCurPtr == &OSIdleTaskTCB )	/* (a) 假如现在的任务是空闲任务 */
	{
		if ( OSRdyList[0].HeadPtr->TaskDelayTicks == 0 )	/* 如果任务 1 延时结束 */
		{
			OSTCBHighRdyPtr = OSRdyList[0].HeadPtr;
		}
		else if ( OSRdyList[1].HeadPtr->TaskDelayTicks == 0 )	/* 如果任务 2 延时结束 */
		{
			OSTCBHighRdyPtr = OSRdyList[1].HeadPtr;
		}
		else
		{
			return;
		}
	}
	else if ( OSTCBCurPtr == OSRdyList[0].HeadPtr )	/* (b) 假如现在的任务是任务 1 */
	{
		if ( OSRdyList[1].HeadPtr->TaskDelayTicks == 0 )	/* 如果任务 2 延时结束 */
		{
			OSTCBHighRdyPtr = OSRdyList[1].HeadPtr;
		}
		else if ( OSTCBCurPtr->TaskDelayTicks != 0 )	/* 如果任务 1 自己还在延时 */
		{
			OSTCBHighRdyPtr = &OSIdleTaskTCB;
		}
		else
		{
			return;
		}
	}
	else if ( OSTCBCurPtr == OSRdyList[1].HeadPtr )	/* (c) 假如现在的任务是任务 2 */
	{
		if ( OSRdyList[0].HeadPtr->TaskDelayTicks == 0 )	/* 如果任务 1 延时结束 */
		{
			OSTCBHighRdyPtr = OSRdyList[0].HeadPtr;
		}
		else if ( OSTCBCurPtr->TaskDelayTicks != 0 )	/* 如果任务 2 自己还在延时 */
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

/* 空闲任务 */	
void OS_IdleTask (void *p_arg)
{
	p_arg = p_arg;
	
	/* 空闲任务什么都不做，只对全局变量OSIdleTaskCtr ++ 操作 */
	for (;;)
	{
		OSIdleTaskCtr++;
	}
}

/* 空闲任务初始化函数 */ 
void OS_IdleTaskInit (OS_ERR *p_err)
{
	OSIdleTaskCtr = (OS_IDLE_CTR) 0;		/* 计数器清零 */
	
	OSTaskCreate ((OS_TCB*)      &OSIdleTaskTCB, 
	              (OS_TASK_PTR)  OS_IdleTask, 
	              (void *)       0,
	              (CPU_STK *)    OSCfg_IdleTaskStkBasePtr,
	              (CPU_STK_SIZE) OSCfg_IdleTaskStkSize,
	              (OS_ERR *)     &p_err);		/* 创建空闲任务 */
}
