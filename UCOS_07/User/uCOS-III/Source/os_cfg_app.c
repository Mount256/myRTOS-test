#include "os.h"
#include "os_cfg_app.h"

CPU_STK 	OSCfg_IdleTaskStk[OS_CFG_IDLE_TASK_STK_SIZE]; 		/* ��������ջ */

CPU_STK		*const 	OSCfg_IdleTaskStkBasePtr = (CPU_STK *) &OSCfg_IdleTaskStk[0];	/* ��������ջ����ʼ��ַ */
CPU_STK		 const	OSCfg_IdleTaskStkSize	 = (CPU_STK_SIZE) OS_CFG_IDLE_TASK_STK_SIZE;	/* ��������ջ�Ĵ�С */

