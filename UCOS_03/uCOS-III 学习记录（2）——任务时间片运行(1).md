参考内容：《[野火]uCOS-III内核实现与应用开发实战指南——基于STM32》第 7 章。

[toc]

这章的内容是：每隔一定的时间间隔，就会进行一次任务切换，使每个任务都能均等享有 CPU 控制权，这种不停地上下文切换的过程，有点像多进程。同是任务切换，与第一篇笔记不同的是：**第一篇笔记是任务自己主动切换，这篇是 SysTick 发起的切换。**

## 1 初始化 SysTick
### 1.1 SysTick 初始化函数 OS_CPU_SysTickInit (os\_cpu\_c.c)

由于 SysTick 属于内核外设，所以在 os\_cpu\_c.c 中定义该函数。实现方法其实很简单，就是我们之前在 STM32 所学的 SysTick 那样进行初始化。

其中函数参数为 ms，意思是经过多少 ms 后触发 SysTick 中断。之后在中断程序内，进行任务切换即可。

```c
/* 初始化SysTick */
void OS_CPU_SysTickInit (CPU_INT32U ms)
{ 
	SysTick->LOAD  = ms * SystemCoreClock / 1000 - 1;    			/* set reload register */
	
	/* set Priority for Cortex-M0 System Interrupts */
	NVIC_SetPriority (SysTick_IRQn, (1<<__NVIC_PRIO_BITS) - 1); 	/* 优先级为 15 ( = (1<<3) - 1 ) */
	
	SysTick->VAL   = 0;                                          	/* Load the SysTick Counter Value */
	
	/* Enable SysTick IRQ and SysTick Timer */
	SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk | 					/* 选择时钟源为 SystemCoreClock */
					SysTick_CTRL_TICKINT_Msk   | 					/* 启用中断 */
					SysTick_CTRL_ENABLE_Msk;                 		/* 开启使能 */   	
}
```

### 1.2 SysTick 中断服务程序 SysTick_Handler (os\_cpu\_c.c)

在中断服务函数内调用 OSTimeTick，启动任务切换。

```c
void SysTick_Handler (void)
{
	OSTimeTick();
}
```

### 1.3 OSTimeTick (os_time.c)

嗯。。。这种函数调用函数，有点套娃的感觉。。。

```c
void OSTimeTick (void)
{
	/* 每间隔一定的 Tick 会进行一次任务切换，因此每个任务平等享有 CPU 控制权 */
	OSSched();  // 任务切换
}
```

## 2 实现任务按一定时间间隔切换的功能

之前第一篇笔记里，实现的是任务自己主动进行切换，而本篇笔记实现的是经过一定时间后进行任务切换，不是任务自己去切换，大家要区分这一点。

### 2.1 任务的创建和 OS 启动 (app.c)

需要注意，由于我们用 SysTick 中断来引发任务切换，所以在一开始初始化 SysTick 前，由于 OS 尚未启动，任务尚未创建，因此要关闭中断。关中断函数的定义下面会提到。

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
	
	/* 关中断，因为此时 OS 未启动，若开启中断，那么 SysTick 将会引发中断 */
	CPU_IntDis();
	
	/* 初始化 SysTick，配置 SysTick 为 10ms 中断一次 */
	OS_CPU_SysTickInit(10);
	
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
	}
	// 不用手动任务切换
}

void Task2 (void *p_arg)
{
	for( ;; )
	{
		flag2 = 1;
		delay( 100 );		
		flag2 = 0;
		delay( 100 );
	}
	// 不用手动任务切换
}

```

### 2.2 开中断和关中断函数 CPU_IntEn、CPU_IntDis (cpu_a.asm)

> 【asm 文件和 s 文件的区别（待考证）】前者是 dos 和 win 下常见的源程序扩展名，后者是 linux 内核源程序中用的扩展名。本质上都是文本文档，没区别。有些 linux 下的编译器在产生汇编码输出的时候，也会生成 .s 的扩展名。

这样写是为了方便我们以后开中断或关中断，因为中断这种东西，需要汇编层面的操作，直接内嵌在 C 中不好看，不如写个函数，每次需要时就调用很方便。

```
    EXPORT  CPU_IntDis
    EXPORT  CPU_IntEn

	AREA |.text|, CODE, READONLY, ALIGN=2
    THUMB
    REQUIRE8
    PRESERVE8

CPU_IntDis
	CPSID 	I	; 关中断
	BX		LR
	
CPU_IntEn
	CPSIE	I	; 开中断
	BX	LR
	
	
	END

```

## 3 实验现象

【图片】

首先，如果没有放大波形，观察到的是红色色块和绿色色块交替出现，色块与色块之间间隔 10ms，这是 SysTick 中断引起的任务切换。

【图片】

接着，我们放大后可以观察到这些色块实际上是由许多小方波组成，这些小方波之间的间隔是 delay(100) 造成的。本质上，我们将第一篇笔记里的任务切换时间间隔扩展到了 10ms 的长度。
