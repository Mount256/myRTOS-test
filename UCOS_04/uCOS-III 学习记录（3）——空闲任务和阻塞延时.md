参考内容：《[野火]uCOS-III内核实现与应用开发实战指南——基于STM32》第 8 章。

[toc]

## 1. 空闲任务

不知道有没有注意到这样一个问题：我在学习 x86 汇编语言的时候，曾详细研读过系统内核的代码，内核本身也是有自己的 TCB 的，内核可作为一个“任务管理器”来对其他任务进行创建、删除等操作。那么 uCOS 作为一个实时操作系统，内核也应该需要自己的任务和 TCB，因此我们需要创建一个空闲任务给内核，这样，在没有任务的时候，内核空转运行空闲任务，同时检查各个任务的状态以做出相应的操作。

### 1.1 数据类型定义

空闲任务虽然与其他任务的作用不同，但它的本质依然是一个任务，TCB、任务栈一个都不能少。

#### 1.1.1 空闲任务 TCB (os.h)

在 os.h 中定义 TCB：
```c
OS_EXT  OS_TCB			OSIdleTaskTCB;
```

#### 1.1.2 空闲任务栈 (os\_cfg\_app.c)

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
- 计数清零：OSIdleTaskCtr 清零，为无聊的计数做准备。

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


