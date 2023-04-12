参考内容：《[野火]uCOS-III内核实现与应用开发实战指南——基于STM32》第 15、16 和 21 章。

从本文开始，是 uCOS 的 API 应用。

[toc]

## 1 任务状态

在 uCOS 中，任务状态分为以下几种：

- **就绪（<code>OS\_TASK\_STATE\_RDY</code>）**：该任务在就绪列表中，就绪的任务已经具备执行的能力，只等待调度器进行调度，新创建的任务会初始化为就绪态。
- **延时（<code>OS\_TASK\_STATE\_DLY</code>）**：该任务处于延时调度状态。
- **等待（<code>OS\_TASK\_STATE\_PEND</code>）**：任务调用 OSQPend()、OSSemPend() 这类等待函数，系统
就会设置一个超时时间让该任务处于等待状态，如果超时时间设置为 0，任务的状态，无限期等下去，直到事件发生。如果超时时间为 N(N>0)，在 N 个时间内任务等待的事件或信号都没发生，就退出等待状态转为就绪状态。**（现阶段忽视）**
- **运行（<code>OS\_TASK\_STATE\_PEND\_TIMEOUT</code>）**：该状态表明任务正在执行，此时它占用处理器，uCOS 调度器选择运行的永远是处于最高优先级的就绪态任务，当任务被运行的一刻，它的任务状态就变成了运行态，其实运行态的任务也是处于就绪列表中的。
- **挂起（<code>OS\_TASK\_STATE\_SUSPENDED</code>）**：任务通过调用 OSTaskSuspend() 函数能够挂起自己或其他任务，调用 OSTaskResume() 是使被挂起的任务回复运行的唯一的方法。挂起一任务意味着该任务再被恢复运行以前不能够取得 CPU 的使用权，类似强行暂停一个任务。
- **延时+挂起（<code>OS\_TASK\_STATE\_DLY\_SUSPENDED</code>）**：任务先产生一个延时，延时没结束的时候被其他任务挂起，挂起的效果叠加，当且仅当延时结束并且挂起被恢复了，该任务才能够再次运行。
- **等待+挂起（<code>OS\_TASK\_STATE\_PEND\_SUSPENDED</code>）**：任务先等待一个事件或信号的发生（无限期等待），还没等待到就被其他任务挂起，挂起的效果叠加，当且仅当任务等待到事件或信号并且挂起被恢复了，该任务才能够再次运行。**（现阶段忽视）**
- **超时等待+挂起（<code>OS\_TASK\_STATE\_PEND\_TIMEOUT\_SUSPENDED</code>）**：任务在指定时间内等待事件或信号的产生，但是任务已经被其他任务挂起。**（现阶段忽视）**
- **删除（<code>OS\_TASK\_STATE\_DEL</code>）**：任务被删除后的状态，任务被删除后将不再运行，除非重新创建任务。

在 os.h 中宏定义了任务的状态值：

```c
/* 系统状态 */
#define  OS_STATE_OS_STOPPED                    (OS_STATE)(0u)
#define  OS_STATE_OS_RUNNING                    (OS_STATE)(1u)
	
/* 任务状态 */
#define	 OS_TASK_STATE_BIT_DLY					(OS_STATE)(0x01u)	/* 挂起位      				*/
#define	 OS_TASK_STATE_BIT_PEND					(OS_STATE)(0x02u)	/* 等待位      				*/
#define	 OS_TASK_STATE_BIT_SUSPENDED			(OS_STATE)(0x04u)	/* 延时/超时位 				*/
	
#define  OS_TASK_STATE_RDY						(OS_STATE)(   0u)	/* 0 0 0  就绪 				*/
#define  OS_TASK_STATE_DLY						(OS_STATE)(   1u)	/* 0 0 1  延时/超时 			*/
#define  OS_TASK_STATE_PEND						(OS_STATE)(   2u)	/* 0 1 0  等待	 			*/
#define  OS_TASK_STATE_PEND_TIMEOUT				(OS_STATE)(   3u)	/* 0 1 1  等待+超时 			*/
#define  OS_TASK_STATE_SUSPENDED				(OS_STATE)(   4u)	/* 1 0 0  挂起 				*/
#define  OS_TASK_STATE_DLY_SUSPENDED			(OS_STATE)(   5u)	/* 1 0 1  挂起+延时/超时 	*/
#define  OS_TASK_STATE_PEND_SUSPENDED			(OS_STATE)(   6u)	/* 1 1 0  挂起+等待		 	*/
#define  OS_TASK_STATE_PEND_TIMEOUT_SUSPENDED	(OS_STATE)(   7u)	/* 1 1 1  挂起+超时+等待 	*/
#define  OS_TASK_STATE_DEL						(OS_STATE)( 255u)	
```

## 2 修改和添加相关代码
### 2.1 修改 TCB（os.h）

TCB 中增加两个成员：
- TaskState：标志任务的状态。
- SuspendCtr：记录任务被挂起了几次。一个任务挂起多少次就要被恢复多少次才能重新运行。

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
	
	OS_STATE		TaskState;			/* 表示任务的状态 */
	
#if OS_CFG_TASK_SUSPENDED_EN > 0u
	OS_NESTING_CTR	SuspendCtr;			/* 任务挂起函数 OSTaskSuspend() 计数器 */
#endif
};
```

### 2.2 添加宏定义和数据类型

在 os_cfg.h 中添加宏定义，用于使能任务挂起和删除功能，这两个功能可以开启也可以关闭：

```c
/* 使能任务挂起功能 */
#define OS_CFG_TASK_SUSPENDED_EN          	1u

/* 使能任务删除功能 */
#define OS_CFG_TASK_DEL_EN					1u
```

在 os_type.h 中增加数据类型：

```c
typedef   CPU_INT08U      OS_NESTING_CTR;
```

## 3 任务管理的函数

下面的这些函数，属于 uCOS 的 API 函数，方便用户进行调用。

### 3.1 任务挂起函数 OSTaskSuspend()（os_task.c）

该函数用于将一个任务挂起，被挂起的任务就位于挂起态了，它的 TCB 将会被移出就绪列表，而且不会参与任何任务调度，除非有其他任务主动将这个任务恢复，即从挂起态转为就绪态，否则它没有机会获得运行权。

该函数完成的工作是：
- 如果任务 TCB 是空的，则默认要挂起的任务是自己。
- 如果任务挂起的是自己，则判断下调度器是否锁住，如果锁住则退出返回错误码，没有锁则继续往下执行。

接下来根据任务的不同状态，执行不同的操作：
- 如果任务在就绪状态，则将任务的状态改为挂起态，挂起计数器置 1，然后从就绪列表删除。
- 如果任务在延时状态，则将任务的状态改为延时加挂起态，挂起计数器置 1，不用改变 TCB 的位置，即还是在延时的时基列表。
- 如果任务在等待状态，则将任务的状态改为等待加挂起态，挂起计数器置 1，不用改变 TCB 的位置，即还是在等待列表等待。**等待列表目前仍未实现。**
- 如果任务在等待加超时态，则将任务的状态改为等待加超时加挂起态，挂起计数器置 1，不用改变 TCB 的位置，即还在等待和时基这两个列表中。**等待列表目前仍未实现。**
- 如果任务处于挂起态，或者是挂起加其他态，则将挂起计数器加一操作，不用改变 TCB 的位置。
- 其他状态则无效，退出返回状态无效错误码。
- 最后，改变了任务状态后，需要进行任务切换。

```c
/* 任务挂起函数 */
#if OS_CFG_TASK_SUSPENDED_EN > 0u
void OSTaskSuspend (OS_TCB *p_tcb, OS_ERR *p_err)
{
	CPU_SR_ALLOC();	
	CPU_CRITICAL_ENTER();
	
	if (p_tcb == (OS_TCB *)0)
	{
		p_tcb = OSTCBCurPtr;
	}
	
	if (p_tcb == OSTCBCurPtr)
	{
		if (OSSchedLockNestingCtr > (OS_NESTING_CTR)0)	/* 如果调度器锁住则不能挂起自己 */
		{
			CPU_CRITICAL_EXIT();
			*p_err = OS_ERR_SCHED_LOCKED;
			return;
		}
	}
	
	*p_err = OS_ERR_NONE;
	
	switch (p_tcb->TaskState)
	{
		case OS_TASK_STATE_RDY:
			OS_CRITICAL_ENTER_CPU_CRITICAL_EXIT();
			p_tcb->TaskState  = OS_TASK_STATE_SUSPENDED;
			p_tcb->SuspendCtr = (OS_NESTING_CTR)1;
			OS_RdyListRemove (p_tcb);
			OS_CRITICAL_EXIT_NO_SCHED();
			break;
		
		case OS_TASK_STATE_DLY:
			p_tcb->TaskState  = OS_TASK_STATE_DLY_SUSPENDED;
			p_tcb->SuspendCtr = (OS_NESTING_CTR)1;
			CPU_CRITICAL_EXIT();
			break;
		
		case OS_TASK_STATE_PEND:
			p_tcb->TaskState  = OS_TASK_STATE_PEND_SUSPENDED;
			p_tcb->SuspendCtr = (OS_NESTING_CTR)1;
			CPU_CRITICAL_EXIT();
			break;
		
		case OS_TASK_STATE_PEND_TIMEOUT:
			p_tcb->TaskState  = OS_TASK_STATE_PEND_TIMEOUT_SUSPENDED;
			p_tcb->SuspendCtr = (OS_NESTING_CTR)1;
			CPU_CRITICAL_EXIT();
			break;
		
		case OS_TASK_STATE_SUSPENDED:
		case OS_TASK_STATE_DLY_SUSPENDED:
		case OS_TASK_STATE_PEND_SUSPENDED:
		case OS_TASK_STATE_PEND_TIMEOUT_SUSPENDED:
			p_tcb->SuspendCtr++;
			CPU_CRITICAL_EXIT();
			break;
		
		default:
			CPU_CRITICAL_EXIT();
			*p_err = OS_ERR_STATE_INVALID;
			return;
	}
	
	OSSched();	/* 任务切换 */
	
	CPU_CRITICAL_EXIT();
}
#endif
```

### 3.2 任务恢复函数 OSTaskResume()（os_task.c）

该函数用于恢复一个处在挂起态的任务。比如，A 任务处在挂起加延时态，B 任务处在挂起加等待态，C 任务处于挂起加等待加延时态，那么当别的任务恢复它们后，A 任务处在延时态，B 任务处在等待态，C 任务处于等待加延时态。言下之意，就是把挂起态给去掉了。

需要注意的是，**任务可以挂其自身，但不能恢复自身**，因为自己都被挂起了，没有机会被运行了，又怎么能自己恢复自己呢！只有当别的任务进行恢复操作时，任务才能从挂起态恢复过来。

```c
/* 任务恢复函数 */
#if OS_CFG_TASK_SUSPENDED_EN > 0u
void OSTaskResume (OS_TCB *p_tcb, OS_ERR *p_err)
{
	CPU_SR_ALLOC();
	CPU_CRITICAL_ENTER();
	
	*p_err = OS_ERR_NONE;
	
	switch (p_tcb->TaskState)
	{
		case OS_TASK_STATE_RDY:
		case OS_TASK_STATE_DLY:
		case OS_TASK_STATE_PEND:
		case OS_TASK_STATE_PEND_TIMEOUT:
			CPU_CRITICAL_EXIT();
			*p_err = OS_ERR_TASK_NOT_SUSPENDED;
			break;
		
		case OS_TASK_STATE_SUSPENDED:
			OS_CRITICAL_ENTER_CPU_CRITICAL_EXIT();
			p_tcb->SuspendCtr--;
			if (p_tcb->SuspendCtr == (OS_NESTING_CTR)0)
			{
				p_tcb->TaskState = OS_TASK_STATE_RDY;
				OS_TaskRdy (p_tcb);
			}
			OS_CRITICAL_EXIT_NO_SCHED();
			break;
		
		case OS_TASK_STATE_DLY_SUSPENDED:
			p_tcb->SuspendCtr--;
			if (p_tcb->SuspendCtr == (OS_NESTING_CTR)0)
			{
				p_tcb->TaskState = OS_TASK_STATE_DLY;
			}
			CPU_CRITICAL_EXIT();
			break;
			
		case OS_TASK_STATE_PEND_SUSPENDED:
			p_tcb->SuspendCtr--;
			if (p_tcb->SuspendCtr == (OS_NESTING_CTR)0)
			{
				p_tcb->TaskState = OS_TASK_STATE_PEND;
			}
			CPU_CRITICAL_EXIT();
			break;
			
		case OS_TASK_STATE_PEND_TIMEOUT_SUSPENDED:
			p_tcb->SuspendCtr--;
			if (p_tcb->SuspendCtr == (OS_NESTING_CTR)0)
			{
				p_tcb->TaskState = OS_TASK_STATE_PEND_TIMEOUT;
			}
			CPU_CRITICAL_EXIT();
			break;
			
		default:
			CPU_CRITICAL_EXIT();
			*p_err = OS_ERR_STATE_INVALID;
			return;
	}
	
	OSSched();	/* 任务切换 */
	
	CPU_CRITICAL_EXIT();
}
#endif
```

### 3.3 任务删除函数 OSTaskDel()（os_task.c）

```c
/* 任务删除函数 */
#if OS_CFG_TASK_DEL_EN > 0u
void OSTaskDel (OS_TCB *p_tcb, OS_ERR *p_err)
{
	CPU_SR_ALLOC();
	
	if (p_tcb == &OSIdleTaskTCB)	/* 不能删除空闲任务 */
	{
		*p_err = OS_ERR_TASK_DEL_IDLE;
		return;
	}
	
	if (p_tcb == (OS_TCB *)0)
	{
		CPU_CRITICAL_ENTER();
		p_tcb = OSTCBCurPtr;
		CPU_CRITICAL_EXIT();
	}
	
	OS_CRITICAL_ENTER();
	
	switch (p_tcb->TaskState)
	{
		case OS_TASK_STATE_RDY:
			OS_RdyListRemove (p_tcb);
			break;
		
		case OS_TASK_STATE_SUSPENDED:
			break;
		
		case OS_TASK_STATE_DLY:
		case OS_TASK_STATE_DLY_SUSPENDED:
			OS_TickListRemove (p_tcb);
			break;
		
		case OS_TASK_STATE_PEND:
		case OS_TASK_STATE_PEND_TIMEOUT:
		case OS_TASK_STATE_PEND_SUSPENDED:
		case OS_TASK_STATE_PEND_TIMEOUT_SUSPENDED:
			OS_TickListRemove (p_tcb);
		
		default:
			OS_CRITICAL_EXIT();
			*p_err = OS_ERR_STATE_INVALID;
			return;
	}
	
	OS_TaskInitTCB (p_tcb);
	p_tcb->TaskState = OS_TASK_STATE_DEL;
	
	OS_CRITICAL_ENTER_CPU_CRITICAL_EXIT();
	
	OSSched();	/* 任务切换 */
	
	*p_err = OS_ERR_NONE;
}
#endif

```

