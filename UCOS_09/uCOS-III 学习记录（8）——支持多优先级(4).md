参考内容：《[野火]uCOS-III内核实现与应用开发实战指南——基于STM32》第 12 章。

[toc]

本篇内容主要是对过往函数的一些修改，因此，一些细节将不会赘述。

## 0 数据类型定义和宏定义
### 0.1 临界段宏定义（os.h）

```c
#define  OS_CRITICAL_ENTER()                    CPU_CRITICAL_ENTER()
#define  OS_CRITICAL_ENTER_CPU_CRITICAL_EXIT()
#define  OS_CRITICAL_EXIT()                     CPU_CRITICAL_EXIT()
#define  OS_CRITICAL_EXIT_NO_SCHED()            CPU_CRITICAL_EXIT()
```

### 0.2 任务控制块 TCB 定义（os.h）

类型定义：
```c
/*----------------------TCB---------------------------*/
/* TCB 重命名为大写字母格式 */
typedef struct os_tcb	OS_TCB;

/* TCB 数据类型声明 */
struct os_tcb{
	CPU_STK			*StkPtr;
	CPU_STK_SIZE	StkSize;
	
	OS_TICK			TaskDelayTicks;		/* 任务延时周期个数 */
	
	OS_PRIO			Prio;				/* 任务优先级（8位整型，最多支持255个优先级） */
	
	OS_TCB			*NextPtr;			/* 就绪列表双向链表的下一个指针 */
	OS_TCB			*PrevPtr;			/* 就绪列表双向链表的前一个指针 */
};
```

全局变量定义：
```c
OS_EXT 	OS_TCB			*OSTCBCurPtr;
OS_EXT	OS_TCB			*OSTCBHighRdyPtr;
```

### 0.3 任务就绪列表定义（os.h）

类型定义：
```c
/*---------------------OS_RDY_LIST----------------------------*/
/* 就绪列表重命名为大写字母格式 */
typedef struct os_rdy_list	OS_RDY_LIST;

/* 就绪列表数据类型声明，将 TCB 串成双向链表 */
struct os_rdy_list{
	OS_TCB		*HeadPtr;
	OS_TCB		*TailPtr;
	OS_OBJ_QTY	NbrEntries;		/* 同一个索引下有多少个任务 */
};
```

全局变量定义：
```c
OS_EXT	OS_RDY_LIST		OSRdyList[OS_CFG_PRIO_MAX];
```

### 0.4 优先级相关变量定义（os.h）

```c
OS_EXT	OS_PRIO			OSPrioCur;		/* 当前优先级 */
OS_EXT	OS_PRIO			OSPrioHighRdy;	/* 最高优先级 */
```

宏定义：
```c
#define  OS_PRIO_INIT           (OS_CFG_PRIO_MAX)
```

## 1 系统初始化 OSInit()（os_core.c） 

该函数用于系统的初始化，说白了就是初始化各种全局变量的地方。该函数完成的工作有：
- 初始化 TCB 相关的全局变量。
- 初始化优先级相关的全局变量。
- 初始化优先级表。
- 初始化就绪列表。
- 初始化空闲任务。

```c
/* OS 系统初始化，用于初始化全局变量 */
void OSInit (OS_ERR *p_err)
{
	/* 系统用一个全局变量 OSRunning 来指示系统的运行状态。系统初始化时，默认为停止状态，即 OS_STATE_OS_STOPPED */
	OSRunning = OS_STATE_OS_STOPPED;
	
	OSTCBCurPtr 	= (OS_TCB *) 0; /* 指向当前正在运行的任务的 TCB 指针 */
	OSTCBHighRdyPtr = (OS_TCB *) 0; /* 指向就绪任务中优先级最高的任务的 TCB */
	
	OSPrioCur 		= (OS_PRIO)0;	/* 初始化当前优先级 */
	OSPrioHighRdy 	= (OS_PRIO)0;	/* 初始化最高优先级 */
	
	OS_PrioInit();		/* 初始化优先级表 */
	
	OS_RdyListInit();  /* 初始化就绪列表 */
	
	OS_IdleTaskInit(p_err); /* 初始化空闲任务 */
	
	if (*p_err != OS_ERR_NONE) {
		return;
	}
}
```

## 2 任务创建函数 OSTaskCreate()（os_task.c）

该函数完成的工作有：
- 初始化任务 TCB，将 TCB 初始化为默认值。
- 初始化任务栈。
- 在任务 TCB 中记录任务的优先级。
- 在任务 TCB 中记录栈顶指针。
- 在任务 TCB 中记录栈的大小。
- 在优先级表中将对应的优先级位置置 1。
- 将任务 TCB 加入就绪列表中。即：将任务 TCB 放到 OSRdyList[优先级] 中，如果同一个优先级有多个任务，那么这些任务的 TCB 就会被放到 OSRdyList[优先级] 串成一个双向链表。

注意：
- 以上工作位于临界段内。

```c
/* 任务创建函数 */
void OSTaskCreate( 	OS_TCB 			*p_tcb,  		/* TCB指针 */
					OS_TASK_PTR 	p_task,  		/* 任务函数名 */
					void 			*p_arg,  		/* 任务的形参 */
					OS_PRIO			prio,			/* 任务优先级 */
					CPU_STK 		*p_stk_base, 	/* 任务栈的起始地址 */
					CPU_STK_SIZE 	stk_size,		/* 任务栈大小 */
					OS_ERR 			*p_err )		/* 错误码 */
{
	CPU_STK		*p_sp;
	CPU_SR_ALLOC();
	
	OS_TaskInitTCB (p_tcb);
	
	p_sp = OSTaskStkInit ( 	p_task,
							p_arg,
							p_stk_base,
							stk_size );  /* 任务栈初始化函数 */
	p_tcb->Prio		= prio;		/* 任务优先级保存在 TCB 的 prio 中*/
	p_tcb->StkPtr 	= p_sp;    	/* 剩余栈的栈顶指针 p_sp 保存到任务控制块 TCB 的 StkPtr 中 */
	p_tcb->StkSize 	= stk_size; /* 将任务栈的大小保存到任务控制块 TCB 的成员 StkSize 中 */
	
	OS_CRITICAL_ENTER();	/* 进入临界段 */
	
	/* 将任务添加到就绪列表 */
	OS_PrioInsert (p_tcb->Prio);
	OS_RdyListInsertTail (p_tcb);
	
	OS_CRITICAL_EXIT();		/* 退出临界段 */
	
	*p_err = OS_ERR_NONE;		/* 函数执行到这里表示没有错误 */
}
```

### 2.1 初始化任务控制块 OS_TaskInitTCB()（os_task.c）

该函数完成的工作是将任务 TCB 的每一个成员都赋值为默认值。其中 OS\_PRIO\_INIT 是任务 TCB 初始化的时候给的默认的一个优先级，
宏展开等于 OS\_CFG\_PRIO\_MAX，这是一个不会被 OS 使用到的优先级。

```c
/* 初始化任务 TCB */
void OS_TaskInitTCB (OS_TCB *p_tcb)
{
	p_tcb->StkPtr 			= (CPU_STK   *)	0;
	p_tcb->StkSize 			= (CPU_STK_SIZE)0;
	
	p_tcb->TaskDelayTicks 	= (OS_TICK    )	0;		
	
	p_tcb->Prio 			= (OS_PRIO    )	OS_PRIO_INIT;				
	
	p_tcb->NextPtr 			= (OS_TCB    *)	0;			
	p_tcb->PrevPtr 			= (OS_TCB    *)	0;			
}
```

## 3 空闲任务初始化 OS_IdleTaskInit()（os_core.c）

该函数为空闲任务创建了一个 TCB，并初始化了空闲任务的栈。注意，空闲任务的优先级是最低的，等于 (OS\_CFG\_PRIO\_MAX - 1u)，这意味着在系统没有任何用户任务运行的情况下，空闲任务就会被运行。

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
	              (OS_ERR *)     &p_err);		/* 创建空闲任务 */
}
```

## 4 系统启动 OSStart()（os_core.c）

该函数完成的工作有：
- 找到优先级表的最高优先级，并将其赋值给 OSPrioHighRdy，再赋值给 OSPrioCur。
- 根据最高优先级，找到对应的任务 TCB 链表，将其头指针赋值给 OSTCBHighRdyPtr，再赋值给 OSTCBCurPtr。
- 启动任务切换。

```c
/* 系统启动函数 */
void OSStart (OS_ERR *p_err)
{
	if ( OSRunning == OS_STATE_OS_STOPPED )
	{
		OSPrioHighRdy = OS_PrioGetHighest();
		OSPrioCur = OSPrioHighRdy;
		
		OSTCBHighRdyPtr = OSRdyList[OSPrioHighRdy].HeadPtr; 
		OSTCBCurPtr = OSTCBHighRdyPtr;
		
		OSStartHighRdy(); 						/* 启动任务切换，不会返回 */
		*p_err = OS_ERR_FATAL_RETURN;			/* 运行至此处，说明发生了致命错误 */
	}
	else{
		*p_err = OS_STATE_OS_RUNNING;
	}
}
```

## 5 可悬起系统调用中断服务程序 PendSV_Handler()（os_cpu_a.s）

该函数完成的工作有：
- 保存旧任务的寄存器状态。
- 使 OSPrioCur = OSPrioHighRdy。
- 使 OSTCBCurPtr = OSTCBHighRdyPtr。
- 恢复新任务的寄存器状态。


```
;**********PendSVHandler异常**********
PendSV_Handler PROC
	
	CPSID	I					; 关中断，防止上下文切换

	MRS		R0, PSP				; 将 PSP 加载到 R0，MRS 是 ARM 32 位数据加载指令，
								; 功能是加载特殊功能寄存器的值到通用寄存器
	CBZ		R0, OS_CPU_PendSVHandler_nosave ; 判断 R0，如果值为 0 则跳转到 OS_CPU_PendSVHandler_nosave
	                                        ; 进行第一次任务切换的时候，R0 肯定为 0
	
	STMDB	R0!, {R4-R11}		; 手动存储 R4-R11 寄存器到当前任务栈中，而其他寄存器会被 CPU 自动入栈
	LDR		R1, = OSTCBCurPtr	; 将 OSTCBCurPtr 指针的地址加载到 R1
	LDR		R1, [R1]			; 将 OSTCBCurPtr 指针加载到 R1
	STR		R0, [R1]			; 存储 R0（任务栈栈顶）的值到 OSTCBCurPtr(->StkPtr) 

OS_CPU_PendSVHandler_nosave

	; 使 OSPrioCur = OSPrioHighRdy
	LDR		R0, = OSPrioCur		; 将 OSPrioCur 指针的地址加载到 R0
	LDR		R1, = OSPrioHighRdy	; 将 OSPrioHighRdy 指针的地址加载到 R1
	LDR		R2, [R1]			; 将 OSPrioCur 指针加载到 R2
	STR		R2, [R0]			; 将 OSPrioHighRdy（R2）存到 OSPrioCur（R0）

	; 使 OSTCBCurPtr = OSTCBHighRdyPtr 
	LDR		R0, = OSTCBCurPtr		; 将 OSTCBCurPtr 指针的地址加载到 R0
	LDR		R1, = OSTCBHighRdyPtr	; 将 OSTCBHighRdyPtr 指针的地址加载到 R1
	LDR		R2, [R1]			; 将 OSTCBCurPtr 指针加载到 R2
	STR		R2, [R0]			; 将 OSTCBHighRdyPtr（R2）存到 OSTCBCurPtr（R0）
	
	LDR     R0, [R2]            ; 加载 OSTCBHighRdyPtr(->StkPtr) 到 R0
	LDMIA   R0!, {R4-R11}       ; 加载需要手动保存的信息到 CPU 寄存器 R4-R11，其他寄存器将在返回后由 CPU 自动装载
	
	MSR     PSP, R0             ; 更新PSP的值，这个时候PSP指向下一个要执行的任务的堆栈的栈底（这个栈底已经加上刚刚手动加载到CPU寄存器R4-R11的偏移）
	ORR     LR, LR, #0x04       ; 确保异常返回使用的堆栈指针是PSP，即LR寄存器的位2要为1
	CPSIE   I                   ; 开中断
	BX      LR                  ; 异常返回，这个时候任务堆栈中的剩下内容将会自动加载到xPSR，PC（任务入口地址），R14，R12，R3，R2，R1，R0（任务的形参）
	                            ; 同时PSP的值也将更新，即指向任务堆栈的栈顶。在STM32中，堆栈是由高地址向低地址生长的。
	
	NOP                         ; 为了汇编指令对齐，不然会有警告
	
	
	ENDP   
```

## 6 阻塞延时 OSTimeDly()（os_time.c）

任务调用 OSTimeDly() 函数之后，任务就处于阻塞态，需要将任务从就绪列表中移除（此处未实现）。因此需要完成的工作有：
- 任务 TCB 记录好延时时间。
- 在优先级表中清除相应的位，达到任务不处于就绪态的目的。

注意：
- 以上工作位于临界段内。

```c
/* 阻塞延时 */
void OSTimeDly (OS_TICK dly)
{
	CPU_SR_ALLOC();
	OS_CRITICAL_ENTER();	/* 进入临界段 */
	
	/* 延时时间 */
	OSTCBCurPtr->TaskDelayTicks = dly;
	OS_PrioRemove (OSTCBCurPtr->Prio);
	
	OS_CRITICAL_EXIT();		/* 退出临界段 */
	
	/* 任务切换 */
	OSSched();
}
```

## 7 任务切换 OSSched()（os_core.c）

任务调度函数根据优先级进行调度。具体完成的工作如下：
- 查找最高优先级。
- 如果找到的最高优先级是当前任务，则直接返回，不进行任务切换，否则进行任务切换。

注意：
- 以上工作位于临界段内。

```c
/* 任务调度 */
void OSSched (void)
{	
	CPU_SR_ALLOC();
	OS_CRITICAL_ENTER();	/* 进入临界段 */
	
	OSPrioHighRdy = OS_PrioGetHighest();	
	OSTCBHighRdyPtr = OSRdyList[OSPrioHighRdy].HeadPtr; 
	
	/* 如果最高优先级的任务是当前任务则直接返回，不进行任务切换 */
	if (OSTCBHighRdyPtr == OSTCBCurPtr)	
	{
		OS_CRITICAL_EXIT();	/* 退出临界段 */
		return;
	}
	
	OS_CRITICAL_EXIT();		/* 退出临界段 */
	OS_TASK_SW();			/* 任务切换   */
}
```

## 8 SysTick 发起中断后调用 OSTimeTick()（os_time.c）

该函数完成的工作有：
- 遍历整个就绪列表，发现有任务在延时，将其延时时间减一。
- 如果减一后发现任务已经延时结束了，将任务从阻塞态变为就绪态，即在优先级表中的相应位置置位。

注意：
- 以上工作位于临界段内。

```c
void OSTimeTick (void)
{
	OS_PRIO i;
	
	CPU_SR_ALLOC();
	OS_CRITICAL_ENTER();	/* 进入临界段 */
	
	/* 遍历整个就绪列表 */
	for ( i = 0u; i < OS_CFG_PRIO_MAX; i++)
	{
		if ( OSRdyList[i].HeadPtr->TaskDelayTicks > 0u )	/* 如果延时未到时，则减 1 */
		{
			OSRdyList[i].HeadPtr->TaskDelayTicks --;
			if (OSRdyList[i].HeadPtr->TaskDelayTicks == 0u)	/* 如果延时时间已到，让任务就绪 */
			{
				OS_PrioInsert (i);
			}
		}
	}
	
	OS_CRITICAL_EXIT();		/* 退出临界段 */
	
	/* 任务调度 */
	OSSched();  
}
```

## 9 将之前所添加的内容进行运用
### 9.1 主函数 main()（app.c）

在 app.c 中，我们添加了 3 个任务。注意：
- 要将任务栈的大小设置得大些，不然可能不够用。
- 3 个任务的优先级分别是 1、2、3，空闲任务占据了最后一个优先级，0 优先级不能使用。

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
	              (OS_ERR *)     &err);

	OSTaskCreate ((OS_TCB*)      &Task2TCB, 
	              (OS_TASK_PTR)  Task2, 
	              (void *)       0,
				  (OS_PRIO)		 2,
	              (CPU_STK*)     &Task2Stk[0],
	              (CPU_STK_SIZE) TASK2_STK_SIZE,
	              (OS_ERR *)     &err);
				  
	OSTaskCreate ((OS_TCB*)      &Task3TCB, 
	              (OS_TASK_PTR)  Task3, 
	              (void *)       0,
				  (OS_PRIO)		 3,
	              (CPU_STK*)     &Task3Stk[0],
	              (CPU_STK_SIZE) TASK3_STK_SIZE,
	              (OS_ERR *)     &err);
	
	/* 启动OS，将不再返回 */				
	OSStart(&err);
}

void Task1 (void *p_arg)
{
	for (;;)
	{
		flag1 = 1;
		OSTimeDly (2);	
		flag1 = 0;
		OSTimeDly (2);
	}
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

现在我们来复原一下整个运行过程。

### 9.2 运行过程
#### 9.2.1 在主函数中

- 系统初始化：初始化各种全局变量，初始化优先级表，初始化就绪列表，初始化空闲任务（包括初始化空闲任务栈和空闲任务 TCB）。
- CPU 初始化：暂为空。
- 关中断：因为此时 OS 未启动，若开启中断，那么 SysTick 将会引发中断，打断初始化流程。
- 初始化 SysTick：配置 SysTick 为 10ms 中断一次，Tick = 10ms。
- 创建任务：包括创建任务栈和任务 TCB，以及将 TCB 插入到就绪列表中，在优先级表对应位置置位。
- 启动系统：先找到最高优先级，然后开始运行最高优先级对应的任务，启动第一次任务切换（此时将完成最后的初始化流程，即有关 PendSV 的中断优先级配置，接着触发 PendSV 异常，发起任务切换），将 CPU 占有权交给任务。

#### 9.2.2 在 Task1 中

- 执行到阻塞函数 OSTimeDly：在 Task1 的 TCB 中记录好延时时间（为 2），同时在优先级表中的相应位置（即优先级 1 的位置）清零。最后启动任务调度。
- 执行任务调度 OSSched：任务调度器先找到最高优先级，然后再找到最高优先级的任务 TCB。如果发现该任务就是当前任务，则不进行任务切换。在本案例中发现最高优先级为 2，对应任务是 Task2，不是当前任务，则发起任务切换（发起 PendSV 异常）。
- PendSV 异常处理程序：保存 Task1 的状态，加载 Task2 的状态，更新全局变量的值。

#### 9.2.3 在 Task2 中

- 执行到阻塞函数 OSTimeDly：在 Task2 的 TCB 中记录好延时时间（为 2），同时在优先级表中的相应位置（即优先级 2 的位置）清零。最后启动任务调度。
- 执行任务调度 OSSched：任务调度器先找到最高优先级，然后再找到最高优先级的任务 TCB。如果发现该任务就是当前任务，则不进行任务切换。在本案例中发现最高优先级为 3，对应任务是 Task3，不是当前任务，则发起任务切换（发起 PendSV 异常）。
- PendSV 异常处理程序：保存 Task2 的状态，加载 Task3 的状态，更新全局变量的值。

#### 9.2.4 在 Task3 中

- 执行到阻塞函数 OSTimeDly：在 Task3 的 TCB 中记录好延时时间（为 2），同时在优先级表中的相应位置（即优先级 31 的位置）清零。最后启动任务调度。
- 执行任务调度 OSSched：任务调度器先找到最高优先级，然后再找到最高优先级的任务 TCB。如果发现该任务就是当前任务，则不进行任务切换。在本案例中发现最高优先级为 31，对应任务是 空闲任务，不是当前任务，则发起任务切换（发起 PendSV 异常）。
- PendSV 异常处理程序：保存 Task3 的状态，加载空闲任务的状态，更新全局变量的值。

请注意，每次任务切换的时间其实非常短，大约只有不到 0.1ms 的长度，不会对 2ms 的延时造成很大的影响。

#### 9.2.5 在空闲任务中 SysTick 发起中断

- 执行 OSTimeTick：遍历整个就绪列表，发现各个任务的延时未到时（此时为 2），全部减一；减一后（此时为 1）发现延时也未到时，说明任务还未到进入就绪态的时候。最后发起任务调度，发现不用进行任务切换，空闲任务继续运行。
- 再次发起中断，执行 OSTimeTick：遍历整个就绪列表，发现各个任务的延时未到时（此时为 1），全部减一；减一后（此时为 0）发现延时已到时，说明任务进入就绪态了。将 Task1、Task2、Task3 的优先级在优先级表中的相应位置重新置位。

### 9.3 实验现象

