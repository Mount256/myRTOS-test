参考内容：《[野火]uCOS-III内核实现与应用开发实战指南——基于STM32》第 6 章。

[toc]

## 前排提醒

-  按照 μC/OS-III 中的函数命名规则，以大小的 OS 开头，表示这是一个外部函数，可以由用户调用，以 OS_ 开头的函数表示内部函数，只能由 μC/OS-III 内部使用。紧接着是文件名，表示该函数放在哪个文件，最后是函数功能名称。例如：OSTaskCreate，说明允许用户调用，位于 os_task.c 文件中。

## 0 数据类型声明
### 0.1 任务控制块（OS_TCB） (os.h)

任务控制块（TCB）：用于记录栈顶指针和栈的大小。

```c
/* TCB 重命名为大写字母格式 */
typedef struct os_tcb	OS_TCB;

/* TCB 数据类型声明 */
struct os_tcb{
	CPU_STK			*StkPtr;
	CPU_STK_SIZE	StkSize;
};
```

### 0.2 就绪列表（OS_RDY_LIST）(os.h)

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

## 1 任务的创建
### 1.1 任务创建函数 OSTaskCreate() (os_task.c)

任务创建函数需要完成的三件事：
- 创建任务栈：调用函数 OSTaskStkInit()
- 填写 TCB：剩余栈栈顶指针 SP 存入 TCB 的第一个成员 StkPtr
- 填写 TCB：将任务栈的大小存入 TCB 的第二个成员 StkSize

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

### 1.2 任务栈创建函数 OSTaskStkInit() (os_cpu_c.c)

注意：
- 由于是使用 PendSV 中断发起任务切换，因此 CPU 在切换新的任务前，会自动将新任务的任务栈按顺序出栈写入到寄存器中，这个顺序为：R0、R1、R2、R3、R12、R14(LR)、R15(PC)、XPSR。所以，CPU 会按这个顺序将寄存器压入旧任务的任务栈中：XPSR、R15(PC)、R14(LR)、R12、R3、R2、R1、R0。
- 由以上讨论可知：1. 部分寄存器仍没有压入栈中，需要我们自己在程序中手动压入；2. 我们在写程序的时候，入栈和出栈的顺序是严格确定好的，要按照硬件要求去写，不能改变。
- 像 0x14141414u 这些数是方便我们调试的，说白了就是，这样写，我们就容易知道这个位置是 R14 的，不是别的寄存器的。这些数字除了方便我们看之外，没有任何意义，你也可以全部初始化为零，或别的数字。
- 函数功能：初始化任务栈，先在任务栈中为寄存器预留栈空间，再返回分配好空间后的栈指针。

```c
/* 任务栈初始化函数 */
CPU_STK *OSTaskStkInit ( OS_TASK_PTR 	p_task,  		/* 任务名，指示着任务的入口地址 */
						 void			*p_arg,  		/* 任务的形参 */
						 CPU_STK		*p_stk_base, 	/* 任务栈的起始地址 */
						 CPU_STK_SIZE	stk_size ) 		/* 任务栈的大小 */
{
	CPU_STK		*p_stk;
	
	p_stk_base = &p_stk_base[stk_size];		/* 获取任务栈的栈顶地址 */
	
	/* 任务第一次运行时，CPU寄存器需要预设数据 */
	/* 首先是异常发生时自动保存的 8 个寄存器 */
	/* R14、R12、R3、R2 和 R1 为了调试方便，需填入与寄存器号相对应的 16 进制数 */
	*--p_stk = (CPU_STK) 0x010000000u;		/* xPSR 的 bit24 必须置 1 		*/
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
在创建好任务后，可以启动 OS 进行任务调度了。

## 2 内核OS的启动
### 2.1 系统初始化 OSInit() (os_core.c)

系统初始化完成的事情：
- 标记系统运行状态：停止状态（因为此时未执行函数 OSSTart() ）
- 当前正在运行的任务

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

### 2.2 

## 3 任务的切换