#include "os.h"

CPU_DATA OSPrioTbl[OS_PRIO_TBL_SIZE];  /* 定义优先级表 */

/* 初始化优先级表 */
void OS_PrioInit (void)
{
	CPU_DATA	i;
	
	/* 全部初始化为 0 */
	for (i = 0u; i < OS_PRIO_TBL_SIZE; i++)
	{
		OSPrioTbl[i] = (CPU_DATA)0;
	}
}

/* 在优先级表的相应位置置位 */
void OS_PrioInsert (OS_PRIO prio)
{
	CPU_DATA	bit;
	CPU_DATA	bit_nbr;
	OS_PRIO		ix;
	
	/* 求模，获取优先级表数组的下标 */
	ix 		= prio / DEF_INT_CPU_NBR_BITS;
	
	/* 求余，将优先级限制在 DEF_INT_CPU_NBR_BITS 内 */
	bit_nbr = (CPU_DATA)prio & (DEF_INT_CPU_NBR_BITS - 1u);		/* 等价于 prio % DEF_INT_CPU_NBR_BITS */
																/* X % (2^N) = X & (2^N-1) */
	bit 	= 1u;
	
	/* 获取优先级在优先级表中对应的位的位置 */
	bit   <<= (DEF_INT_CPU_NBR_BITS - 1u) - bit_nbr;
	
	/* 将优先级在优先级表中对应的位置 1 */
	OSPrioTbl[ix] |= bit;
}

/* 清除优先级表的相应位置 */
void OS_PrioRemove (OS_PRIO prio)
{
	CPU_DATA	bit;
	CPU_DATA	bit_nbr;
	OS_PRIO		ix;
	
	/* 求模，获取优先级表数组的下标 */
	ix 		= prio / DEF_INT_CPU_NBR_BITS;
	
	/* 求余，将优先级限制在 DEF_INT_CPU_NBR_BITS 内 */
	bit_nbr = (CPU_DATA)prio & (DEF_INT_CPU_NBR_BITS - 1u);		/* 等价于 prio % DEF_INT_CPU_NBR_BITS */
																/* X % (2^N) = X & (2^N-1) */
	bit 	= 1u;
	
	/* 获取优先级在优先级表中对应的位的位置 */
	bit   <<= (DEF_INT_CPU_NBR_BITS - 1u) - bit_nbr;
	
	/* 将优先级在优先级表中对应的位清 0 */
	OSPrioTbl[ix] &= ~bit;
}

/* 获取最高优先级 */
OS_PRIO OS_PrioGetHighest (void)
{
	CPU_DATA	*p_tbl;
	OS_PRIO		prio;
	
	prio = (OS_PRIO)0;
	p_tbl = &OSPrioTbl[0];		/* 优先级表首地址 */
	
	while (*p_tbl == (CPU_DATA)0)	/* 找到不为 0 的数组元素 */
	{
		prio += DEF_INT_CPU_NBR_BITS;
		p_tbl++;
	}
	
	/* 找到优先级表中置位的最高的优先级 */
	prio += (OS_PRIO)CPU_CntLeadZeros(*p_tbl);
	return prio;
}
