#ifndef CPU_CORE_H
#define CPU_CORE_H

#include "cpu.h"
#include "lib_def.h"
#include "cpu_def.h"
#include "cpu_cfg.h"

/**************************************全局变量***********************************************/

#ifdef CPU_CORE_MODULE
	#define	CPU_CORE_EXT
#else
	#define CPU_CORE_EXT 	extern
#endif
	
/**********************************开启/关闭***************************************************/
	
/* 是否开启时间戳？ */
#if ((CPU_CFG_TS_32_EN == DEF_ENABLED) || (CPU_CFG_TS_64_EN == DEF_ENABLED))
#define	CPU_CFG_TS_EN	DEF_ENABLED
#else
#define CPU_CFG_TS_EN	DEF_DISABLED
#endif

/* 是否开启时间戳定时器？ */
#if ((CPU_CFG_TS_EN == DEF_ENABLED) || (defined(CPU_CFG_INT_DIS_MEAS_EN)) )
#define	CPU_CFG_TS_TMR_EN	DEF_ENABLED
#else
#define CPU_CFG_TS_TMR_EN	DEF_DISABLED
#endif
	
/*************************************************************************************/
	
typedef CPU_INT32U	CPU_TS32;
typedef CPU_INT32U	CPU_TS_TMR_FREQ;
typedef	CPU_TS32	CPU_TS;
typedef	CPU_INT32U	CPU_TS_TMR;

/************************************全局变量区************************************************/

#if (CPU_CFG_TS_TMR_EN == DEF_ENABLED)
CPU_CORE_EXT	CPU_TS_TMR_FREQ		CPU_TS_TmrFreq_Hz;
#endif

#if (CPU_CFG_INT_DIS_MEAS_EN == DEF_ENABLED)
CPU_CORE_EXT 	CPU_INT16U 			CPU_IntDisMeasCtr;
CPU_CORE_EXT	CPU_INT16U			CPU_IntDisNestCtr;

CPU_CORE_EXT	CPU_TS_TMR			CPU_IntDisMeasStart_cnts;
CPU_CORE_EXT	CPU_TS_TMR			CPU_IntDisMeasStop_cnts;
CPU_CORE_EXT	CPU_TS_TMR			CPU_IntDisMeasMaxCur_cnts;
CPU_CORE_EXT	CPU_TS_TMR			CPU_IntDisMeasMax_cnts;
CPU_CORE_EXT	CPU_TS_TMR			CPU_IntDisMeasOvrhd_cnts;
#endif
	
/************************************函数声明区*************************************************/

/*----------------------时间戳的相关函数---------------------------*/
/* CPU 初始化函数 */
void CPU_Init (void);

/* 时间戳初始化函数 */
#if ((CPU_CFG_TS_EN == DEF_ENABLED) || (defined(CPU_CFG_INT_DIS_MEAS_EN)) )
static void CPU_TS_Init (void);
#endif

/* 时间戳定时器初始化函数 */
#if (CPU_CFG_TS_TMR_EN == DEF_ENABLED)
void CPU_TS_TmrInit (void);
#endif

/* 获取 CPU 的 HCLK 时钟 */
CPU_INT32U BSP_CPU_ClkFreq (void);

/* 获得 OS 系统时钟 */
#if (CPU_CFG_TS_TMR_EN == DEF_ENABLED)
void CPU_TS_TmrFreqSet (CPU_TS_TMR_FREQ freq_hz);
#endif

/* 获得 DWT 的 CYCCNT */
#if (CPU_CFG_TS_TMR_EN == DEF_ENABLED)
CPU_TS_TMR CPU_TS_TmrRd (void);
#endif

/*----------------------测量关中断时间的相关函数---------------------------*/

/* 测量关中断时间的初始化 */
#ifdef CPU_CFG_INT_DIS_MEAS_EN
static void CPU_IntDisMeasInit (void);
#endif

/* 开始测量关中断时间 */
#ifdef CPU_CFG_INT_DIS_MEAS_EN
void CPU_IntDisMeasStart (void);
#endif

/* 停止测量关中断时间 */
#ifdef CPU_CFG_INT_DIS_MEAS_EN
void CPU_IntDisMeasStop (void);
#endif

// ------>未实现完毕......

#endif /* CPU_CORE_H */
