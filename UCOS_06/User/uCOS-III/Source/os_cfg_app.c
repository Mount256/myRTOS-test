#include "os.h"
#include "os_cfg_app.h"

CPU_STK 	OSCfg_IdleTaskStk[OS_CFG_IDLE_TASK_STK_SIZE]; 		/* 空闲任务栈 */

CPU_STK		*const 	OSCfg_IdleTaskStkBasePtr = (CPU_STK *) &OSCfg_IdleTaskStk[0];	/* 空闲任务栈的起始地址 */
CPU_STK		 const	OSCfg_IdleTaskStkSize	 = (CPU_STK_SIZE) OS_CFG_IDLE_TASK_STK_SIZE;	/* 空闲任务栈的大小 */

