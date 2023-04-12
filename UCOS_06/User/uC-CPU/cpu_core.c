#define CPU_CORE_MODULE

#include "cpu_core.h"

/*
*********************************************************************************************************
*                                             寄存器定义
*********************************************************************************************************
*/
/*
 在Cortex-M内核里面有一个外设叫DWT(Data Watchpoint and Trace)，该外设有一个32位的寄存器叫CYCCNT，
 它是一个向上的 计数器，记录的是内核时钟运行的个数，当CYCCNT溢出之后，会清0重新开始向上计数。
 使能CYCCNT计数的操作步骤：
 1、先使能DWT外设，这个由另外内核调试寄存器DEMCR的位24控制，写1使能
 2、使能CYCCNT寄存器之前，先清0
 3、使能CYCCNT寄存器，这个由DWT_CTRL(代码上宏定义为DWT_CR)的位0控制，写1使能
 */
#define  BSP_REG_DEM_CR                       (*(CPU_REG32 *)0xE000EDFC)
#define  BSP_REG_DWT_CR                       (*(CPU_REG32 *)0xE0001000)
#define  BSP_REG_DWT_CYCCNT                   (*(CPU_REG32 *)0xE0001004)
#define  BSP_REG_DBGMCU_CR                    (*(CPU_REG32 *)0xE0042004)

/*
*********************************************************************************************************
*                                            寄存器位定义
*********************************************************************************************************
*/

#define  BSP_DBGMCU_CR_TRACE_IOEN_MASK                   0x10
#define  BSP_DBGMCU_CR_TRACE_MODE_ASYNC                  0x00
#define  BSP_DBGMCU_CR_TRACE_MODE_SYNC_01                0x40
#define  BSP_DBGMCU_CR_TRACE_MODE_SYNC_02                0x80
#define  BSP_DBGMCU_CR_TRACE_MODE_SYNC_04                0xC0
#define  BSP_DBGMCU_CR_TRACE_MODE_MASK                   0xC0

#define  BSP_BIT_DEM_CR_TRCENA                          (1<<24)

#define  BSP_BIT_DWT_CR_CYCCNTENA                       (1<<0)

/*
*********************************************************************************************************
*                                            时间戳的相关函数
*********************************************************************************************************
*/

/* CPU 初始化函数 */
void CPU_Init (void)
{
#if ((CPU_CFG_TS_EN == DEF_ENABLED) || (CPU_CFG_TS_TMR_EN == DEF_ENABLED))
	CPU_TS_Init();		/* 时间戳初始化函数 */
#endif
}

/* 时间戳初始化函数 */
#if ((CPU_CFG_TS_EN == DEF_ENABLED) || (CPU_CFG_TS_TMR_EN == DEF_ENABLED))
static void CPU_TS_Init (void)
{
#if (CPU_CFG_TS_TMR_EN == DEF_ENABLED)
	CPU_TS_TmrFreq_Hz = 0u;		/* CPU 系统时钟 */
	CPU_TS_TmrInit();
#endif	
}
#endif

/* 时间戳定时器初始化函数 */
#if (CPU_CFG_TS_TMR_EN == DEF_ENABLED)
void CPU_TS_TmrInit (void)
{
	CPU_INT32U fclk_freq;
	fclk_freq = BSP_CPU_ClkFreq();
	
	BSP_REG_DEM_CR |= BSP_BIT_DEM_CR_TRCENA;		/* 启用 DWT 外设 */
	BSP_REG_DWT_CYCCNT = (CPU_INT32U) 0u;			/* DWT CYCCNT 寄存器计数清零 */
	BSP_REG_DBGMCU_CR |= BSP_BIT_DWT_CR_CYCCNTENA;	/* 启用 DWT CYCCNT 计数器 */
	
	CPU_TS_TmrFreqSet ((CPU_TS_TMR_FREQ) fclk_freq);
}
#endif

/* 获取 CPU 的 HCLK 时钟，理应是在硬件中获取的，但是为了软件仿真，直接手动配置 */
CPU_INT32U BSP_CPU_ClkFreq (void)
{
#if 0
	RCC_ClocksTypeDef rcc_clocks;
	RCC_GetClocksFreq (&rcc_clocks);
	return ((CPU_INT32U)rcc_clocks.HCLK_Frequency);
#else
	CPU_INT32U CPU_HCLK;
	CPU_HCLK = 25000000;
	return CPU_HCLK;
#endif
}

/* 初始化 OS 系统时钟 */
#if (CPU_CFG_TS_TMR_EN == DEF_ENABLED)
void CPU_TS_TmrFreqSet (CPU_TS_TMR_FREQ freq_hz)
{
	CPU_TS_TmrFreq_Hz = freq_hz;
}
#endif

/* 获得 DWT 的 CYCCNT */
#if (CPU_CFG_TS_TMR_EN == DEF_ENABLED)
CPU_TS_TMR CPU_TS_TmrRd (void)
{
	CPU_TS_TMR ts_tmr_cnts;
	ts_tmr_cnts = (CPU_TS_TMR)BSP_REG_DWT_CYCCNT;
	return ts_tmr_cnts;
}
#endif

/*
*********************************************************************************************************
*                                            测量关中断时间的相关函数
*********************************************************************************************************
*/

/* 测量关中断时间的初始化 */
#ifdef CPU_CFG_INT_DIS_MEAS_EN
static void CPU_IntDisMeasInit (void)
{
	CPU_TS_TMR 	time_meas_tot_cnts;
	CPU_INT16U 	i;
	
	CPU_SR_ALLOC();
	
	CPU_IntDisMeasCtr 			= 0u;
	CPU_IntDisNestCtr 			= 0u;
	CPU_IntDisMeasStart_cnts 	= 0u;
	CPU_IntDisMeasStop_cnts 	= 0u;
	CPU_IntDisMeasMaxCur_cnts 	= 0u;
	CPU_IntDisMeasMax_cnts 		= 0u;
	CPU_IntDisMeasOvrhd_cnts 	= 0u;
	time_meas_tot_cnts			= 0u;
	
	CPU_INT_DIS();
	
	for ( i = 0u; i < CPU_CFG_INT_DIS_MEAS_OVRHD_NBR; i++ )
	{
		CPU_IntDisMeasMaxCur_cnts = 0u;
        CPU_IntDisMeasStart();                                  /* 执行多个连续的开始/停止时间测量 */
        CPU_IntDisMeasStop();
        time_meas_tot_cnts += CPU_IntDisMeasMaxCur_cnts;        /* 计算总的时间 */
    }
                                                                /* 得到平均值，就是每一次测量额外消耗的时间 */
    CPU_IntDisMeasOvrhd_cnts  = (time_meas_tot_cnts + (CPU_CFG_INT_DIS_MEAS_OVRHD_NBR / 2u))
                                                    /  CPU_CFG_INT_DIS_MEAS_OVRHD_NBR;
    CPU_IntDisMeasMaxCur_cnts =  0u;                            
    CPU_IntDisMeasMax_cnts    =  0u;
	
    CPU_INT_EN();
}
#endif

/* 开始测量关中断时间 */
#ifdef CPU_CFG_INT_DIS_MEAS_EN
void CPU_IntDisMeasStart (void)
{
	CPU_IntDisMeasCtr++;
	if (CPU_IntDisNestCtr == 0u)
	{
		CPU_IntDisMeasStart_cnts = CPU_TS_TmrRd();  /* 保存时间戳 */
	}
	CPU_IntDisNestCtr++;
}
#endif

/* 停止测量关中断时间 */
#ifdef CPU_CFG_INT_DIS_MEAS_EN
void CPU_IntDisMeasStop (void)
{
	CPU_TS_TMR  time_ints_disd_cnts;

    CPU_IntDisNestCtr--;
    if (CPU_IntDisNestCtr == 0u) 									/* 若嵌套层数为 0 */
	{                                  
        CPU_IntDisMeasStop_cnts = CPU_TS_TmrRd();                   /* 保存时间戳 */
                                                                    
        time_ints_disd_cnts     = CPU_IntDisMeasStop_cnts -
                                  CPU_IntDisMeasStart_cnts;			/* 得到关中断时间 */
        /* 更新最大关中断时间 */                                                            
        if (CPU_IntDisMeasMaxCur_cnts < time_ints_disd_cnts) 
		{
            CPU_IntDisMeasMaxCur_cnts = time_ints_disd_cnts;
        }
        if (CPU_IntDisMeasMax_cnts    < time_ints_disd_cnts) 
		{
            CPU_IntDisMeasMax_cnts    = time_ints_disd_cnts;
        }
    }
}
#endif

// ------>未实现完毕......
