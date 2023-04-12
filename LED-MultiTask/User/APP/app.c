#include <includes.h>

static OS_TCB 	AppTaskStartTCB;
static OS_TCB   AppTaskLed0TCB;
static OS_TCB   AppTaskLed1TCB;
static OS_TCB	AppTaskBeepTCB;

static CPU_STK  AppTaskStartStk[APP_TASK_START_STK_SIZE];
static CPU_STK  AppTaskLed0Stk [APP_TASK_LED0_STK_SIZE ];
static CPU_STK  AppTaskLed1Stk [APP_TASK_LED1_STK_SIZE ];
static CPU_STK	AppTaskBeepStk [APP_TASK_BEEP_STK_SIZE ];

static void AppTaskStart (void *arg);
static void AppTaskLED0  (void *arg);
static void AppTaskLED1  (void *arg);
static void AppTaskBeep  (void *arg);

/***********************************************************************************************************/

int main (void)
{
	OS_ERR err;
	
	BSP_Init ();
	OSInit (&err);
	
	/********创建起始任务********/
	OSTaskCreate( (OS_TCB 		*) 	&AppTaskStartTCB,  				/* 任务控制块 		*/
				  (CPU_CHAR 	*)	"App Task Start",				/* 任务名称 			*/
				  (OS_TASK_PTR	 )	AppTaskStart,					/* 任务函数名称 		*/
				  (void 		*)	0,								/* 任务入口函数形参 	*/
				  (OS_PRIO	 	 )	APP_TASK_START_PRIO,			/* 任务的优先级 		*/
				  (CPU_STK		*)	&AppTaskStartStk[0],			/* 栈的起始地址 		*/
				  (CPU_STK_SIZE	 )  APP_TASK_START_STK_SIZE / 10, 	/* 任务栈的限制位置 	*/
				  (CPU_STK_SIZE  )	APP_TASK_START_STK_SIZE,		/* 任务栈大小 		*/
			      (OS_MSG_QTY  	 ) 	5u,				/* 设置可以发送到任务的最大消息数 	*/
                  (OS_TICK       ) 	0u,				/* 在任务之间循环时的时间片的时间量 	*/					
                  (void       	*) 	0,				/* 指向用户提供的内存位置的指针，用作 TCB 扩展 */
                  (OS_OPT        )	(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), /* 任务特定选项 */
                  (OS_ERR     	*)	&err
				);
				  
	OSStart (&err);
}

/************************************************************************************************************/

static void AppTaskStart (void *arg)
{
	CPU_INT32U  cpu_clk_freq;
    CPU_INT32U  cnts;
    OS_ERR      err;
   (void) 		arg;
	
    BSP_Init();     // 板级初始化   
    CPU_Init();		// 初始化 CPU 组件（时间戳、关中断时间测量和主机名）

    cpu_clk_freq = BSP_CPU_ClkFreq();     					// 获取CPU内部时钟频率                      
    cnts = cpu_clk_freq / (CPU_INT32U) OSCfg_TickRate_Hz;   // 根据用户设定的时钟节拍频率计算 SysTick 定时器的计数值      
    OS_CPU_SysTickInit (cnts);                      		// 调用 SysTick 初始化函数，设置定时器计数值和启动定时器             
     
    Mem_Init();                 	// 初始化内存管理组件（堆内存池和内存池表）                               
#if OS_CFG_STAT_TASK_EN > 0u
    OSStatTaskCPUUsageInit (&err);  // 计算没有应用任务（只有空闲任务）运行时 CPU 的（最大）容量                             
#endif

    CPU_IntDisMeasMaxCurReset();	// 复位（清零）当前最大关中断时间
	
	/********LED0 TASK CREATE*********/
	OSTaskCreate( (OS_TCB 		*) 	&AppTaskLed0TCB,  				/* 任务控制块 		*/
				  (CPU_CHAR 	*)	"App Task LED0",				/* 任务名称 			*/
				  (OS_TASK_PTR	 )	AppTaskLED0,					/* 任务函数名称 		*/
				  (void 		*)	0,								/* 任务入口函数形参 	*/
				  (OS_PRIO	 	 )	APP_TASK_LED0_PRIO,				/* 任务的优先级 		*/
				  (CPU_STK		*)	&AppTaskLed0Stk[0],				/* 栈的起始地址 		*/
				  (CPU_STK_SIZE	 )  APP_TASK_LED0_STK_SIZE / 10, 	/* 任务栈的限制位置 	*/
				  (CPU_STK_SIZE  )	APP_TASK_LED0_STK_SIZE,		/* 任务栈大小 		*/
			      (OS_MSG_QTY  	 ) 	5u,				/* 设置可以发送到任务的最大消息数 	*/
                  (OS_TICK       ) 	0u,				/* 在任务之间循环时的时间片的时间量 	*/					
                  (void       	*) 	0,				/* 指向用户提供的内存位置的指针，用作 TCB 扩展 */
                  (OS_OPT        )	(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), /* 任务特定选项 */
                  (OS_ERR     	*)	&err
				);
	
	/********LED1 TASK CREATE*********/
	OSTaskCreate( (OS_TCB 		*) 	&AppTaskLed1TCB,  				/* 任务控制块 		*/
				  (CPU_CHAR 	*)	"App Task LED1",				/* 任务名称 			*/
				  (OS_TASK_PTR	 )	AppTaskLED1,					/* 任务函数名称 		*/
				  (void 		*)	0,								/* 任务入口函数形参 	*/
				  (OS_PRIO	 	 )	APP_TASK_LED1_PRIO,				/* 任务的优先级 		*/
				  (CPU_STK		*)	&AppTaskLed1Stk[0],				/* 栈的起始地址 		*/
				  (CPU_STK_SIZE	 )  APP_TASK_LED1_STK_SIZE / 10, 	/* 任务栈的限制位置 	*/
				  (CPU_STK_SIZE  )	APP_TASK_LED1_STK_SIZE,		/* 任务栈大小 		*/
			      (OS_MSG_QTY  	 ) 	5u,				/* 设置可以发送到任务的最大消息数 	*/
                  (OS_TICK       ) 	0u,				/* 在任务之间循环时的时间片的时间量 	*/					
                  (void       	*) 	0,				/* 指向用户提供的内存位置的指针，用作 TCB 扩展 */
                  (OS_OPT        )	(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), /* 任务特定选项 */
                  (OS_ERR     	*)	&err
				);
				  
	/********BEEP TASK CREATE*********/
	OSTaskCreate( (OS_TCB 		*) 	&AppTaskBeepTCB,  				/* 任务控制块 		*/
				  (CPU_CHAR 	*)	"App Task Beep",				/* 任务名称 			*/
				  (OS_TASK_PTR	 )	AppTaskBeep,					/* 任务函数名称 		*/
				  (void 		*)	0,								/* 任务入口函数形参 	*/
				  (OS_PRIO	 	 )	APP_TASK_BEEP_PRIO,				/* 任务的优先级 		*/
				  (CPU_STK		*)	&AppTaskBeepStk[0],				/* 栈的起始地址 		*/
				  (CPU_STK_SIZE	 )  APP_TASK_BEEP_STK_SIZE / 10, 	/* 任务栈的限制位置 	*/
				  (CPU_STK_SIZE  )	APP_TASK_BEEP_STK_SIZE,		/* 任务栈大小 		*/
			      (OS_MSG_QTY  	 ) 	5u,				/* 设置可以发送到任务的最大消息数 	*/
                  (OS_TICK       ) 	0u,				/* 在任务之间循环时的时间片的时间量 	*/					
                  (void       	*) 	0,				/* 指向用户提供的内存位置的指针，用作 TCB 扩展 */
                  (OS_OPT        )	(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), /* 任务特定选项 */
                  (OS_ERR     	*)	&err
				);
				  
	OSTaskDel (&AppTaskStartTCB, &err);
}

/************************************************************************************************************/

static void AppTaskLED0 (void *arg)
{
	OS_ERR err;
	(void) arg;
	
	while (1)
	{
		LED0_REVERSE;
		OSTimeDly (2000, OS_OPT_TIME_DLY, &err);
	}
}

static void AppTaskLED1 (void *arg)
{
	OS_ERR err;
	(void) arg;
	
	while (1)
	{
		LED1_REVERSE;
		OSTimeDly (1000, OS_OPT_TIME_DLY, &err);
	}
}

static void AppTaskBeep  (void *arg)
{
	OS_ERR err;
	(void) arg;
	
	while (1)
	{
		BEEP_REVERSE;
		OSTimeDly (500, OS_OPT_TIME_DLY, &err);
	}
}

