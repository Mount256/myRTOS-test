#include "os.h"

/* 初始化时基列表 */
void OS_TickListInit (void)
{
	OS_TICK_SPOKE_IX	i;
	OS_TICK_SPOKE		*p_spoke;
	
	for (i = 0u; i < OSCfg_TickWheelSize; i++)
	{
		p_spoke 				= (OS_TICK_SPOKE *)&OSCfg_TickWheel[i];
		p_spoke->FirstPtr 		= (OS_TCB        *)0;
		p_spoke->NbrEntries 	= (OS_OBJ_QTY     )0u;
		p_spoke->NbrEntriesMax 	= (OS_OBJ_QTY     )0u;
	}
}

/* 往时基列表插入一个任务 TCB，根据延时时间的大小升序排列 */
void OS_TickListInsert (OS_TCB *p_tcb, OS_TICK time)
{
	OS_TICK_SPOKE_IX	spoke;
	OS_TICK_SPOKE		*p_spoke;
	OS_TCB				*p_tcb0;
	OS_TCB				*p_tcb1;
	
	/* TickCtrMatch 等于当前时基计数器的值加上任务要延时的时间 */
	p_tcb->TickCtrMatch = OSTickCtr + time;	
	
	/* TickRemain 表示任务还需要多少个 SysTick 周期，每来一个周期就会减一 */
	p_tcb->TickRemain   = time;		
	
	/* （哈希算法）求余得到的 spoke 作为时基列表 OSCfg_TickWheel[] 的索引 */
	spoke 	= (OS_TICK_SPOKE_IX)(p_tcb->TickCtrMatch % OSCfg_TickWheelSize);
	
	/* 获取该双向链表的根指针 */
	p_spoke = &OSCfg_TickWheel[spoke];
	
	/* 若双向链表为空 */
	if (p_spoke->NbrEntries == (OS_OBJ_QTY)0u)
	{
		p_tcb->TickNextPtr  = (OS_TCB   *)0;
		p_tcb->TickPrevPtr  = (OS_TCB   *)0;
		p_spoke->FirstPtr   =  p_tcb;
		p_spoke->NbrEntries = (OS_OBJ_QTY)1u;
	}
	else /* 若双向链表不为空 */
	{
		p_tcb1 = p_spoke->FirstPtr;		/* 先获取第一个节点 */	
		/* 开始遍历双向链表 */
		while (p_tcb1 != (OS_TCB *)0)
		{
			p_tcb1->TickRemain = p_tcb1->TickCtrMatch - OSTickCtr;	/* 计算被访问节点的剩余时间 */
			if (p_tcb->TickRemain > p_tcb1->TickRemain)	/* (1) 如果新节点的剩余时间大于被访问节点的剩余时间 */
			{
				if (p_tcb1->NextPtr != (OS_TCB *)0)		/* (1.1) 如果被访问节点不是最后一个节点 */
				{
					p_tcb1 = p_tcb1->NextPtr;			/* 访问下一个节点，继续查找 */
				}
				else		/* (1.2) 否则，意味着整个链表已经遍历完毕，新节点的剩余时间最大，在最后一个节点插入新节点 */
				{
					p_tcb->TickNextPtr  = (OS_TCB *)0;
					p_tcb->TickPrevPtr  =  p_tcb1;
					p_tcb1->TickNextPtr =  p_tcb;
					p_tcb1 = (OS_TCB *)0;		/* 插入操作完成后清零，下一次将跳出循环 */
				}
			}
			else /* (2) 否则，被访问节点的剩余时间小于等于新节点的剩余时间，则插入到该节点前面 */
			{
				if (p_tcb1->TickPrevPtr == (OS_TCB *)0) /* (2.1) 如果被访问节点是第一个节点 */
				{
					p_tcb->TickPrevPtr  = (OS_TCB *)0;
					p_tcb->TickNextPtr  = p_tcb1;
					p_tcb1->TickPrevPtr = p_tcb;
					p_spoke->FirstPtr   = p_tcb;
				}
				else		/* (2.2) 否则，是在两个节点之间插入新节点 */
				{
					p_tcb0              = p_tcb1->TickPrevPtr;
					p_tcb->TickPrevPtr  = p_tcb0;
					p_tcb->TickNextPtr  = p_tcb1;
					p_tcb0->TickNextPtr = p_tcb;
					p_tcb1->TickPrevPtr = p_tcb;
				}
				p_tcb1 = (OS_TCB *)0;		/* 插入操作完成后清零，下一次将跳出循环 */
			}
		}
		p_spoke->NbrEntries++;		/* 节点数加一 */
	}
	
	if (p_spoke->NbrEntriesMax < p_spoke->NbrEntries)	/* 刷新最大值 */
	{
		p_spoke->NbrEntriesMax = p_spoke->NbrEntries;
	}
	
	/* 任务 TCB 中的 TickSpokePtr 回指根节点 */
	p_tcb->TickSpokePtr = p_spoke;	
}

/* 往时基列表删除一个指定的任务 TCB */
void OS_TickListRemove (OS_TCB *p_tcb)
{
	OS_TICK_SPOKE		*p_spoke;
	OS_TCB				*p_tcb1;
	OS_TCB				*p_tcb2;
	
	/* 获取任务 TCB 所在链表的根指针 */
	p_spoke = p_tcb->TickSpokePtr;
	
	if (p_spoke != (OS_TICK_SPOKE *)0)		/* 若链表存在 */
	{
		if (p_tcb == p_spoke->FirstPtr)		/* (1) 如果被删除的节点是第一个节点 */
		{
			p_tcb1 = p_tcb->TickNextPtr;
			p_spoke->FirstPtr = p_tcb1;
			if (p_tcb1 != (OS_TCB *)0)	/* 若删除后链表非空，则新的头节点的前向指针域设为 0，否则不用操作 */
			{
				p_tcb1->TickPrevPtr = (OS_TCB *)0;
			}
		}
		else								/* (2) 如果被删除的节点不是第一个节点 */
		{
			p_tcb1 = p_tcb->TickPrevPtr;
			p_tcb2 = p_tcb->TickNextPtr;
			p_tcb1->TickNextPtr = p_tcb2;
			if (p_tcb2 != (OS_TCB *)0)	/* 若删除的不是最后一个节点，则新的尾节点的前向指针域设为 0，否则不用操作 */
			{
				p_tcb2->TickPrevPtr = p_tcb1;
			}
		}
		
		p_tcb->TickNextPtr 	= (OS_TCB *)0;
		p_tcb->TickPrevPtr 	= (OS_TCB *)0;
		p_tcb->TickSpokePtr = (OS_TICK_SPOKE *)0;
		p_tcb->TickRemain 	= (OS_TICK)0u;	/* 剩余时间清零 */
		p_tcb->TickCtrMatch = (OS_TICK)0u;
		
		p_spoke->NbrEntries--;		/* 节点数减一 */
	}
}

/* 更新时基计数器，扫描时基列表中的任务延时是否到期 */
void OS_TickListUpdate (void)
{
	OS_TICK_SPOKE_IX	spoke;
	OS_TICK_SPOKE		*p_spoke;
	OS_TCB				*p_tcb;
	OS_TCB				*p_tcb_next;
	CPU_BOOLEAN			done;
	
	CPU_SR_ALLOC();
	OS_CRITICAL_ENTER();
	
	/* 时基计数器加一 */
	OSTickCtr++;
	
	/* （哈希算法）求余得到的 spoke 作为时基列表 OSCfg_TickWheel[] 的索引 */
	spoke 	= (OS_TICK_SPOKE_IX)(OSTickCtr % OSCfg_TickWheelSize);
	
	/* 获取该双向链表的根指针 */
	p_spoke = &OSCfg_TickWheel[spoke];
	
	/* 获取该双向链表的第一个节点 */	
	p_tcb   = p_spoke->FirstPtr;	
	done    = DEF_FALSE;
	
	while (done == DEF_FALSE)
	{
		if (p_tcb != (OS_TCB *)0)	/* 若节点存在 */
		{
			p_tcb_next = p_tcb->TickNextPtr;
			//p_tcb->TickRemain = p_tcb->TickCtrMatch - OSTickCtr;	/* 计算节点的剩余时间 */
			
			if (OSTickCtr == p_tcb->TickCtrMatch)	/* 如果节点的延时时间已到 */
			{
				OS_TaskRdy (p_tcb);					/* 则让任务就绪 */
			}
			else	/* 否则，节点的延时时间未到 */
			{
				done = DEF_TRUE;	/* 则退出循环，因为链表节点按延时时间升序排列，该节点的延时时间未到，后面的节点肯定未到 */
			}
			p_tcb = p_tcb_next;
		}
		else
		{
			done = DEF_TRUE;
		}
	}
	
	OS_CRITICAL_EXIT();
}
