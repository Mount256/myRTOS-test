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
	
	/********������ʼ����********/
	OSTaskCreate( (OS_TCB 		*) 	&AppTaskStartTCB,  				/* ������ƿ� 		*/
				  (CPU_CHAR 	*)	"App Task Start",				/* �������� 			*/
				  (OS_TASK_PTR	 )	AppTaskStart,					/* ���������� 		*/
				  (void 		*)	0,								/* ������ں����β� 	*/
				  (OS_PRIO	 	 )	APP_TASK_START_PRIO,			/* ��������ȼ� 		*/
				  (CPU_STK		*)	&AppTaskStartStk[0],			/* ջ����ʼ��ַ 		*/
				  (CPU_STK_SIZE	 )  APP_TASK_START_STK_SIZE / 10, 	/* ����ջ������λ�� 	*/
				  (CPU_STK_SIZE  )	APP_TASK_START_STK_SIZE,		/* ����ջ��С 		*/
			      (OS_MSG_QTY  	 ) 	5u,				/* ���ÿ��Է��͵�����������Ϣ�� 	*/
                  (OS_TICK       ) 	0u,				/* ������֮��ѭ��ʱ��ʱ��Ƭ��ʱ���� 	*/					
                  (void       	*) 	0,				/* ָ���û��ṩ���ڴ�λ�õ�ָ�룬���� TCB ��չ */
                  (OS_OPT        )	(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), /* �����ض�ѡ�� */
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
	
    BSP_Init();     // �弶��ʼ��   
    CPU_Init();		// ��ʼ�� CPU �����ʱ��������ж�ʱ���������������

    cpu_clk_freq = BSP_CPU_ClkFreq();     					// ��ȡCPU�ڲ�ʱ��Ƶ��                      
    cnts = cpu_clk_freq / (CPU_INT32U) OSCfg_TickRate_Hz;   // �����û��趨��ʱ�ӽ���Ƶ�ʼ��� SysTick ��ʱ���ļ���ֵ      
    OS_CPU_SysTickInit (cnts);                      		// ���� SysTick ��ʼ�����������ö�ʱ������ֵ��������ʱ��             
     
    Mem_Init();                 	// ��ʼ���ڴ������������ڴ�غ��ڴ�ر�                               
#if OS_CFG_STAT_TASK_EN > 0u
    OSStatTaskCPUUsageInit (&err);  // ����û��Ӧ������ֻ�п�����������ʱ CPU �ģ��������                             
#endif

    CPU_IntDisMeasMaxCurReset();	// ��λ�����㣩��ǰ�����ж�ʱ��
	
	/********LED0 TASK CREATE*********/
	OSTaskCreate( (OS_TCB 		*) 	&AppTaskLed0TCB,  				/* ������ƿ� 		*/
				  (CPU_CHAR 	*)	"App Task LED0",				/* �������� 			*/
				  (OS_TASK_PTR	 )	AppTaskLED0,					/* ���������� 		*/
				  (void 		*)	0,								/* ������ں����β� 	*/
				  (OS_PRIO	 	 )	APP_TASK_LED0_PRIO,				/* ��������ȼ� 		*/
				  (CPU_STK		*)	&AppTaskLed0Stk[0],				/* ջ����ʼ��ַ 		*/
				  (CPU_STK_SIZE	 )  APP_TASK_LED0_STK_SIZE / 10, 	/* ����ջ������λ�� 	*/
				  (CPU_STK_SIZE  )	APP_TASK_LED0_STK_SIZE,		/* ����ջ��С 		*/
			      (OS_MSG_QTY  	 ) 	5u,				/* ���ÿ��Է��͵�����������Ϣ�� 	*/
                  (OS_TICK       ) 	0u,				/* ������֮��ѭ��ʱ��ʱ��Ƭ��ʱ���� 	*/					
                  (void       	*) 	0,				/* ָ���û��ṩ���ڴ�λ�õ�ָ�룬���� TCB ��չ */
                  (OS_OPT        )	(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), /* �����ض�ѡ�� */
                  (OS_ERR     	*)	&err
				);
	
	/********LED1 TASK CREATE*********/
	OSTaskCreate( (OS_TCB 		*) 	&AppTaskLed1TCB,  				/* ������ƿ� 		*/
				  (CPU_CHAR 	*)	"App Task LED1",				/* �������� 			*/
				  (OS_TASK_PTR	 )	AppTaskLED1,					/* ���������� 		*/
				  (void 		*)	0,								/* ������ں����β� 	*/
				  (OS_PRIO	 	 )	APP_TASK_LED1_PRIO,				/* ��������ȼ� 		*/
				  (CPU_STK		*)	&AppTaskLed1Stk[0],				/* ջ����ʼ��ַ 		*/
				  (CPU_STK_SIZE	 )  APP_TASK_LED1_STK_SIZE / 10, 	/* ����ջ������λ�� 	*/
				  (CPU_STK_SIZE  )	APP_TASK_LED1_STK_SIZE,		/* ����ջ��С 		*/
			      (OS_MSG_QTY  	 ) 	5u,				/* ���ÿ��Է��͵�����������Ϣ�� 	*/
                  (OS_TICK       ) 	0u,				/* ������֮��ѭ��ʱ��ʱ��Ƭ��ʱ���� 	*/					
                  (void       	*) 	0,				/* ָ���û��ṩ���ڴ�λ�õ�ָ�룬���� TCB ��չ */
                  (OS_OPT        )	(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), /* �����ض�ѡ�� */
                  (OS_ERR     	*)	&err
				);
				  
	/********BEEP TASK CREATE*********/
	OSTaskCreate( (OS_TCB 		*) 	&AppTaskBeepTCB,  				/* ������ƿ� 		*/
				  (CPU_CHAR 	*)	"App Task Beep",				/* �������� 			*/
				  (OS_TASK_PTR	 )	AppTaskBeep,					/* ���������� 		*/
				  (void 		*)	0,								/* ������ں����β� 	*/
				  (OS_PRIO	 	 )	APP_TASK_BEEP_PRIO,				/* ��������ȼ� 		*/
				  (CPU_STK		*)	&AppTaskBeepStk[0],				/* ջ����ʼ��ַ 		*/
				  (CPU_STK_SIZE	 )  APP_TASK_BEEP_STK_SIZE / 10, 	/* ����ջ������λ�� 	*/
				  (CPU_STK_SIZE  )	APP_TASK_BEEP_STK_SIZE,		/* ����ջ��С 		*/
			      (OS_MSG_QTY  	 ) 	5u,				/* ���ÿ��Է��͵�����������Ϣ�� 	*/
                  (OS_TICK       ) 	0u,				/* ������֮��ѭ��ʱ��ʱ��Ƭ��ʱ���� 	*/					
                  (void       	*) 	0,				/* ָ���û��ṩ���ڴ�λ�õ�ָ�룬���� TCB ��չ */
                  (OS_OPT        )	(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), /* �����ض�ѡ�� */
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

