#define CPU_CORE_MODULE

#include "cpu_core.h"

/*
*********************************************************************************************************
*                                             �Ĵ�������
*********************************************************************************************************
*/
/*
 ��Cortex-M�ں�������һ�������DWT(Data Watchpoint and Trace)����������һ��32λ�ļĴ�����CYCCNT��
 ����һ�����ϵ� ����������¼�����ں�ʱ�����еĸ�������CYCCNT���֮�󣬻���0���¿�ʼ���ϼ�����
 ʹ��CYCCNT�����Ĳ������裺
 1����ʹ��DWT���裬����������ں˵��ԼĴ���DEMCR��λ24���ƣ�д1ʹ��
 2��ʹ��CYCCNT�Ĵ���֮ǰ������0
 3��ʹ��CYCCNT�Ĵ����������DWT_CTRL(�����Ϻ궨��ΪDWT_CR)��λ0���ƣ�д1ʹ��
 */
#define  BSP_REG_DEM_CR                       (*(CPU_REG32 *)0xE000EDFC)
#define  BSP_REG_DWT_CR                       (*(CPU_REG32 *)0xE0001000)
#define  BSP_REG_DWT_CYCCNT                   (*(CPU_REG32 *)0xE0001004)
#define  BSP_REG_DBGMCU_CR                    (*(CPU_REG32 *)0xE0042004)

/*
*********************************************************************************************************
*                                            �Ĵ���λ����
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

/*******************************************************************************************************/

/* CPU ��ʼ������ */
void CPU_Init (void)
{
#if ((CPU_CFG_TS_EN == DEF_ENABLED) || (CPU_CFG_TS_TMR_EN == DEF_ENABLED))
	CPU_TS_Init();		/* ʱ�����ʼ������ */
#endif
}

/* ʱ�����ʼ������ */
#if ((CPU_CFG_TS_EN == DEF_ENABLED) || (CPU_CFG_TS_TMR_EN == DEF_ENABLED))
static void CPU_TS_Init (void)
{
#if (CPU_CFG_TS_TMR_EN == DEF_ENABLED)
	CPU_TS_TmrFreq_Hz = 0u;		/* CPU ϵͳʱ�� */
	CPU_TS_TmrInit();
#endif	
}
#endif

/* ʱ�����ʱ����ʼ������ */
#if (CPU_CFG_TS_TMR_EN == DEF_ENABLED)
void CPU_TS_TmrInit (void)
{
	CPU_INT32U fclk_freq;
	fclk_freq = BSP_CPU_ClkFreq();
	
	BSP_REG_DEM_CR |= BSP_BIT_DEM_CR_TRCENA;		/* ���� DWT ���� */
	BSP_REG_DWT_CYCCNT = (CPU_INT32U) 0u;			/* DWT CYCCNT �Ĵ����������� */
	BSP_REG_DBGMCU_CR |= BSP_BIT_DWT_CR_CYCCNTENA;	/* ���� DWT CYCCNT ������ */
	
	CPU_TS_TmrFreqSet ((CPU_TS_TMR_FREQ) fclk_freq);
}
#endif

/* ��ȡ CPU �� HCLK ʱ�ӣ���Ӧ����Ӳ���л�ȡ�ģ�����Ϊ���������棬ֱ���ֶ����� */
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

/* ��ʼ�� OS ϵͳʱ�� */
#if (CPU_CFG_TS_TMR_EN == DEF_ENABLED)
void CPU_TS_TmrFreqSet (CPU_TS_TMR_FREQ freq_hz)
{
	CPU_TS_TmrFreq_Hz = freq_hz;
}
#endif

/* ��� DWT �� CYCCNT */
#if (CPU_CFG_TS_TMR_EN == DEF_ENABLED)
CPU_TS_TMR CPU_TS_TmrRd (void)
{
	CPU_TS_TMR ts_tmr_cnts;
	ts_tmr_cnts = (CPU_TS_TMR)BSP_REG_DWT_CYCCNT;
	return ts_tmr_cnts;
}
#endif