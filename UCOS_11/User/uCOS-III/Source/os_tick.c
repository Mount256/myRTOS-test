#include "os.h"

/* ��ʼ��ʱ���б� */
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

/* ��ʱ���б����һ������ TCB��������ʱʱ��Ĵ�С�������� */
void OS_TickListInsert (OS_TCB *p_tcb, OS_TICK time)
{
	OS_TICK_SPOKE_IX	spoke;
	OS_TICK_SPOKE		*p_spoke;
	OS_TCB				*p_tcb0;
	OS_TCB				*p_tcb1;
	
	/* TickCtrMatch ���ڵ�ǰʱ����������ֵ��������Ҫ��ʱ��ʱ�� */
	p_tcb->TickCtrMatch = OSTickCtr + time;	
	
	/* TickRemain ��ʾ������Ҫ���ٸ� SysTick ���ڣ�ÿ��һ�����ھͻ��һ */
	p_tcb->TickRemain   = time;		
	
	/* ����ϣ�㷨������õ��� spoke ��Ϊʱ���б� OSCfg_TickWheel[] ������ */
	spoke 	= (OS_TICK_SPOKE_IX)(p_tcb->TickCtrMatch % OSCfg_TickWheelSize);
	
	/* ��ȡ��˫������ĸ�ָ�� */
	p_spoke = &OSCfg_TickWheel[spoke];
	
	/* ��˫������Ϊ�� */
	if (p_spoke->NbrEntries == (OS_OBJ_QTY)0u)
	{
		p_tcb->TickNextPtr  = (OS_TCB   *)0;
		p_tcb->TickPrevPtr  = (OS_TCB   *)0;
		p_spoke->FirstPtr   =  p_tcb;
		p_spoke->NbrEntries = (OS_OBJ_QTY)1u;
	}
	else /* ��˫������Ϊ�� */
	{
		p_tcb1 = p_spoke->FirstPtr;		/* �Ȼ�ȡ��һ���ڵ� */	
		/* ��ʼ����˫������ */
		while (p_tcb1 != (OS_TCB *)0)
		{
			p_tcb1->TickRemain = p_tcb1->TickCtrMatch - OSTickCtr;	/* ���㱻���ʽڵ��ʣ��ʱ�� */
			if (p_tcb->TickRemain > p_tcb1->TickRemain)	/* (1) ����½ڵ��ʣ��ʱ����ڱ����ʽڵ��ʣ��ʱ�� */
			{
				if (p_tcb1->NextPtr != (OS_TCB *)0)		/* (1.1) ��������ʽڵ㲻�����һ���ڵ� */
				{
					p_tcb1 = p_tcb1->NextPtr;			/* ������һ���ڵ㣬�������� */
				}
				else		/* (1.2) ������ζ�����������Ѿ�������ϣ��½ڵ��ʣ��ʱ����������һ���ڵ�����½ڵ� */
				{
					p_tcb->TickNextPtr  = (OS_TCB *)0;
					p_tcb->TickPrevPtr  =  p_tcb1;
					p_tcb1->TickNextPtr =  p_tcb;
					p_tcb1 = (OS_TCB *)0;		/* ���������ɺ����㣬��һ�ν�����ѭ�� */
				}
			}
			else /* (2) ���򣬱����ʽڵ��ʣ��ʱ��С�ڵ����½ڵ��ʣ��ʱ�䣬����뵽�ýڵ�ǰ�� */
			{
				if (p_tcb1->TickPrevPtr == (OS_TCB *)0) /* (2.1) ��������ʽڵ��ǵ�һ���ڵ� */
				{
					p_tcb->TickPrevPtr  = (OS_TCB *)0;
					p_tcb->TickNextPtr  = p_tcb1;
					p_tcb1->TickPrevPtr = p_tcb;
					p_spoke->FirstPtr   = p_tcb;
				}
				else		/* (2.2) �������������ڵ�֮������½ڵ� */
				{
					p_tcb0              = p_tcb1->TickPrevPtr;
					p_tcb->TickPrevPtr  = p_tcb0;
					p_tcb->TickNextPtr  = p_tcb1;
					p_tcb0->TickNextPtr = p_tcb;
					p_tcb1->TickPrevPtr = p_tcb;
				}
				p_tcb1 = (OS_TCB *)0;		/* ���������ɺ����㣬��һ�ν�����ѭ�� */
			}
		}
		p_spoke->NbrEntries++;		/* �ڵ�����һ */
	}
	
	if (p_spoke->NbrEntriesMax < p_spoke->NbrEntries)	/* ˢ�����ֵ */
	{
		p_spoke->NbrEntriesMax = p_spoke->NbrEntries;
	}
	
	/* ���� TCB �е� TickSpokePtr ��ָ���ڵ� */
	p_tcb->TickSpokePtr = p_spoke;	
}

/* ��ʱ���б�ɾ��һ��ָ�������� TCB */
void OS_TickListRemove (OS_TCB *p_tcb)
{
	OS_TICK_SPOKE		*p_spoke;
	OS_TCB				*p_tcb1;
	OS_TCB				*p_tcb2;
	
	/* ��ȡ���� TCB ��������ĸ�ָ�� */
	p_spoke = p_tcb->TickSpokePtr;
	
	if (p_spoke != (OS_TICK_SPOKE *)0)		/* ��������� */
	{
		if (p_tcb == p_spoke->FirstPtr)		/* (1) �����ɾ���Ľڵ��ǵ�һ���ڵ� */
		{
			p_tcb1 = p_tcb->TickNextPtr;
			p_spoke->FirstPtr = p_tcb1;
			if (p_tcb1 != (OS_TCB *)0)	/* ��ɾ��������ǿգ����µ�ͷ�ڵ��ǰ��ָ������Ϊ 0�������ò��� */
			{
				p_tcb1->TickPrevPtr = (OS_TCB *)0;
			}
		}
		else								/* (2) �����ɾ���Ľڵ㲻�ǵ�һ���ڵ� */
		{
			p_tcb1 = p_tcb->TickPrevPtr;
			p_tcb2 = p_tcb->TickNextPtr;
			p_tcb1->TickNextPtr = p_tcb2;
			if (p_tcb2 != (OS_TCB *)0)	/* ��ɾ���Ĳ������һ���ڵ㣬���µ�β�ڵ��ǰ��ָ������Ϊ 0�������ò��� */
			{
				p_tcb2->TickPrevPtr = p_tcb1;
			}
		}
		
		p_tcb->TickNextPtr 	= (OS_TCB *)0;
		p_tcb->TickPrevPtr 	= (OS_TCB *)0;
		p_tcb->TickSpokePtr = (OS_TICK_SPOKE *)0;
		p_tcb->TickRemain 	= (OS_TICK)0u;	/* ʣ��ʱ������ */
		p_tcb->TickCtrMatch = (OS_TICK)0u;
		
		p_spoke->NbrEntries--;		/* �ڵ�����һ */
	}
}

/* ����ʱ����������ɨ��ʱ���б��е�������ʱ�Ƿ��� */
void OS_TickListUpdate (void)
{
	OS_TICK_SPOKE_IX	spoke;
	OS_TICK_SPOKE		*p_spoke;
	OS_TCB				*p_tcb;
	OS_TCB				*p_tcb_next;
	CPU_BOOLEAN			done;
	
	CPU_SR_ALLOC();
	OS_CRITICAL_ENTER();
	
	/* ʱ����������һ */
	OSTickCtr++;
	
	/* ����ϣ�㷨������õ��� spoke ��Ϊʱ���б� OSCfg_TickWheel[] ������ */
	spoke 	= (OS_TICK_SPOKE_IX)(OSTickCtr % OSCfg_TickWheelSize);
	
	/* ��ȡ��˫������ĸ�ָ�� */
	p_spoke = &OSCfg_TickWheel[spoke];
	
	/* ��ȡ��˫������ĵ�һ���ڵ� */	
	p_tcb   = p_spoke->FirstPtr;	
	done    = DEF_FALSE;
	
	while (done == DEF_FALSE)
	{
		if (p_tcb != (OS_TCB *)0)	/* ���ڵ���� */
		{
			p_tcb_next = p_tcb->TickNextPtr;
			//p_tcb->TickRemain = p_tcb->TickCtrMatch - OSTickCtr;	/* ����ڵ��ʣ��ʱ�� */
			
			if (OSTickCtr == p_tcb->TickCtrMatch)	/* ����ڵ����ʱʱ���ѵ� */
			{
				OS_TaskRdy (p_tcb);					/* ����������� */
			}
			else	/* ���򣬽ڵ����ʱʱ��δ�� */
			{
				done = DEF_TRUE;	/* ���˳�ѭ������Ϊ����ڵ㰴��ʱʱ���������У��ýڵ����ʱʱ��δ��������Ľڵ�϶�δ�� */
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
