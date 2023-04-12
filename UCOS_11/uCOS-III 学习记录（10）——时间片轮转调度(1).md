参考内容：《[野火]uCOS-III内核实现与应用开发实战指南——基于STM32》第 14 章。

[toc]

## 0 时间片轮转调度的意义

我们现在实现的 uCOS 内核，包含了就绪列表和时基列表。就绪列表的插入规则与优先级有关，而时基列表的插入规则与时基计数器和延时时间有关。上一篇文章中，已经实现了时基列表的任务调度。但是我们写的内核还有一些缺陷，当不进行任务调度时（**注意：任务调度的实质是切换到高优先级的任务去执行**），系统只能执行就绪列表下的双向链表头指针对应的任务，那么相同优先级的其他任务就执行不到了。因此，我们希望相同优先级的多个任务都可以执行到，其中一个办法就是每隔一段时间就切换到相同优先级上的任务，这样这些任务都有机会被运行到。

**时间片轮转调度用于解决相同优先级下多个任务的运行问题**。现在假设 A 优先级下有任务 1、任务 2、任务 3，B 优先级有任务 4 且被阻塞 6 个时间片的长度，所以现在要运行 A 优先级下的任务。
- 如果不实现时间片轮转，那么 A 优先级排在最前面的任务 1 将运行 6 个时间片，即独占了这 6 个时间片。
- 如果实现时间片轮转，那么 A 优先级下的任务可以商量好，大家这样来：任务 1 先运行 2 个时间片，任务 2 再运行 2 个时间片，最后任务 3 也运行 2 个时间片，很公平，大家都被运行了。当然，也不一定平均分配，比如任务 1 先运行 1 个时间片，任务 2 再运行 2 个时间片，最后任务 3 运行 3 个时间片，这样也可以。

相同优先级的任务，谁都可以被运行，这就是时间片轮转调度带来的好处了。现在，我们就来实现这个机制。

## 1 修改任务控制块 TCB（os.h）

在实现轮转调度之前，先添加 TCB 的成员，有两个：
- TimeQuanta：用于记录该任务需要多少个时间片，这个值设置好后一般是不动的。
- TimeQuantaCtr：用于时间片计数，表示任务剩余的时间片个数。一旦数到了零，说明该任务已经用完了时间片，需要切换其他任务了。

```c
struct os_tcb{
	CPU_STK			*StkPtr;
	CPU_STK_SIZE	StkSize;
	
	OS_PRIO			Prio;				/* 任务优先级 */
	
	OS_TCB			*NextPtr;			/* 就绪列表双向链表的下一个指针 */
	OS_TCB			*PrevPtr;			/* 就绪列表双向链表的前一个指针 */
	
	OS_TCB			*TickNextPtr;		/* 指向链表的下一个 TCB 节点 */
	OS_TCB			*TickPrevPtr;		/* 指向链表的上一个 TCB 节点 */
	OS_TICK_SPOKE	*TickSpokePtr;		/* 用于回指到链表根部 */
	OS_TICK			TickCtrMatch;		/* 该值等于时基计数器 OSTickCtr 的值加上 TickRemain 的值 */
	OS_TICK			TickRemain;			/* 设置任务还需要等待多少个时钟周期 */
	
	OS_TICK			TimeQuanta;			/* 任务需要多少个时间片 */
	OS_TICK			TimeQuantaCtr;		/* 任务剩余的时间片个数 */
};
```

## 2 时间片轮转调度函数 OS_SchedRoundRobin()（os_core.c）

该函数实现了时间片轮转调度的功能，完成的步骤是：
- 传入的参数是就绪列表的一个数组元素，进而可以获知对应的双向链表头指针，即链表的第一个 TCB 节点。
- 如果双向链表为空，或者获得的是最低优先级的双向链表（是空闲任务 TCB 所在的地方），那么不进行调度。
- 否则就是普通情况了，把第一个 TCB 的时间片计数器减一。
- 如果减一后发现还未归零，则说明该任务的时间片未用完，不进行调度。
- 如果减一后发现还归零了，则说明该任务的时间片已用完，将第一个 TCB 放到链表最后。
- 第一个 TCB 换成了新的任务 TCB，设置好时间片计数器。

```c
/* 时间片调度函数 */
#if OS_CFG_SCHED_ROUND_ROBIN_EN > 0u
void OS_SchedRoundRobin (OS_RDY_LIST *p_rdy_list)
{
	OS_TCB *p_tcb;
	
	CPU_SR_ALLOC();
	CPU_CRITICAL_ENTER();		/* 进入临界段 */
	
	p_tcb = p_rdy_list->HeadPtr;
	
	/* 如果链表为空，或是空闲任务，则退出 */
	if ((p_tcb == (OS_TCB *)0) || (p_tcb == &OSIdleTaskTCB))
	{
		CPU_CRITICAL_EXIT();	/* 退出临界段 */
		return;
	}
	
	/* 如果时间片未用完，时间片个数减一 */
	if (p_tcb->TimeQuantaCtr > (OS_TICK)0)
	{
		p_tcb->TimeQuantaCtr--;
	}
	
	/* 如果减一后，时间片仍未用完，则退出 */
	if (p_tcb->TimeQuantaCtr > (OS_TICK)0)
	{
		CPU_CRITICAL_EXIT();	/* 退出临界段 */
		return;
	}
	
	/* 如果链表只有一个节点，则退出 */
	if (p_rdy_list->NbrEntries < (OS_OBJ_QTY)2)
	{
		CPU_CRITICAL_EXIT();	/* 退出临界段 */
		return;
	}
	
	/* 运行到此处时，意味着当前任务已经用完了时间片，将任务放到链表最后 */
	OS_RdyListMoveHeadToTail (p_rdy_list);
	
	/* 重设下一个任务的时间片计数 */
	p_tcb = p_rdy_list->HeadPtr;
	p_tcb->TimeQuantaCtr = p_tcb->TimeQuanta;

	CPU_CRITICAL_EXIT();		/* 退出临界段 */
}
#endif
```
需要注意的是，时间片轮转调度功能可以开启，也可以关闭，在宏定义 OS\_CFG\_SCHED\_ROUND\_ROBIN\_EN 中（位于 os\_cfg.h）可以设置。

## 3 修改相关代码

接下来，修改有关时间片的代码部分。

### 3.1 SysTick 中断发起后调用 OSTimeTick()（os_time.c）

当 SysTick 发起一次中断时，说明一个时间片已经过去，需调用 OS_SchedRoundRobin()，更新 TCB 中的时间片计数器，同时检查有无 TCB 的时间片用完。

```c
void OSTimeTick (void)
{
	/* 更新时基列表 */
	OS_TickListUpdate();
	
	/* 时间片调度 */
#if OS_CFG_SCHED_ROUND_ROBIN_EN > 0u
	OS_SchedRoundRobin (&OSRdyList[OSPrioCur]);
#endif
	
	/* 任务调度 */
	OSSched();  
}
```

### 3.2 任务创建函数 OSTaskCreate()（os_task.c）

需要加入初始化时间片成员的代码。现在，创建一个任务时，用户还需要指定该任务的时间片个数。

```c
/* 任务创建函数 */
void OSTaskCreate( 	OS_TCB 			*p_tcb,  		/* TCB指针 */
					OS_TASK_PTR 	p_task,  		/* 任务函数名 */
					void 			*p_arg,  		/* 任务的形参 */
					OS_PRIO			prio,			/* 任务优先级 */
					CPU_STK 		*p_stk_base, 	/* 任务栈的起始地址 */
					CPU_STK_SIZE 	stk_size,		/* 任务栈大小 */
					OS_TICK			time_quanta,	/* 时间片个数 */
					OS_ERR 			*p_err )		/* 错误码 */
{
	CPU_STK		*p_sp;
	CPU_SR_ALLOC();
	
	OS_TaskInitTCB (p_tcb);
	
	p_sp = OSTaskStkInit ( 	p_task,
							p_arg,
							p_stk_base,
							stk_size );  /* 任务栈初始化函数 */
	p_tcb->Prio		= prio;		/* 任务优先级保存在 TCB 的 prio 中 */
	p_tcb->StkPtr 	= p_sp;    	/* 剩余栈的栈顶指针 p_sp 保存到任务控制块 TCB 的 StkPtr 中 */
	p_tcb->StkSize 	= stk_size; /* 将任务栈的大小保存到任务控制块 TCB 的成员 StkSize 中 */
	
	p_tcb->TimeQuanta = time_quanta;
#if OS_CFG_SCHED_ROUND_ROBIN_EN > 0u
	p_tcb->TimeQuantaCtr = time_quanta;
#endif
	
	OS_CRITICAL_ENTER();	/* 进入临界段 */
	
	/* 将任务添加到就绪列表 */
	OS_PrioInsert (p_tcb->Prio);
	OS_RdyListInsertTail (p_tcb);
	
	OS_CRITICAL_EXIT();		/* 退出临界段 */
	
	*p_err = OS_ERR_NONE;		/* 函数执行到这里表示没有错误 */
}
```

### 3.3 空闲任务初始化函数 OS_IdleTaskInit()（os_core.c）

将空闲任务的时间片分配为 0，因为空闲任务位于最低优先级，而且是独占了最低优先级，因此不需要分配时间片。

```c
/* 空闲任务初始化函数 */ 
void OS_IdleTaskInit (OS_ERR *p_err)
{
	OSIdleTaskCtr = (OS_IDLE_CTR) 0;		/* 计数器清零 */
	
	OSTaskCreate ((OS_TCB*)      &OSIdleTaskTCB, 
	              (OS_TASK_PTR)  OS_IdleTask, 
	              (void *)       0,
				  (OS_PRIO)		(OS_CFG_PRIO_MAX - 1u),
	              (CPU_STK *)    OSCfg_IdleTaskStkBasePtr,
	              (CPU_STK_SIZE) OSCfg_IdleTaskStkSize,
				  (OS_TICK)		 0,
	              (OS_ERR *)     &p_err);		/* 创建空闲任务 */
}
```

## 4 时间片轮调度的应用
