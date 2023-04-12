参考内容：《[野火]uCOS-III内核实现与应用开发实战指南——基于STM32》第 13 章。

[toc]

## 1 数据类型定义和宏定义
### 1.1 时基列表相关宏定义和全局变量（os_cfg_app.h/c & os.h）

在 os\_cfg\_app.h 中，宏定义时基列表的大小，其推荐值为<code>任务数/4</code>，推荐使用质数，不推荐使用偶数。如果算出来的大小是偶数，则需加 1 变成质数。

```c
/* 时基列表大小 */
#define OS_CFG_TICK_WHEEL_SIZE			17u
```

接下来在 os\_cfg\_app.c 中，定义两个全局变量，分别是时基列表（数组）和时基列表的大小。注意需要在 os.h 中声明。

```c
OS_TICK_SPOKE		OSCfg_TickWheel[OS_CFG_TICK_WHEEL_SIZE];		/* 时基列表 */
OS_OBJ_QTY	const	OSCfg_TickWheelSize = (OS_OBJ_QTY)OS_CFG_TICK_WHEEL_SIZE;	/* 时基列表大小 */
```

在 os.h 中定义 32 位的 Tick 计数器，即 SysTick 周期计数器，记录系统启动到现在或者从上一次复位到现在经过了多少个 SysTick 周期。每发起一次 SysTick 中断，该变量就会加一。

```c
OS_EXT	OS_TICK			OSTickCtr;		/* SysTick 周期计数器 */
```

### 1.2 时基列表定义（os.h）

在 os.h 中定义时基列表结构体，其组成为：
- FirstPtr：用于指向 TCB 双向链表的第一个节点。时基列表的每个数组元素都记录着一条 TCB 单向链表的信息，被插入该条链表的 TCB 会按照延时时间做升序排列。
- NbrEntries：记录 TCB 链表有多少个节点。
- NbrEntriesMax：记录 TCB 链表最多的时候有多少个节点，在节点增加时会刷新，在删除节点时不刷新。

```c
typedef struct os_tick_spoke 	OS_TICK_SPOKE;					/* 时基列表重命名为大写字母格式 */

struct os_tick_spoke{
	OS_TCB		*FirstPtr;			/* TCB 单向链表的头节点 */
	OS_OBJ_QTY	NbrEntries;			/* 单向链表有多少个节点 */
	OS_OBJ_QTY	NbrEntriesMax;		/* 单向链表最多时有多少个节点，该值会被刷新 */
};
```

### 1.3 修改 TCB 定义（os.h）

在 TCB 中新增加了 5 个成员，分别是：
- TickNextPtr：双向链表中指向下一个 TCB 节点。
- TickPrevPtr：双向链表中指向上一个 TCB 节点。
- TickSpokePtr：指向时基列表数组的指针，即用来指示该任务 TCB 属于哪条双向链表。
- TickCtrMatch：该值表示当前时基计数器的值加上任务要延时的周期。
- TickRemain：表示任务还需要延时多少个 SysTick 周期。该变量替代了 TaskDelayTicks 的功能，所以可以去掉 TaskDelayTicks。

```c
struct os_tcb{
	CPU_STK			*StkPtr;
	CPU_STK_SIZE	StkSize;
	
	//OS_TICK			TaskDelayTicks;		/* 任务延时周期个数 */
	
	OS_PRIO			Prio;				/* 任务优先级 */
	
	OS_TCB			*NextPtr;			/* 就绪列表双向链表的下一个指针 */
	OS_TCB			*PrevPtr;			/* 就绪列表双向链表的前一个指针 */
	
	OS_TCB			*TickNextPtr;		/* 指向时基列表双向链表的下一个 TCB 节点 */
	OS_TCB			*TickPrevPtr;		/* 指向时基列表双向链表的上一个 TCB 节点 */
	OS_TICK_SPOKE	*TickSpokePtr;		/* 用于回指到链表根部 */
	
	OS_TICK			TickCtrMatch;		/* 该值等于时基计数器 OSTickCtr 的值加上 TickRemain 的值 */
	OS_TICK			TickRemain;			/* 设置任务还需要等待多少个时钟周期 */
};
```

如下图所示，这就是时基列表的结构。因此，目前实现的 uCOS 一共有两个列表：就绪列表和时基列表，它们均根据自己的规则建立一个 TCB 双向链表，列表中的数组会存放双向链表的信息。就绪列表的数组下标表示优先级，时基列表的数组下标表示时间周期。

【图片】

## 2 时基列表的相关函数
### 2.1 初始化时基列表 OS_TickListInit()（os_tick.c）

该函数用于初始化时基列表数组，将全部成员清零。

```c
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
```

需要在 OSInit() 中调用此函数以完成初始化工作。初始化后的时基列表如下所示：

【图片】

### 2.2 往时基列表插入任务控制块 OS_TickListInsert()（os_tick.c）

该函数的作用是将任务 TCB 插入到时基列表中，即对应的 TCB 双向链表，注意这是一个升序排序的双向链表，按照延时时间的长度升序排序。该函数完成的工作有：

- 获得该任务要延时的时间 time 后，在 TCB 中记录延时到期时的时间，即将当前时基计数器的值加上任务要延时的时间记录在 TickCtrMatch。
- 在 TCB 中记录任务要延时的时间。TickRemain 的功能可等价为 TaskDelayTicks 的功能。

我们先停一下，举个例子来理解上面的原理。比如现在 OSTickCtr=10（即从系统启动到现在已经过去了 10 个 SysTick 周期），任务需要延时的时间 time=2（即任务需要延时 2 个 SysTick 周期），那么意味着当 OSTickCtr=12 时任务延时结束，这个 12 就是 TickCtrMatch 的值。

```c
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
```

