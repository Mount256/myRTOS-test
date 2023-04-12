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

/*
*********************************************************************************************************
*                                            8λǰ������ұ�
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
*                                            ʱ�������غ���
*********************************************************************************************************
*/

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

/* ��ȡ CPU �� HCLK ʱ�ӣ���Ӧ����Ӳ���л�ȡ�ģ�����Ϊ��������棬ֱ���ֶ����� */
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

/*
*********************************************************************************************************
*                                            �������ж�ʱ�����غ���
*********************************************************************************************************
*/

/* �������ж�ʱ��ĳ�ʼ�� */
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
        CPU_IntDisMeasStart();                                  /* ִ�ж�������Ŀ�ʼ/ֹͣʱ����� */
        CPU_IntDisMeasStop();
        time_meas_tot_cnts += CPU_IntDisMeasMaxCur_cnts;        /* �����ܵ�ʱ�� */
    }
                                                                /* �õ�ƽ��ֵ������ÿһ�β����������ĵ�ʱ�� */
    CPU_IntDisMeasOvrhd_cnts  = (time_meas_tot_cnts + (CPU_CFG_INT_DIS_MEAS_OVRHD_NBR / 2u))
                                                    /  CPU_CFG_INT_DIS_MEAS_OVRHD_NBR;
    CPU_IntDisMeasMaxCur_cnts =  0u;                            
    CPU_IntDisMeasMax_cnts    =  0u;
	
    CPU_INT_EN();
}
#endif

/* ��ʼ�������ж�ʱ�� */
#ifdef CPU_CFG_INT_DIS_MEAS_EN
void CPU_IntDisMeasStart (void)
{
	CPU_IntDisMeasCtr++;
	if (CPU_IntDisNestCtr == 0u)
	{
		CPU_IntDisMeasStart_cnts = CPU_TS_TmrRd();  /* ����ʱ��� */
	}
	CPU_IntDisNestCtr++;
}
#endif

/* ֹͣ�������ж�ʱ�� */
#ifdef CPU_CFG_INT_DIS_MEAS_EN
void CPU_IntDisMeasStop (void)
{
	CPU_TS_TMR  time_ints_disd_cnts;

    CPU_IntDisNestCtr--;
    if (CPU_IntDisNestCtr == 0u) 									/* ��Ƕ�ײ���Ϊ 0 */
	{                                  
        CPU_IntDisMeasStop_cnts = CPU_TS_TmrRd();                   /* ����ʱ��� */
                                                                    
        time_ints_disd_cnts     = CPU_IntDisMeasStop_cnts -
                                  CPU_IntDisMeasStart_cnts;			/* �õ����ж�ʱ�� */
        /* ���������ж�ʱ�� */                                                            
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

// ------>δʵ�����......

/*
*********************************************************************************************************
*                                 ǰ���㺯����ֻʵ���� 32 λ��
*********************************************************************************************************
*/
#ifndef CPU_CFG_LEAD_ZEROS_ASM_PRESENT
CPU_DATA CPU_CntLeadZeros (CPU_DATA val)
{
	CPU_DATA	nbr_lead_zeros;
	CPU_INT08U	ix;
	
#if (CPU_CFG_DATA_SIZE == CPU_WORD_SIZE_32)  
	if (val > 0x0000FFFFu)		/* ���� 16 λ���������� 0 */
	{
		if (val > 0x00FFFFFFu)	/* ���λ 31-24���������� 0 */
		{
			ix = (CPU_INT08U)(val >> 24u); 	/* ���� 24 λ */
			nbr_lead_zeros = (CPU_DATA)(CPU_LeadZerosTbl[ix] + 0u);
		}
		else					/* ��λ 31-24 ���� 0�����λ 23-16 */
		{
			ix = (CPU_INT08U)(val >> 16u); 	/* ���� 16 λ */
			nbr_lead_zeros = (CPU_DATA)(CPU_LeadZerosTbl[ix] + 8u);
		}
	}
	else						/* ���� 16 λ���������� 0 */
	{
		if (val > 0x000000FFu)	/* ���λ 15-8���������� 0 */
		{
			ix = (CPU_INT08U)(val >> 8u); 	/* ���� 8 λ */
			nbr_lead_zeros = (CPU_DATA)(CPU_LeadZerosTbl[ix] + 16u);
		}
		else					/* ��λ 15-8 ���� 0�����λ 7-0 */
		{
			ix = (CPU_INT08U)(val >> 0u); 	/* ���� 0 λ */
			nbr_lead_zeros = (CPU_DATA)(CPU_LeadZerosTbl[ix] + 24u);
		}
	}
#endif
	return nbr_lead_zeros;
}
#endif
