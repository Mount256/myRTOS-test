参考内容：《[野火]uCOS-III内核实现与应用开发实战指南——基于STM32》第 6 章。

[toc]

## 前排提醒

- 每一节标题最后的括号是表明该数据类型或函数位于哪个文件中。
- 按照 μC/OS-III 中的函数命名规则，以大小的 OS 开头，表示这是一个外部函数，可以由用户调用，以 OS_ 开头的函数表示内部函数，只能由 μC/OS-III 内部使用。紧接着是文件名，表示该函数放在哪个文件，最后是函数功能名称。例如：OSTaskCreate，说明允许用户调用，位于 os_task.c 文件中。
- extern 的巧妙定义 (os.h)：
```c
#ifdef 	OS_GLOBALS
	#define OS_EXT
#else
	#define OS_EXT	extern
#endif
```
- 在 cpu.h 和 os_type.h 中：
```c
#ifndef CPU_H
#define CPU_H

typedef unsigned short  CPU_INT16U;
typedef unsigned int  	CPU_INT32U;
typedef unsigned char 	CPU_INT08U;

typedef CPU_INT32U  	CPU_ADDR;

/* 堆栈数据类型重定义 */
typedef CPU_INT32U		CPU_STK;
typedef CPU_ADDR		CPU_STK_SIZE;

typedef volatile CPU_INT32U		CPU_REG32;

#endif /* CPU_H */

/************************************************/

#ifndef OS_TYPE_H
#define OS_TYPE_H

#include "cpu.h"

typedef   CPU_INT16U      OS_OBJ_QTY;
typedef   CPU_INT08U      OS_PRIO;
typedef   CPU_INT08U      OS_STATE;

#endif  /* OS_TYPE_H */
```

## 0 数据类型声明
### 0.1 任务控制块（OS_TCB）(os.h)

- 任务控制块（TCB）：用于记录栈顶指针和栈的大小。
```c
/* TCB 重命名为大写字母格式 */
typedef struct os_tcb	OS_TCB;

/* TCB 数据类型声明 */
struct os_tcb{
	CPU_STK			*StkPtr;
	CPU_STK_SIZE	StkSize;
};
```

- OSTCBCurPtr：全局变量定义，用于记录当前正在运行的任务。
```c
OS_EXT 	OS_TCB			*OSTCBCurPtr;
```

### 0.2 就绪列表（OS_RDY_LIST）(os.h)

- 就绪列表：存储任务 TCB 的一个双向链表。为了使同一个优先级支持多个任务，uCOS 使用头尾指针来将 TCB 串成一个双向链表。目前只用到头指针，用来指向任务的 TCB。
```c
/* 就绪列表重命名为大写字母格式 */
typedef struct os_rdy_list	OS_RDY_LIST;

/* 就绪列表数据类型声明，将 TCB 串成双向链表 */
struct os_rdy_list{
	OS_TCB		*HeadPtr;
	OS_TCB		*TailPtr;
};

/* 任务函数名 */
typedef void (*OS_TASK_PTR)(void *p_arg);
```

- OSTCBHighRdyPtr：全局变量定义，指向就绪任务中优先级最高的任务的 TCB。
```c
OS_EXT	OS_TCB			*OSTCBHighRdyPtr;
```

- OSRdyList：全局变量定义，把任务 TCB 指针放到 OSRdyList 数组中。即，使用这个数组，可将各个任务 TCB 组织起来。数组的下标表示任务的优先级。目前用不到这个优先级。
- OS\_CFG\_PRIO\_MAX：宏定义，表示系统支持多少个优先级，目前这里仅用来表示这个就绪列表可以存多少个任务的 TCB 指针。
```c
OS_EXT	OS_RDY_LIST		OSRdyList[OS_CFG_PRIO_MAX];

其中在 os_cfg.h :
/* 支持最大的优先级 */
#define OS_CFG_PRIO_MAX		32u
```

### 0.3 系统状态 (OSRunning) (os.h)

OSRunning: 全局变量定义，用于指示系统运行状态
```c
OS_EXT 	OS_STATE		OSRunning;
```

目前我们有两个任务状态：
```c
/* 任务状态 */
#define  OS_STATE_OS_STOPPED                    (OS_STATE)(0u)
#define  OS_STATE_OS_RUNNING                    (OS_STATE)(1u)
```

## 1 任务的创建
### 1.1 任务创建函数 OSTaskCreate() (os_task.c)

任务创建函数需要完成的三件事：
- 创建任务栈：调用函数 OSTaskStkInit()。
- 填写 TCB：剩余栈栈顶指针 SP 存入 TCB 的第一个成员 StkPtr。
- 填写 TCB：将任务栈的大小存入 TCB 的第二个成员 StkSize。

```c
/* 任务创建函数 */
void OSTaskCreate( 	OS_TCB 			*p_tcb,  		/* TCB指针 */
					OS_TASK_PTR 	p_task,  		/* 任务函数名 */
					void 			*p_arg,  		/* 任务的形参 */
					CPU_STK 		*p_stk_base, 	/* 任务栈的起始地址 */
					CPU_STK_SIZE 	stk_size,		/* 任务栈大小 */
					OS_ERR 			*p_err )		/* 错误码 */
{
	CPU_STK		*p_sp;
	
	p_sp = OSTaskStkInit ( 	p_task,
							p_arg,
							p_stk_base,
							stk_size );  /* 任务栈初始化函数 */
	p_tcb->StkPtr 	= p_sp;    	/* 剩余栈的栈顶指针 p_sp 保存到任务控制块 TCB 的第一个成员 StkPtr 中 */
	p_tcb->StkSize 	= stk_size; /* 将任务栈的大小保存到任务控制块 TCB 的成员 StkSize 中 */
	
	*p_err = OS_ERR_NONE;		/* 函数执行到这里表示没有错误 */
}
```
接下来解释函数 OSTaskStkInit。

#### 1.1.1 任务栈创建函数 OSTaskStkInit() (os_cpu_c.c)

任务栈用来存储各寄存器的状态，以及其他中间数据。

注意：
- 由于是使用 PendSV 中断发起任务切换，因此 CPU 在切换新的任务前，会自动将新任务的任务栈按顺序出栈写入到寄存器中，这个顺序为：R0、R1、R2、R3、R12、R14(LR)、R15(PC)、XPSR。所以，CPU 会按这个顺序将寄存器压入旧任务的任务栈中：XPSR、R15(PC)、R14(LR)、R12、R3、R2、R1、R0。
- 由以上讨论可知：1. 部分寄存器仍没有压入栈中，需要我们自己在程序中手动压入；2. 我们在写程序的时候，入栈和出栈的顺序是严格确定好的，要按照硬件要求去写，不能改变。
- 像 0x14141414u 这些数是方便我们调试的，说白了就是，这样写，我们就容易知道这个位置是 R14 的，不是别的寄存器的。这些数字除了方便我们看之外，没有任何意义，你也可以全部初始化为零，或别的数字。
- R13 哪去了？R13 是栈指针寄存器（PSP），当然不能压入栈了，任务运行时要用到，一个随时改变的值是没必要压入的。不过，你可以将 p_stk 视为 R13。

函数完成的事情：
- 初始化任务栈，先在任务栈中为寄存器预留栈空间，再返回分配好栈空间后的栈指针。

```c
/* 任务栈初始化函数 */
CPU_STK *OSTaskStkInit ( OS_TASK_PTR 	p_task,  		/* 任务名，指示着任务的入口地址 */
						 void			*p_arg,  		/* 任务的形参 */
						 CPU_STK		*p_stk_base, 	/* 任务栈的起始地址 */
						 CPU_STK_SIZE	stk_size ) 		/* 任务栈的大小 */
{
	CPU_STK		*p_stk;
	
	p_stk = &p_stk_base[stk_size];		/* 获取任务栈的栈顶地址 */
	
	/* 任务第一次运行时，CPU寄存器需要预设数据 */
	/* 首先是异常发生时自动保存的 8 个寄存器 */
	/* R14、R12、R3、R2 和 R1 为了调试方便，需填入与寄存器号相对应的 16 进制数 */
	*--p_stk = (CPU_STK) 0x01000000u;		/* xPSR 的 bit24 必须置 1 		*/
	*--p_stk = (CPU_STK) p_task;			/* R15(PC) 任务的入口地址 		*/
	*--p_stk = (CPU_STK) 0x14141414u;		/* R14(LR)						*/
	*--p_stk = (CPU_STK) 0x12121212u;		/* R12							*/
	*--p_stk = (CPU_STK) 0x03030303u;		/* R3							*/
	*--p_stk = (CPU_STK) 0x02020202u;		/* R2							*/
	*--p_stk = (CPU_STK) 0x01010101u;		/* R1							*/
	*--p_stk = (CPU_STK) p_arg;				/* R0 : 任务形参  				*/
	/* 剩下的是 8 个需要手动加载到 CPU 寄存器的参数，为了调试方便填入与寄存器号相对应的 16 进制数 */
	*--p_stk = (CPU_STK) 0x11111111u;		/* R11							*/
	*--p_stk = (CPU_STK) 0x10101010u;		/* R10							*/
	*--p_stk = (CPU_STK) 0x09090909u;		/* R9							*/
	*--p_stk = (CPU_STK) 0x08080808u;		/* R8							*/
	*--p_stk = (CPU_STK) 0x07070707u;		/* R7							*/
	*--p_stk = (CPU_STK) 0x06060606u;		/* R6							*/
	*--p_stk = (CPU_STK) 0x05050505u;		/* R5							*/
	*--p_stk = (CPU_STK) 0x04040404u;		/* R4							*/
	
	return p_stk;	/* 此时 p_stk 指向剩余栈的栈顶 */
}
```

如下图所示，即为任务栈的结构：

【图片】

在创建好任务后，可以启动 OS 进行任务调度了。

## 2 内核OS的启动
### 2.1 系统初始化 OSInit() (os_core.c)

不过，先等等，系统初始化应在创建任务前完成。

系统初始化完成的事情：
- 标记系统运行状态：停止状态（因为此时未执行函数 OSSTart() ）。
- 初始化 OSTCBCurPtr：指向当前正在运行的任务的 TCB，因为此时没有任务创建，因此为 0。
- 初始化 OSTCBHighRdyPtr：指向就绪任务中优先级最高的任务的 TCB。因为本章没有使用优先级，因此为 0。

```c
/* OS 系统初始化，用于初始化全局变量 */
void OSInit (OS_ERR *p_err)
{
	/* 系统用一个全局变量 OSRunning 来指示系统的运行状态。系统初始化时，默认为停止状态，即 OS_STATE_OS_STOPPED */
	OSRunning = OS_STATE_OS_STOPPED;
	
	OSTCBCurPtr 	= (OS_TCB *) 0; /* 指向当前正在运行的任务的 TCB 指针 */
	OSTCBHighRdyPtr = (OS_TCB *) 0; /* 指向就绪任务中优先级最高的任务的 TCB */
	
	OSRdyListInit();  /* 初始化就绪列表 */
	
	*p_err = OS_ERR_NONE;  /* 函数执行到这里表示没有错误 */
}
```
注意到有个就绪列表初始化的函数，下面来讲解此函数。

#### 2.1.1 就绪列表初始化函数 OS_RdyListInit() (os_core.c)

初始化完成的事情：
- 遍历整个就绪列表，将各节点的头、尾指针清零。这些指针日后将用来存储 TCB 指针。

注意：
- 此函数不允许用户自己调用。

```c
/* 初始化就绪列表 */
void OS_RdyListInit (void)
{
	OS_PRIO		i;
	OS_RDY_LIST	*p_rdy_list;
	
	for ( i = 0u; i < OS_CFG_PRIO_MAX; i++ )
	{
		p_rdy_list = &OSRdyList[i];
		p_rdy_list->HeadPtr = (OS_TCB *) 0;
		p_rdy_list->TailPtr = (OS_TCB *) 0;
	}
}
```

### 2.2 启动系统内核 OSStart() (os_core.c)

现在所有事情准备完毕：系统内核初始化完毕，任务也创建完毕。即可启动系统 OS，进行任务的切换。

完成的事情：
- 让 OSTCBHighRdyPtr 指向第 1 个任务。由于本文尚未用到优先级，因此最高优先级在这里无意义。
- 启动任务切换函数 OSStartHighRdy()，并且不再返回本函数。

```c
/* 系统启动函数 */
void OSStart (OS_ERR *p_err)
{
	if ( OSRunning == OS_STATE_OS_STOPPED )
	{
		OSTCBHighRdyPtr = OSRdyList[0].HeadPtr; /* 手动配置任务 1 先运行 */
		OSStartHighRdy(); 						/* 启动任务切换，不会返回 */
		*p_err = OS_ERR_FATAL_RETURN;			/* 运行至此处，说明发生了致命错误 */
	}
	else{
		*p_err = OS_STATE_OS_RUNNING;
	}
}
```
下面讲解 OSStartHighRdy。不得不说，个人认为，这是 uCOS 最精彩的部分之一，编写者巧妙地利用中断达到了预期的功能（虽然这也是现代操作系统进行任务切换的常用方式，但依然让我体会到了什么是编程的艺术）。

## 3 任务的切换

### 3.1 任务切换函数 OSStartHighRdy (ARM汇编) (os_cpu_a.s)

> PendSV是可悬起异常，如果我们把它配置最低优先级，那么如果同时有多个异常被触发，它会在其他异常执行完毕后再执行，而且任何异常都可以中断它。

uCOS 使用中断的方式来进行任务切换。在此之前，需要做一些准备。

注意：
- 常量定义时，前面要空格。

完成的事情：
- 配置 PendSV 异常的优先级为最低。CM3 内核支持 256 个优先级，因此最低优先级为 0xFF。在寄存器 SCB_SHPR3 中配置其优先级。
- 因为系统刚启动，还没有任务，设置栈指针 PSP 为 0。
- 触发 PendSV 异常，开中断，进行上下文切换。在寄存器 NVIC\_INT\_CTRL 的位 28 为 PENDSVSET，置位表示 PendSV 异常触发。

```
;**********常量**********
    NVIC_INT_CTRL		EQU 	0xE000ED04		; 中断控制及状态寄存器 SCB_ICSR
    NVIC_SYSPRI14		EQU		0xE000ED22		; 系统优先级寄存器 SCB_SHPR3：bit 16~23
    NVIC_PENDSV_PRI		EQU		0xFF			; PendSV 优先级的值(最低)
    NVIC_PENDSVSET		EQU		0x10000000		; 触发 PendSV 异常的值 Bit28：PENDSVSET

;**********开始进行第一次任务切换**********
OSStartHighRdy

	; 配置 PendSV 的优先级为 0XFF，即最低，防止接下来的 PendSV 中断服务程序进行上下文切换，
	; 即 PendSV 中断服务程序不允许中断
	LDR 	R0, = NVIC_SYSPRI14				; 系统优先级寄存器 SCB_SHPR3：bit 16~23	
	LDR		R1, = NVIC_PENDSV_PRI
	STRB	R1, [R0]

	; 设置 PSP 的值为 0，开始第一个任务切换
	; 在任务中，使用的栈指针都是 PSP，后面如果判断出 PSP 为 0，则表示第一次任务切换
	MOVS	R0, #0		
	MSR		PSP, R0

	; 触发 PendSV 异常，如果中断启用且有编写 PendSV 异常服务函数的话，
	; 则内核会响应 PendSV 异常，去执行 PendSV 异常服务函数
	LDR		R0, = NVIC_INT_CTRL			; 中断控制及状态寄存器 SCB_ICSR 的地址
	LDR		R1, = NVIC_PENDSVSET		; 触发 PendSV 异常的值 Bit28：PENDSVSET
	STR		R1, [R0]

	; 开中断
	CPSIE	I

	; 程序永远不会执行到这
OSStartHang
	B		OSStartHang
```

### 3.2 中断服务程序 PendSV_Handler (ARM汇编) (os_cpu_a.s)

一旦触发了 PendSV 异常，那么将运行该中断服务程序。这个程序的结构大体如下：
```
OS_CPU_PendSVHandler
    CPSID I ; 关中断
    ;保存上文 
    ;....................... 
    ;切换下文 
    CPSIE I ;开中断
    BX LR ;异常返回
```

在看下面的程序之前，撇开系统启动的话题，不妨想一下，假设我们找到了优先级最高的任务，现在需要切换到这个任务，我们需要做些什么？
- 首先，需要保存之前任务的寄存器状态，当前 PSP 指向的是当前任务的栈，因此可先把它们压到之前任务的栈中。注意，进入中断前，硬件已自动压入了一些寄存器的状态，其他的寄存器需要我们自己手动压栈。接着需要更新（保存）当前任务的 TCB 内容，回忆一下，这个 TCB 存储了该任务的栈指针和栈大小等信息，因此可将 PSP 存入 StkPtr。总之，寄存器和 TCB，缺一不可。
- 其次，需要切换栈，这一点毋庸置疑吧？因此，OSTCBCurPtr 即用来记录当前运行任务的 TCB 指针需要指向新的 TCB，而这个 TCB 存储了该任务的栈指针和栈大小等信息，那么我们可以将该任务 TCB 的 StkPtr 传给 PSP。
- 再次，找到的优先级最高任务的 TCB 指针存放在 OSTCBHighRdyPtr，所以要更新 OSTCBCurPtr 的内容，使 OSTCBCurPtr = OSTCBHighRdyPtr。
- 最后，让新任务栈中的寄存器状态出栈，加载到寄存器中。我们要手动加载部分寄存器，剩余的寄存器由中断返回时加载，在这时 PC 值也被更新为新任务的地址了（这个地址不一定是任务入口处，也可能是之前被打断的地方）。
- 到此为止，我们修改了什么？PSP、OSTCBCurPtr 和寄存器状态。

好，读懂这段代码应该是顺理成章的事情了。总结一下，程序完成的功能有：
- 关中断。
- 先判断栈指针是否为 0，如果为 0，说明是系统刚启动，在进行第一次任务切换，之前没有任务，那么我们不用将寄存器手动压栈了，跳过这个步骤。至于 CPU 自己压入的寄存器值，可以不管。
- 如果不是第一次任务切换，那么需要将寄存器手动压栈。**（保存上文）**
- 将 OSTCBHighRdyPtr（新任务 TCB） 存到 OSTCBCurPtr 中，表明现在运行的任务已改变，得到新任务 TCB。
- 从新任务 TCB 得到了其栈指针，那么加载新任务的栈指针到 PSP，得到新任务栈的位置。
- 已得到了新任务的栈，那么将其存储的寄存器状态手动出栈，加载到寄存器中。**（加载下文） （不得不说，以上三步应该是整个切换过程中最画龙点睛的地方！）**
- 完成上下文切换，开中断。

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
	
	
	END          	
```

简要说明下面这行代码的意思：在 CM3 中，栈指针分为 MSP 和 PSP，任意时刻只能使用其中一个，MSP为复位后缺省使用的堆栈指针，异常永远使用MSP，如果手动开启PSP，那么线程使用PSP，否则也使用MSP。置 LR 的位 2 为 1，那么异常返回后，CPU 将使用 PSP。

```
ORR     LR, LR, #0x04       ; 确保异常返回使用的堆栈指针是PSP，即LR寄存器的位2要为1
```

### 3.3 任务切换函数 OSShed() (os_core.c)

该函数用于任务切换，由于还没有实现优先级等功能，因此我们先使用两个任务轮转的方式来编写。实质是，通过 PendSV 异常（宏定义 OS\_TASK\_SW）来改变 OSTCBCurPtr 的值，从而达到任务切换的效果。

```c
void OSSched (void)
{
	if( OSTCBCurPtr == OSRdyList[0].HeadPtr )
	{
		OSTCBHighRdyPtr = OSRdyList[1].HeadPtr;
	}
	else
	{
		OSTCBHighRdyPtr = OSRdyList[0].HeadPtr;
	}
	
	OS_TASK_SW();
}
```

在 os_cpu.h 中已经定义：
```c
#ifndef  OS_CPU_H
#define  OS_CPU_H

/*********************************************************************************************************/
#ifndef  NVIC_INT_CTRL
	#define  NVIC_INT_CTRL         		*((CPU_REG32 *)0xE000ED04)   /* 中断控制及状态寄存器 SCB_ICSR */
#endif

#ifndef  NVIC_PENDSVSET
	#define  NVIC_PENDSVSET       		0x10000000    /* 触发PendSV异常的值 Bit28：PENDSVSET */
#endif

#define  OS_TASK_SW()               NVIC_INT_CTRL = NVIC_PENDSVSET
#define  OSIntCtxSw()               NVIC_INT_CTRL = NVIC_PENDSVSET

/*********************************************************************************************************/

void OSStartHighRdy(void);
void PendSV_Handler(void);

#endif   /* OS_CPU_H */
```

## 4 任务创建及切换实例

功能：实现两个任务的切换。

在 app.c 中：
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
	
	/* 初始化相关的全局变量 */
	OSInit(&err);
	
	/* 创建任务 */
	OSTaskCreate ((OS_TCB*)      &Task1TCB, 
	              (OS_TASK_PTR ) Task1, 
	              (void *)       0,
	              (CPU_STK*)     &Task1Stk[0],
	              (CPU_STK_SIZE) TASK1_STK_SIZE,
	              (OS_ERR *)     &err);

	OSTaskCreate ((OS_TCB*)      &Task2TCB, 
	              (OS_TASK_PTR ) Task2, 
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

void delay (uint32_t count)
{
	for(; count!=0; count--);
}


void Task1 (void *p_arg)
{
	for( ;; )
	{
		flag1 = 1;
		delay( 100 );		
		flag1 = 0;
		delay( 100 );
		
		/* 任务切换，这里是手动切换 */		
		OSSched();
	}
}

void Task2 (void *p_arg)
{
	for( ;; )
	{
		flag2 = 1;
		delay( 100 );		
		flag2 = 0;
		delay( 100 );
		
		/* 任务切换，这里是手动切换 */
		OSSched();
	}
}
```


