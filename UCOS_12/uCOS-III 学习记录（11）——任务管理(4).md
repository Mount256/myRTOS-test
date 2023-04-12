参考内容：《[野火]uCOS-III内核实现与应用开发实战指南——基于STM32》第 15、16 和 21 章。

从本文开始，是 uCOS 的 API 应用。

[toc]

## 1 任务状态

在 uCOS 中，任务状态分为以下几种，任务就是在这几种状态中来回变化的：

- **就绪（<code>OS\_TASK\_STATE\_RDY</code>）**：该任务在就绪列表中，就绪的任务已经具备执行的能力，只等待调度器进行调度，新创建的任务会初始化为就绪态。
- **延时（<code>OS\_TASK\_STATE\_DLY</code>）**：该任务处于延时调度状态。
- **等待（<code>OS\_TASK\_STATE\_PEND</code>）**：任务调用 OSQPend()、OSSemPend() 这类等待函数，系统就会设置一个超时时间让该任务处于等待状态，如果超时时间设置为 0，任务的状态，无限期等下去，直到事件发生。如果超时时间为 N(N>0)，在 N 个时间内任务等待的事件或信号都没发生，就退出等待状态转为就绪状态。**（现阶段忽视）**
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

**任务的挂起与恢复函数**在很多时候都是很有用的，比如我们想暂停某个任务运行一段时间，但是我们又需要在其恢复的时候继续工作，那么删除任务是不可能的，因为删除了任务的话，任务的所有的信息都是不可能恢复的了，删除是完完全全删除了，里面的资源都被系统释放掉，但是挂起任务就不会这样。**调用挂起任务函数，仅仅是将任务进入挂起态，其内部的资源都会保留下来，同时也不会参与系统中任务的调度，当调用恢复函数的时候，整个任务立即从挂起态进入就绪态，并且参与任务的调度**，如果该任务的优先级是当前就绪态优先级最高的任务，那么立即会按照挂起前的任务状态继续执行该任务。也就是说，挂起任务之前是什么状态，都会被系统保留下来，在恢复的瞬间，继续执行。

**删除任务是说任务将返回并处以删除（休眠）状态，任务的代码不再被 uCOS 调用，删除任务不是删除代码**。删除任务和挂起任务有些相似，但最大的不同就是**删除任务 TCB 的操作**。我们知道在任务创建的时候，需要给每个任务分配一个 TCB，TCB 存储有关这个任务重要的信息，对任务间有至关重要的作用，挂起任务根本不会动 TCB，但删除任务就会把 TCB 进行初始化，这样关于任务的任何信息都被抹去。**注意，删除任务并不会释放任务的栈空间。**

以上所提及的三个函数，都属于 uCOS 的 API 函数，方便用户进行调用。

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

该函数根据任务的不同状态，执行不同的操作：
- 只要任务没有处于挂起态的，退出返回任务没有被挂起的错误码。
- 如果任务在挂起状态，则递减挂起计数器 SuspendCtr，如果 SuspendCtr 等于 0，则将任务的状态改为就绪态，并让任务就绪。
- 如果任务在延时加挂起态，则递减挂起计数器 SuspendCtr，如果 SuspendCtr 等于 0，则将任务的状态改为延时态。
- 如果任务在延时加等待态，则递减挂起计数器 SuspendCtr，如果 SuspendCtr 等于 0，则将任务的状态改为等待态。
- 如果任务在等待加超时加挂起态，则递减挂起计数器 SuspendCtr，如果 SuspendCtr 等于 0，则将任务的状态改为等待加超时态。
- 其他状态则无效，退出返回状态无效错误码。
- 最后，改变了任务状态后，需要进行任务切换。

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

任务通常会运行在一个死循环中，也不会退出，如果一个任务不再需要，可以调用 uCOS 中的任务删除 API 函数接口显式地将其删除。

该函数用于将任务删除。所谓删除，就是将被删除的任务 TCB 移出就绪列表，同时清空该任务 TCB 的所有信息。

该函数完成的工作有：
- 如果发现删除的是空闲任务，则返回错误码，因为空闲任务不能被删除，系统必须至少有一个任务在运行，当没有其他用户任务运行的时候，系统就会运行空闲任务。
- 如果任务 TCB 是空的，则默认要删除的任务是自己。

然后根据任务的不同状态，执行不同的操作：
- 如果任务处于就绪态，则从就绪列表移除。
- 如果任务处于挂起态，则什么都不用做。
- 如果任务在延时态或者是延时加挂起态，则从时基列表移除。
- 如果任务在等待态或是等待加其他态，则从时基列表和等待列表移除。**等待列表目前仍未实现。**
- 其他状态则无效，退出返回状态无效错误码。

最后：
- 清空 TCB 至默认值。
- 修改任务的状态为删除态，即处于休眠。
- 进行任务调度。

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

## 4 任务管理的应用
### 4.1 主函数 main()（app.c）

修改两个任务：
- Task1：两次的阻塞延时改为两次的挂起任务自身。
- Task2：增加恢复任务 Task1。
- Task3 不用修改。

```c
#include "ARMCM3.h"
#include "os.h"

#define  TASK1_STK_SIZE       128
#define  TASK2_STK_SIZE       128
#define  TASK3_STK_SIZE       128

static   CPU_STK   Task1Stk[TASK1_STK_SIZE];
static   CPU_STK   Task2Stk[TASK2_STK_SIZE];
static   CPU_STK   Task3Stk[TASK3_STK_SIZE];

static   OS_TCB    Task1TCB;
static   OS_TCB    Task2TCB;
static   OS_TCB    Task3TCB;

uint32_t flag1;
uint32_t flag2;
uint32_t flag3;

void Task1 (void *p_arg);
void Task2 (void *p_arg);
void Task3 (void *p_arg);

/* 软件延时 */
void delay(uint32_t count);

int main (void)
{
	OS_ERR err;
	
	/* 初始化相关的全局变量，创建空闲任务 */
	OSInit(&err);
	
	/* CPU 初始化：初始化时间戳 */
	CPU_Init();
	
	/* 关中断，因为此时 OS 未启动，若开启中断，那么 SysTick 将会引发中断 */
	CPU_IntDis();
	
	/* 初始化 SysTick，配置 SysTick 为 10ms 中断一次，Tick = 10ms */
	OS_CPU_SysTickInit(10);
	
	/* 创建任务 */
	OSTaskCreate ((OS_TCB*)      &Task1TCB, 
	              (OS_TASK_PTR)  Task1, 
	              (void *)       0,
				  (OS_PRIO)		 1,
	              (CPU_STK*)     &Task1Stk[0],
	              (CPU_STK_SIZE) TASK1_STK_SIZE,
				  (OS_TICK)		 0,
	              (OS_ERR *)     &err);

	OSTaskCreate ((OS_TCB*)      &Task2TCB, 
	              (OS_TASK_PTR)  Task2, 
	              (void *)       0,
				  (OS_PRIO)		 2,
	              (CPU_STK*)     &Task2Stk[0],
	              (CPU_STK_SIZE) TASK2_STK_SIZE,
				  (OS_TICK)		 0,
	              (OS_ERR *)     &err);
				  
	OSTaskCreate ((OS_TCB*)      &Task3TCB, 
	              (OS_TASK_PTR)  Task3, 
	              (void *)       0,
				  (OS_PRIO)		 3,
	              (CPU_STK*)     &Task3Stk[0],
	              (CPU_STK_SIZE) TASK3_STK_SIZE,
				  (OS_TICK)		 0,
	              (OS_ERR *)     &err);
	
	/* 启动OS，将不再返回 */				
	OSStart(&err);
}

/* 软件延时 */
void delay (uint32_t count)
{
	for(; count!=0; count--);
}


void Task1 (void *p_arg)
{
	OS_ERR	err;
	
	for (;;)
	{
		flag1 = 1;
		OSTaskSuspend (&Task1TCB, &err);
		flag1 = 0;
		OSTaskSuspend (&Task1TCB, &err);
	}
}

void Task2 (void *p_arg)
{
	OS_ERR	err;
	
	for (;;)
	{
		flag2 = 1;
		OSTimeDly (2);		
		flag2 = 0;
		OSTimeDly (2);
		OSTaskResume (&Task1TCB, &err);
	}
}

void Task3 (void *p_arg)
{
	for (;;)
	{
		flag3 = 1;
		OSTimeDly (2);		
		flag3 = 0;
		OSTimeDly (2);
	}
}
```

### 4.2 运行过程
#### 4.2.1 在主函数中

- 系统初始化：初始化各种全局变量，初始化优先级表，初始化就绪列表，初始化时基列表，初始化空闲任务（包括初始化空闲任务栈和空闲任务 TCB）。
- CPU 初始化：暂为空。
- 关中断：因为此时 OS 未启动，若开启中断，那么 SysTick 将会引发中断，打断初始化流程。
- 初始化 SysTick：配置 SysTick 为 10ms 中断一次，Tick = 10ms。
- 创建任务：包括创建任务栈和任务 TCB，以及将 TCB 插入到就绪列表中，在优先级表对应位置置位。
- 启动系统：先找到最高优先级，然后开始运行最高优先级对应的任务（最高优先级为 1，即为 Task1），启动第一次任务切换（此时将完成最后的初始化流程，即有关 PendSV 的中断优先级配置，接着触发 PendSV 异常，发起任务切换），将 CPU 占有权交给任务 Task1。

#### 4.2.2 第一次在 Task1 中

- flag1 = 1。
- 执行到任务挂起函数 OSTaskSuspend：挂起自身任务，挂起计数器加一，移出就绪列表，进行任务调度。
- 执行任务调度 OSSched：任务调度器先找到最高优先级，然后再找到最高优先级的任务。 TCB。如果发现该任务就是当前任务，则不进行任务切换。在本案例中发现最高优先级为 2，对应任务是 Task2，不是当前任务，则发起任务切换（发起 PendSV 异常）。
- PendSV 异常处理程序：保存 Task1 的状态，加载 Task2 的状态，更新全局变量的值。

#### 4.2.3 第一次在 Task2 中

- flag2 = 1。
- 执行到阻塞函数 OSTimeDly：将 Task2 的 TCB 插入到时基列表中（TickCtrMatch = 2），将就绪列表中的 TCB 移除（同时在优先级表中的相应位置，即优先级 2 的位置清零），然后启动任务调度。
- 执行任务调度 OSSched：任务调度器先找到最高优先级，然后再找到最高优先级的任务 TCB。如果发现该任务就是当前任务，则不进行任务切换。在本案例中发现最高优先级为 3，对应任务是 Task3，不是当前任务，则发起任务切换（发起 PendSV 异常）。
- PendSV 异常处理程序：保存 Task2 的状态，加载 Task3 的状态，更新全局变量的值。

#### 4.2.4 第一次在 Task3 中

- flag3 = 1。
- 执行到阻塞函数 OSTimeDly：将 Task3 的 TCB 插入到时基列表中（TickCtrMatch = 2），将就绪列表中的 TCB 移除（同时在优先级表中的相应位置，即优先级 3 的位置清零），然后启动任务调度。
- 执行任务调度 OSSched：任务调度器先找到最高优先级，然后再找到最高优先级的任务 TCB。如果发现该任务就是当前任务，则不进行任务切换。在本案例中发现最高优先级为 31，对应任务是 空闲任务，不是当前任务，则发起任务切换（发起 PendSV 异常）。
- PendSV 异常处理程序：保存 Task3 的状态，加载空闲任务的状态，更新全局变量的值。

#### 4.2.5 在空闲任务中 SysTick 发起中断

- 执行 OSTimeTick：时基计数器加一（OSTickCtr = 1），检查时基列表（OSCfg\_TickWheel[1]），发现各个任务的延时未到期（TickCtrMatch = 2）。最后发起任务调度，发现不用进行任务切换，空闲任务继续运行。
- 再次发起中断，执行 OSTimeTick：时基计数器加一（OSTickCtr = 2），检查时基列表（OSCfg\_TickWheel[2]），发现各个任务的延时已到期（TickCtrMatch = 2），将它们全部置为就绪态。置为就绪态的过程是：在时基列表中删除对应 TCB，在就绪列表中加入对应 TCB（同时将 Task2、Task3 的优先级在优先级表中的相应位置重新置位）。最后发起任务调度，发现最高优先级为 2，对应的是 Task2，切换到 Task2 运行。

#### 4.2.6 第二次在 Task2 中

- 将全局变量 flag2 由 1 变成 0。
- 执行到阻塞函数 OSTimeDly：将 Task2 的 TCB 插入到时基列表中（TickCtrMatch = 4），将就绪列表中的 TCB 移除（同时在优先级表中的相应位置，即优先级 1 的位置清零），然后启动任务调度。
- 执行任务调度 OSSched：任务调度器先找到最高优先级，然后再找到最高优先级的任务 TCB。如果发现该任务就是当前任务，则不进行任务切换。在本案例中发现最高优先级为 3，对应任务是 Task3，不是当前任务，则发起任务切换（发起 PendSV 异常）。
- PendSV 异常处理程序：保存 Task2 的状态，加载 Task3 的状态，更新全局变量的值。

#### 4.2.7 第二次在 Task3 中

- 将全局变量 flag3 由 1 变成 0。
- 执行到阻塞函数 OSTimeDly：将 Task3 的 TCB 插入到时基列表中（TickCtrMatch = 4），将就绪列表中的 TCB 移除（同时在优先级表中的相应位置，即优先级 1 的位置清零），然后启动任务调度。
- 执行任务调度 OSSched：任务调度器先找到最高优先级，然后再找到最高优先级的任务 TCB。如果发现该任务就是当前任务，则不进行任务切换。在本案例中发现最高优先级为 31，对应任务是 空闲任务，不是当前任务，则发起任务切换（发起 PendSV 异常）。
- PendSV 异常处理程序：保存 Task3 的状态，加载空闲任务的状态，更新全局变量的值。

#### 4.2.8 在空闲任务中 SysTick 发起中断

- 执行 OSTimeTick：时基计数器加一（OSTickCtr = 3），检查时基列表（OSCfg\_TickWheel[3]），发现各个任务的延时未到期（TickCtrMatch = 4）。最后发起任务调度，发现不用进行任务切换，空闲任务继续运行。
- 再次发起中断，执行 OSTimeTick：时基计数器加一（OSTickCtr = 4），检查时基列表（OSCfg\_TickWheel[4]），发现各个任务的延时已到期（TickCtrMatch = 4），将它们全部置为就绪态。置为就绪态的过程是：在时基列表中删除对应 TCB，在就绪列表中加入对应 TCB（同时将 Task2、Task3 的优先级在优先级表中的相应位置重新置位）。最后发起任务调度，发现最高优先级为 2，对应的是 Task2，切换到 Task2 运行。

#### 4.2.9 第三次在 Task2 中

- 执行 OSTaskResume：恢复 Task1，使 Task1 的挂起计数器减一，将 Task1 的 TCB 加入到就绪列表中。注意，由于 TCB 不存在于时基列表中，因此不用删除。之后发起任务调度，发现最高优先级为 1，对应的是 Task1，切换到 Task1 运行。

#### 4.2.10 第二次在 Task1 中

- 将全局变量 flag1 由 1 变成 0。
- 执行到任务挂起函数 OSTaskSuspend：挂起自身任务，挂起计数器加一，移出就绪列表，进行任务调度。
- 执行任务调度 OSSched：任务调度器先找到最高优先级，然后再找到最高优先级的任务。 TCB。如果发现该任务就是当前任务，则不进行任务切换。在本案例中发现最高优先级为 2，对应任务是 Task2，不是当前任务，则发起任务切换（发起 PendSV 异常）。
- PendSV 异常处理程序：保存 Task1 的状态，加载 Task2 的状态，更新全局变量的值。

如此反复，不再赘述。

### 4.3 实验现象

实验现象如下图所示，符合以上分析：

【图片】



