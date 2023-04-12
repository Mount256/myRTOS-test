参考内容：《[野火]uCOS-III内核实现与应用开发实战指南——基于STM32》第 8 章。

[toc]

## 1. 空闲任务

不知道有没有注意到这样一个问题：我在学习 x86 汇编语言的时候，曾详细研读过系统内核的代码，内核本身也是有自己的 TCB 的，内核可作为一个“任务管理器”来对其他任务进行创建、删除等操作。那么 uCOS 作为一个实时操作系统，内核也应该需要自己的任务和 TCB，因此我们需要创建一个空闲任务给内核，这样，在没有任务的时候，内核空转运行空闲任务，同时检查各个任务的状态以做出相应的操作（因此 uCOS 内核也可视为“任务管理器”）。

### 1.1 数据类型定义

空闲任务虽然与其他任务的作用不同，但它的本质依然是一个任务，TCB、任务栈一个都不能少。

#### 1.1.1 空闲任务 TCB (os.h)

在 os.h 中定义 TCB：
```c
OS_EXT  OS_TCB			OSIdleTaskTCB;
```

#### 1.1.2 空闲任务栈 (os\_cfg\_app.c)

有关空闲任务栈的定义，并不在 os.h 中。

首先在 os\_cfg\_app.h 中定义了空闲任务栈的大小（感觉叫栈尺寸更合适？栈粒度应该是 4 字节）：
```c
/* 空闲任务栈大小 */
#define OS_CFG_IDLE_TASK_STK_SIZE		128u
```

然后，在 os\_cfg\_app.c 中定义栈和栈大小：
```c
CPU_STK 	OSCfg_IdleTaskStk[OS_CFG_IDLE_TASK_STK_SIZE]; 		/* 空闲任务栈 */

CPU_STK		*const 	OSCfg_IdleTaskStkBasePtr = (CPU_STK *) &OSCfg_IdleTaskStk[0];	/* 空闲任务栈的起始地址 */
CPU_STK		 const	OSCfg_IdleTaskStkSize	 = (CPU_STK_SIZE) OS_CFG_IDLE_TASK_STK_SIZE;	/* 空闲任务栈的大小 */
```

### 1.2 空闲任务函数 OS_IdleTask (os_core.c)

作为一个任务，任务主体是不能缺的。

按照书上的教程，它定义了一个全局变量（OSIdleTaskCtr），用来计数。我不知道这个计数是干什么用的，我觉得在目前的学习阶段而言，空闲任务函数可以什么都不做。可能是作者觉得内核干一些无意义的事也好过什么事也不干吧。
```c
typedef   CPU_INT32U	  OS_IDLE_CTR;

OS_EXT  OS_IDLE_CTR		OSIdleTaskCtr;
```

注意：
- 本函数不允许用户调用。

以下是空闲任务函数，用来无聊的计数：
```c
/* 空闲任务 */	
void OS_IdleTask (void *p_arg)
{
	p_arg = p_arg;
	
	/* 空闲任务什么都不做，只对全局变量OSIdleTaskCtr ++ 操作 */
	for (;;)
	{
		OSIdleTaskCtr++;
	}
}
```

### 1.3 空闲任务初始化函数 (os_core.c)

很简单，就完成两个功能：
- 创建空闲任务：调用 OSTaskCreate 即可。
- 计数清零：将 OSIdleTaskCtr 清零，为无聊的计数做准备。

注意：
- 本函数不允许用户调用。

```c
/* 空闲任务初始化函数 */ 
void OS_IdleTaskInit (OS_ERR *p_err)
{
	OSIdleTaskCtr = (OS_IDLE_CTR) 0;		/* 计数器清零 */
	
	OSTaskCreate ((OS_TCB*)      &OSIdleTaskTCB, 
	              (OS_TASK_PTR)  OS_IdleTask, 
	              (void *)       0,
	              (CPU_STK *)    OSCfg_IdleTaskStkBasePtr,
	              (CPU_STK_SIZE) OSCfg_IdleTaskStkSize,
	              (OS_ERR *)     &p_err);		/* 创建空闲任务 */
}
```

那么，在 OS 初始化的时候，调用该函数即可完成空闲任务的创建：
```c
/* OS 系统初始化，用于初始化全局变量 */
void OSInit (OS_ERR *p_err)
{
	/* 系统用一个全局变量 OSRunning 来指示系统的运行状态。系统初始化时，默认为停止状态，即 OS_STATE_OS_STOPPED */
	OSRunning = OS_STATE_OS_STOPPED;
	
	OSTCBCurPtr 	= (OS_TCB *) 0; /* 指向当前正在运行的任务的 TCB 指针 */
	OSTCBHighRdyPtr = (OS_TCB *) 0; /* 指向就绪任务中优先级最高的任务的 TCB */
	
	OS_RdyListInit();  /* 初始化就绪列表 */
	
	OS_IdleTaskInit(p_err); /* ----> 初始化空闲任务 */
	
	if (*p_err != OS_ERR_NONE) {
		return;
	}
}
```

## 2 阻塞延时

很多时候，某些任务运行到某处就需要延时一段时间，什么都不做。比如驱动某外设，需要按照时序，延时一段时间，再去访问接口。为了能榨干 CPU 的性能，不让它空转，在延时的这段时间内，还是要去完成其他的任务。

任务自行发起的延时，会使得任务自身发生阻塞，不再运行下去，这种延时就叫阻塞延时。而在阻塞延时期间，我们不让 CPU 闲着干等着，所以就有了任务切换（任务调度）。如果没有别的任务可以切换（调度），那就切换到内核的空闲任务。无论怎样，只要别让 CPU 闲着就行了。

### 2.1 数据类型定义

在 TCB 中加入了记录延时长度的成员，最小单位为 1 个 Tick，之前在上一篇笔记中我们初始化一个 Tick 为 10ms，因此最小单位为 10ms。

```c
/* TCB 重命名为大写字母格式 */
typedef struct os_tcb	OS_TCB;

/* TCB 数据类型声明 */
struct os_tcb{
	CPU_STK			*StkPtr;
	CPU_STK_SIZE	StkSize;
	OS_TICK			TaskDelayTicks;		/* 任务延时多少个 Ticks，注意 1 个 Ticks 为 10ms */
};
```

下面来实现阻塞延时的函数。

### 2.2 阻塞延时函数 OSTimeDly (os_time.c)

一旦我们在任务中发起延时，那么在阻塞延时函数中完成两件事：
- 延时长度记录到当前运行任务的 TCB 中，方便系统查看。
- 发起任务切换（更准确的说法是任务调度）。

```c
/* 阻塞延时 */
void OSTimeDly (OS_TICK dly)
{
	/* 延时时间 */
	OSTCBCurPtr->TaskDelayTicks = dly;
	/* 任务切换 */
	OSSched();
}
```

特别需要注意，阻塞延时函数跟软延时 Delay 的本质是不同的。软延时是 CPU 真的会卡在 for 循环里，什么事都干不了；而阻塞延时函数的实质是 CPU 设置了一下延时时间，然后就去忙别的了。那谁负责计时？当然是我们的 SysTick 了。

### 2.3 SysTick 发起中断后调用 OSTimeTick (os_time.c)

之前已提及，阻塞延时函数负责设置延时时间，书承上节内容，在 SysTick 发起一次中断时，表明一次 Tick 已经到来（在本案例中是 已经过去了 10ms），此时，把 TCB 中记录延时的数值减去 1，表示已经过去了一个 Tick（即过去了 10ms）。之后，还要发起一次任务调度，看看有没有任务已经延时结束的。

因此，实际上该函数起了这么一个作用：每隔 10ms，我帮你计数一次，起到了一个延时计数的功能。

```c
void OSTimeTick (void)
{
	OS_PRIO i;
	
	/* 遍历整个就绪列表，如果延时未到时，则减 1 */
	for ( i = 0u; i < OS_CFG_PRIO_MAX; i++)
	{
		if ( OSRdyList[i].HeadPtr->TaskDelayTicks > 0u )
		{
			OSRdyList[i].HeadPtr->TaskDelayTicks --;
		}
	}
	
	/* 任务调度 */
	OSSched();  
}
```

### 2.4 任务调度 OSChed (os_core.c)

从现在起，该函数不再叫任务切换了，而是叫任务调度器，因为已经有三个任务了。

我们实现的任务调度的算法很简单，也很朴素，就是去一个个检查其他其他任务是否延时结束，如果某个任务延时结束了，那么就切换到这个任务去运行。如果找不到一个任务延时结束，那么就维持当前任务运行。

在这里，我们实现了两个用户任务，一个空闲任务。那么，SysTick 每发起一次中断就会调用本函数，检查的步骤为：

- 如果当前任务为空闲任务，那么检查任务 1 和任务 2 是否延时结束。如果任务 1 延时结束，那么就运行任务 1 （OSTCBHighRdyPtr 指向任务 1 的 TCB）；如果任务 2 延时结束，那么就运行任务 2 （OSTCBHighRdyPtr 指向任务 2 的 TCB）。
- 如果当前任务为任务 1，那么检查任务 1 自己和任务 2 是否延时结束。如果任务 2 延时结束，那么就运行任务 2 （OSTCBHighRdyPtr 指向任务 2 的 TCB）；如果任务 1 自己未延时结束，那么就运行空闲任务（OSTCBHighRdyPtr 指向空闲任务的 TCB）。
- 如果当前任务为任务 2，那么检查任务 2 自己和任务 1 是否延时结束。如果任务 1 延时结束，那么就运行任务 1 （OSTCBHighRdyPtr 指向任务 1 的 TCB）；如果任务 2 自己未延时结束，那么就运行空闲任务（OSTCBHighRdyPtr 指向空闲任务的 TCB）。
- 最后，触发 PendSV 异常，保存上文，使 OSTCBCurPtr 获得 OSTCBHighRdyPtr，切换下文。

以上，就是我们实现的简单的调度算法。
```c
/* 任务调度 */
void OSSched (void)
{
	if ( OSTCBCurPtr == &OSIdleTaskTCB )	/* (a) 假如现在的任务是空闲任务 */
	{
		if ( OSRdyList[0].HeadPtr->TaskDelayTicks == 0 )	/* 如果任务 1 延时结束 */
		{
			OSTCBHighRdyPtr = OSRdyList[0].HeadPtr;
		}
		else if ( OSRdyList[1].HeadPtr->TaskDelayTicks == 0 )	/* 如果任务 2 延时结束 */
		{
			OSTCBHighRdyPtr = OSRdyList[1].HeadPtr;
		}
		else
		{
			return;
		}
	}
	else if ( OSTCBCurPtr == OSRdyList[0].HeadPtr )	/* (b) 假如现在的任务是任务 1 */
	{
		if ( OSRdyList[1].HeadPtr->TaskDelayTicks == 0 )	/* 如果任务 2 延时结束 */
		{
			OSTCBHighRdyPtr = OSRdyList[1].HeadPtr;
		}
		else if ( OSTCBCurPtr->TaskDelayTicks != 0 )	/* 如果任务 1 自己还在延时 */
		{
			OSTCBHighRdyPtr = &OSIdleTaskTCB;
		}
		else
		{
			return;
		}
	}
	else if ( OSTCBCurPtr == OSRdyList[1].HeadPtr )	/* (c) 假如现在的任务是任务 2 */
	{
		if ( OSRdyList[0].HeadPtr->TaskDelayTicks == 0 )	/* 如果任务 1 延时结束 */
		{
			OSTCBHighRdyPtr = OSRdyList[0].HeadPtr;
		}
		else if ( OSTCBCurPtr->TaskDelayTicks != 0 )	/* 如果任务 2 自己还在延时 */
		{
			OSTCBHighRdyPtr = &OSIdleTaskTCB;
		}
		else
		{
			return;
		}
	}
	
	OS_TASK_SW();
}
```

最后一个困惑：为什么要将 TCB 指针赋值给 OSTCBHighRdyPtr 而不是 OSTCBCurPtr 呢？我个人理解：因为 uCOS 是抢占式多任务，将任务 A 赋值给最高优先级时，当前任务会被切换为该任务 A。而且可以看看 PendSV 异常汇编程序，你会发现它实现的最核心的东西其实是 OSTCBCurPtr = OSTCBHighRdyPtr。我现在不是很懂，也许学到后面会逐渐明白的吧。

## 3 实验：阻塞延时的运用
### 3.1 主函数 (app.c)

在任务中，实现了阻塞延时，去掉了软件延时和手动切换任务。

```c
#include "ARMCM3.h"
#include "os.h"

#define  TASK1_STK_SIZE       20
#define  TASK2_STK_SIZE       20

static   CPU_STK   Task1Stk[TASK1_STK_SIZE];
static   CPU_STK   Task2Stk[TASK2_STK_SIZE];

static   OS_TCB    Task1TCB;
static   OS_TCB    Task2TCB;

uint32_t flag1;
uint32_t flag2;

void Task1 (void *p_arg);
void Task2 (void *p_arg);
void delay(uint32_t count);

int main (void)
{
	OS_ERR err;
	
	/* 初始化相关的全局变量，创建空闲任务 */
	OSInit(&err);
	
	/* 关中断，因为此时 OS 未启动，若开启中断，那么 SysTick 将会引发中断 */
	CPU_IntDis();
	
	/* 初始化 SysTick，配置 SysTick 为 10ms 中断一次，Tick = 10ms */
	OS_CPU_SysTickInit(10);
	
	/* 创建任务 */
	OSTaskCreate ((OS_TCB*)      &Task1TCB, 
	              (OS_TASK_PTR) Task1, 
	              (void *)       0,
	              (CPU_STK*)     &Task1Stk[0],
	              (CPU_STK_SIZE) TASK1_STK_SIZE,
	              (OS_ERR *)     &err);

	OSTaskCreate ((OS_TCB*)      &Task2TCB, 
	              (OS_TASK_PTR) Task2, 
	              (void *)       0,
	              (CPU_STK*)     &Task2Stk[0],
	              (CPU_STK_SIZE) TASK2_STK_SIZE,
	              (OS_ERR *)     &err);
				  
	/* 将任务加入到就绪列表 */
	OSRdyList[0].HeadPtr = &Task1TCB;
	OSRdyList[1].HeadPtr = &Task2TCB;
	
	/* 启动OS，将不再返回 */				
	OSStart(&err);
}

void Task1 (void *p_arg)
{
	for (;;)
	{
		flag1 = 1;
		OSTimeDly (2);	// 20ms
		flag1 = 0;
		OSTimeDly (2);
	}
	// 不用手动任务切换
}

void Task2 (void *p_arg)
{
	for (;;)
	{
		flag2 = 1;
		OSTimeDly (2);		
		flag2 = 0;
		OSTimeDly (2);
	}
	// 不用手动任务切换
}
```

**（1）初始化流程如下：**
- OS 初始化：完成就绪列表和空闲任务的初始化。
- 关闭中断。
- SysTick 初始化：设置一个滴答为 10ms。
- 创建任务。
- 将任务加入就绪列表。
- OS 启动：启动任务切换，先运行就绪列表里的第一个任务 Task1。

**（2）在第一个任务 Task1 中，执行 Flag1 = 1 后，执行到 OSTimeDly：**
- 将当前任务 TCB 的记录延时成员设置好。
- 由 OSSched 进行任务调度。
- 因为此时没有别的任务运行，所以会被切换到 Task2。

**（3）在 Task2 中，执行 Flag2 = 1 后，也执行到 OSTimeDly：**
- 将当前任务 TCB 的记录延时成员设置好。
- 由 OSSched 进行任务调度。
- 因为此时 Task1 仍处于阻塞状态，所以会被切换到空闲任务。

**（4）在运行空闲任务的同时，SysTick 也在工作中：**
- 发现一个 Tick 到来了，说明 10ms 间隔已到，发起中断 SysTick_Handler。
- 执行函数 OSTimeTick，将所有任务 TCB 中的延时时长全部减 1，表示已过去了 10ms。
- 发起 OSSched 进行任务调度。
- 因为这时两个任务仍在阻塞中，所以继续切换到空闲任务。
- 这样，每发起一次 Tick，都会减去 1，同时去检查两个任务是否还在阻塞中，CPU 一直运行空闲任务。直到，当两个任务同时阻塞完毕的时候（因为我们设置的两个任务阻塞时间相同），又是一次 Tick 的到来，此时空闲任务将切换到 Task1（因为任务调度函数中判断 Task1 是否阻塞是写在前面的）。此时你发现又回到了第（2）个步骤，只不过这一次是 Flag1 = 0，后面的运行过程就不多说了。

### 3.2 实验现象

【图片】

可以发现，因为我们延时的长度一样，所以两个任务几乎是同时进行的，按照上面的分析，确实就是这个样子，看起来就像是并行线程，很神奇吧！
