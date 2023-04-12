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

> 我们先停一下，举个例子来理解上面的原理。比如现在**时基计数器 OSTickCtr=10（即从系统启动到现在已经过去了 10 个 SysTick 周期），任务需要延时的时间 time=2（即任务需要延时 2 个 SysTick 周期）**，那么意味着当 OSTickCtr=12 （即从系统启动到现在已经过去了 12 个 SysTick 周期）时任务延时结束，此处的 12 就是 TickCtrMatch 的值。当时基计数器等于 12 时，捕捉到该任务延时结束（所以这也是 Match 的意思？）。
> 
> 嗯......看到这，你是不是觉得 OSTickCtr 有点像时间戳的作用了......但其实，OSTickCtr 记录的是“周期的时间戳”，而真正的时间戳记录的可是真实的时间哦！
>
> 回忆一下前几章中任务延时的办法：SysTick 发起一次中断，系统就遍历就绪列表中的 TCB，给每个 TCB 中的 TaskDelayTicks 减一。现在我们的做法是：定义了一个全局变量，从系统启动就开始计数。任务结束延时的时期，就是这个全局变量加上延时的周期长度，这个值我们记录在 TCB 中。为什么要换方法呢？你看看这两种办法的区别：前者是我们帮每个 TCB 减一，因此比较麻烦；后者是我们在外面计总的时间，你们这些要延时的 TCB 自己看好时间，到时间后你自己站出来报到。而且更绝的是，我们将这些 TCB 按照延时时间排好顺序，如果前一个 TCB 延时时间未到，那后面的 TCB 看都不用看，肯定没到时间。你看，把复杂的事情简单化，uCOS 绝吧！

- TickCtrMatch 除以时基列表的数组大小 OSCfg_TickWheelSize 求余，作为下标访问时基列表数组。

> 继续以上面例子为例来说明。现在，**TickCtrMatch = 12，我们定义 OSCfg\_TickWheelSize = 17，求余后是 12**，因此要访问的是 OSCfg\_TickWheel[12]。这里运用了**哈希算法**，为什么这样做需要到本文最后再解释。接下来，这个任务将插入到 OSCfg\_TickWheel[12] 下的双向升序链表中，剩余的工作就是大家熟悉的插入链表操作了。

- 如果双向链表为空：则该 TCB 的前向、后向指针域皆为零。
- 如果双向链表不为空：需要遍历整个链表，访问每一个节点，计算该节点的剩余时间，与待插入节点的剩余时间比较一下：如果被访问节点的大，那就插入到它前面去；否则，就是待插入节点的大了，插入到被访问节点的后面去。如果遍历完了都没找到比待插入节点大的，那就排在最后面去。
- 最后，新插入的 TCB 的 TickSpokePtr 需要指向对应的数组元素。

> 继续以上面例子说明。这个任务已经插入到 OSCfg\_TickWheel[12] 下的双向升序链表中了，那么该任务 TCB 的 TickSpokePtr 将指向 OSCfg\_TickWheel[12]，以说明自己在时基列表中的第 12 个位置。

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

再举个例子。我们现在 OSTickCtr = 10，OSCfg\_TickWheelSize = 12，有以下三个任务，情况分别是：
- 任务 1：需要延时 1 个周期。因此，TickRemain = 1，TickCtrMatch = 11，需要插入的数组下标是 11 % 12 = 11。
- 任务 2：需要延时 13 个周期。因此，TickRemain = 13，TickCtrMatch = 23，需要插入的数组下标是 23 % 12 = 11。
- 任务 3：需要延时 25 个周期。因此，TickRemain = 25，TickCtrMatch = 35，需要插入的数组下标是 35 % 12 = 11。

那么插入时基列表后的情况如下图：

【图片】

注意，这三个任务 TCB 都在同一条链表中，那只是数字凑巧而已。假如我把任务 2 的延时改为 14 个周期，那么插入的数组下标是 0，会插到别的链表中。

### 2.3 在时基列表删除一个指定的任务控制块 OS_TickListRemove() 

该函数是在时基列表中删除一个指定的任务 TCB，完成的工作有：
- 根据 TCB 的 TickSpokePtr，获取对应的时基列表数组元素，进而获取链表的头指针。
- 若头指针为空，则说明链表不存在，直接退出。
- 若不为空，说明链表存在，删除指定节点。
- 删除完后，将该 TCB 的前向、后向指针域、剩余时间全部清零。节点数减一。

```c
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
```

### 2.4 检查任务延时是否到期 OS_TickListUpdate()（os_tick.c）
