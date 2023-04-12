参考内容：《[野火]uCOS-III内核实现与应用开发实战指南——基于STM32》第 12 章。

[toc]

本篇内容主要是对过往函数的一些修改。

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
PendSV_Handler
	
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
