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
*                                            8位前导零查找表
*********************************************************************************************************
*/

#ifndef   CPU_CFG_LEAD_ZEROS_ASM_PRESENT
static  const  CPU_INT08U  CPU_CntLeadZerosTbl[256] = {                             /* Data vals :                      */
/*   0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F   */
    8u,  7u,  6u,  6u,  5u,  5u,  5u,  5u,  4u,  4u,  4u,  4u,  4u,  4u,  4u,  4u,  /*   0x00 to 0x0F                   */
    3u,  3u,  3u,  3u,  3u,  3u,  3u,  3u,  3u,  3u,  3u,  3u,  3u,  3u,  3u,  3u,  /*   0x10 to 0x1F                   */
    2u,  2u,  2u,  2u,  2u,  2u,  2u,  2u,  2u,  2u,  2u,  2u,  2u,  2u,  2u,  2u,  /*   0x20 to 0x2F                   */
    2u,  2u,  2u,  2u,  2u,  2u,  2u,  2u,  2u,  2u,  2u,  2u,  2u,  2u,  2u,  2u,  /*   0x30 to 0x3F                   */
    1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  /*   0x40 to 0x4F                   */
    1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  /*   0x50 to 0x5F                   */
    1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  /*   0x60 to 0x6F                   */
    1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  1u,  /*   0x70 to 0x7F                   */
    0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  /*   0x80 to 0x8F                   */
    0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  /*   0x90 to 0x9F                   */
    0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  /*   0xA0 to 0xAF                   */
    0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  /*   0xB0 to 0xBF                   */
    0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  /*   0xC0 to 0xCF                   */
    0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  /*   0xD0 to 0xDF                   */
    0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  /*   0xE0 to 0xEF                   */
    0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u,  0u   /*   0xF0 to 0xFF                   */
};
#endif

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

/*
*********************************************************************************************************
*                                 前导零函数（只实现了 32 位）
*********************************************************************************************************
*/
#ifndef CPU_CFG_LEAD_ZEROS_ASM_PRESENT
CPU_DATA CPU_CntLeadZeros (CPU_DATA val)
{
	CPU_DATA	nbr_lead_zeros;
	CPU_INT08U	ix;
	
#if (CPU_CFG_DATA_SIZE == CPU_WORD_SIZE_32)  
	if (val > 0x0000FFFFu)		/* 检查高 16 位，若不都是 0 */
	{
		if (val > 0x00FFFFFFu)	/* 检查位 31-24，若不都是 0 */
		{
			ix = (CPU_INT08U)(val >> 24u); 	/* 右移 24 位 */
			nbr_lead_zeros = (CPU_DATA)(CPU_LeadZerosTbl[ix] + 0u);
		}
		else					/* 若位 31-24 都是 0，检查位 23-16 */
		{
			ix = (CPU_INT08U)(val >> 16u); 	/* 右移 16 位 */
			nbr_lead_zeros = (CPU_DATA)(CPU_LeadZerosTbl[ix] + 8u);
		}
	}
	else						/* 检查低 16 位，若不都是 0 */
	{
		if (val > 0x000000FFu)	/* 检查位 15-8，若不都是 0 */
		{
			ix = (CPU_INT08U)(val >> 8u); 	/* 右移 8 位 */
			nbr_lead_zeros = (CPU_DATA)(CPU_LeadZerosTbl[ix] + 16u);
		}
		else					/* 若位 15-8 都是 0，检查位 7-0 */
		{
			ix = (CPU_INT08U)(val >> 0u); 	/* 右移 0 位 */
			nbr_lead_zeros = (CPU_DATA)(CPU_LeadZerosTbl[ix] + 24u);
		}
	}
#endif
	return nbr_lead_zeros;
}
#endif
