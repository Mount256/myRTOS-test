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

任务自行发起的延时，会使得任务自身发生阻塞，不再运行下去。这种延时就叫阻塞延时。而在阻塞延时期间，我们不让 CPU 闲着干等着，所以就有了任务切换。如果没有别的任务可以切换，那就切换到内核的空闲任务。无论怎样，只要别让 CPU 闲着就行了。

### 2.1 数据类型定义

在 TCB 中加入了记录延时长度的成员，单位为 1 个 Tick，之前在上一篇笔记中我们初始化一个 Tick 为 10ms。

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


