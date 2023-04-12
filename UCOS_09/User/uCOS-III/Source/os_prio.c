#include "os.h"

CPU_DATA OSPrioTbl[OS_PRIO_TBL_SIZE];  /* �������ȼ��� */

/* ��ʼ�����ȼ��� */
void OS_PrioInit (void)
{
	CPU_DATA	i;
	
	/* ȫ����ʼ��Ϊ 0 */
	for (i = 0u; i < OS_PRIO_TBL_SIZE; i++)
	{
		OSPrioTbl[i] = (CPU_DATA)0;
	}
}

/* �����ȼ������Ӧλ����λ */
void OS_PrioInsert (OS_PRIO prio)
{
	CPU_DATA	bit;
	CPU_DATA	bit_nbr;
	OS_PRIO		ix;
	
	/* ��ģ����ȡ���ȼ���������±� */
	ix 		= prio / DEF_INT_CPU_NBR_BITS;
	
	/* ���࣬�����ȼ������� DEF_INT_CPU_NBR_BITS �� */
	bit_nbr = (CPU_DATA)prio & (DEF_INT_CPU_NBR_BITS - 1u);		/* �ȼ��� prio % DEF_INT_CPU_NBR_BITS */
																/* X % (2^N) = X & (2^N-1) */
	bit 	= 1u;
	
	/* ��ȡ���ȼ������ȼ����ж�Ӧ��λ��λ�� */
	bit   <<= (DEF_INT_CPU_NBR_BITS - 1u) - bit_nbr;
	
	/* �����ȼ������ȼ����ж�Ӧ��λ�� 1 */
	OSPrioTbl[ix] |= bit;
}

/* ������ȼ������Ӧλ�� */
void OS_PrioRemove (OS_PRIO prio)
{
	CPU_DATA	bit;
	CPU_DATA	bit_nbr;
	OS_PRIO		ix;
	
	/* ��ģ����ȡ���ȼ���������±� */
	ix 		= prio / DEF_INT_CPU_NBR_BITS;
	
	/* ���࣬�����ȼ������� DEF_INT_CPU_NBR_BITS �� */
	bit_nbr = (CPU_DATA)prio & (DEF_INT_CPU_NBR_BITS - 1u);		/* �ȼ��� prio % DEF_INT_CPU_NBR_BITS */
																/* X % (2^N) = X & (2^N-1) */
	bit 	= 1u;
	
	/* ��ȡ���ȼ������ȼ����ж�Ӧ��λ��λ�� */
	bit   <<= (DEF_INT_CPU_NBR_BITS - 1u) - bit_nbr;
	
	/* �����ȼ������ȼ����ж�Ӧ��λ�� 0 */
	OSPrioTbl[ix] &= ~bit;
}

/* ��ȡ������ȼ� */
OS_PRIO OS_PrioGetHighest (void)
{
	CPU_DATA	*p_tbl;
	OS_PRIO		prio;
	
	prio = (OS_PRIO)0;
	p_tbl = &OSPrioTbl[0];		/* ���ȼ����׵�ַ */
	
	while (*p_tbl == (CPU_DATA)0)	/* �ҵ���Ϊ 0 ������Ԫ�� */
	{
		prio += DEF_INT_CPU_NBR_BITS;
		p_tbl++;
	}
	
	/* �ҵ����ȼ�������λ����ߵ����ȼ� */
	prio += (OS_PRIO)CPU_CntLeadZeros(*p_tbl);
	return prio;
}
