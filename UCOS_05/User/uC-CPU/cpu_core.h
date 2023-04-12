#ifndef CPU_CORE_H
#define CPU_CORE_H

#include "cpu.h"
#include "cpu_def.h"

/**************************************ȫ�ֱ���***********************************************/

#ifdef CPU_CORE_MODULE
	#define	CPU_CORE_EXT
#else
	#define CPU_CORE_EXT 	extern
#endif
	
/**********************************����/�ر�***************************************************/
	
/* �Ƿ���ʱ����� */
#if ((CPU_CFG_TS_32_EN == DEF_ENABLED) || (CPU_CFG_TS_64_EN == DEF_ENABLED))
#define	CPU_CFG_TS_EN	DEF_ENABLED
#else
#define CPU_CFG_TS_EN	DEF_DISABLED
#endif

/* �Ƿ���ʱ�����ʱ���� */
#if ((CPU_CFG_TS_EN == DEF_ENABLED) || defined(CPU_CFG_INT_DIS_MEAS_EN))
#define	CPU_CFG_TS_TMR_EN	DEF_ENABLED
#else
#define CPU_CFG_TS_TMR_EN	DEF_DISABLED
#endif
	
/*************************************************************************************/
	
typedef CPU_INT32U	CPU_TS32;
typedef CPU_INT32U	CPU_TS_TMR_FREQ;
typedef	CPU_TS32	CPU_TS;
typedef	CPU_INT32U	CPU_TS_TMR;

/************************************ȫ�ֱ�����************************************************/

#if (CPU_CFG_TS_TMR_EN == DEF_ENABLED)
CPU_CORE_EXT	CPU_TS_TMR_FREQ		CPU_TS_TmrFreq_Hz;
#endif
	
/************************************����������*************************************************/

/* CPU ��ʼ������ */
void CPU_Init (void);

/* ʱ�����ʼ������ */
#if ((CPU_CFG_TS_EN == DEF_ENABLED) || (CPU_CFG_TS_TMR_EN == DEF_ENABLED))
static void CPU_TS_Init (void);
#endif

/* ʱ�����ʱ����ʼ������ */
#if (CPU_CFG_TS_TMR_EN == DEF_ENABLED)
void CPU_TS_TmrInit (void);
#endif

/* ��ȡ CPU �� HCLK ʱ�� */
CPU_INT32U BSP_CPU_ClkFreq (void);

/* ��� OS ϵͳʱ�� */
#if (CPU_CFG_TS_TMR_EN == DEF_ENABLED)
void CPU_TS_TmrFreqSet (CPU_TS_TMR_FREQ freq_hz);
#endif

/* ��� DWT �� CYCCNT */
#if (CPU_CFG_TS_TMR_EN == DEF_ENABLED)
CPU_TS_TMR CPU_TS_TmrRd (void);
#endif

#endif /* CPU_CORE_H */
