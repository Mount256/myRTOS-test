参考内容：《[野火]uCOS-III内核实现与应用开发实战指南——基于STM32》第 10 章。

[toc]

## 1 临界段

临界段，又叫做临界区。对于多线程而言，它是一段不可分割、不可上下文切换的代码。对于 uCOS 而言，它是一段不可被中断的代码。临界段是不能被中断的，需要关中断或锁调度器（OSSched）以保护临界段。

什么情况下临界段代码会被打断？由刚才的描述可知，临界段被打断有两种情况：
- 外部中断。
- 系统调度。在系统调度中会产生 PendSV 异常中断，最终也可以归结为中断。

因此，**对临界段保护的实质就是控制中断的开启和关闭。**

uCOS 定义了进入临界段的宏和退出临界段的宏，这两个宏分别实现了中断的开启和关闭。
- OS\_CRITICAL\_ENTER() 或 CPU\_CRITICAL\_ENTER()
- OS\_CRITICAL\_EXIT() 或 CPU\_CRITICAL\_EXIT()

还有一个宏，用于存储中断的状态：
- CPU\_SR\_ALLOC()

## 2 临界段的保护
### 2.1 Cortex-M 内核的中断指令

在 CM 内核中，中断是通过 CPS 指令来控制的。
```
CPSID I     ;PRIMASK=1，关中断
CPSIE I     ;PRIMASK=0，开中断
CPSID F     ;FAULTMASK=1，关异常
CPSIE F     ;FAULTMASK=0，开异常
```

PRIMASK 和 FAULTMAST 是 CM 内核里面三个中断屏蔽寄存器中的两个，还有一个是 BASEPRI，最后一个用不到，就不进行介绍了。
- PRIMASK：只有 1 个比特位的寄存器。在它被置 1 后，就关掉所有可屏蔽的异常，只剩下 NMI（不可屏蔽中断）和硬件 FAULT（HardFault） 可以响应。它的默认值是 0，表示没有关中断。
- FAULTMASK：只有 1 个比特位的寄存器。当它置 1 时，只有 NMI（不可屏蔽中断）才能响应，所有其他的异常，甚至是硬件 FAULT（HardFault），也通通闭嘴。它的默认值是 0，表示没有关异常。

因此，也可以通过 MSR 指令修改 PRIMASK（或 FAULTMASK）来开启或关闭中断：
```
MOVS   R0, #1
MSR    PRIMASK,  R0     ; 将 1 写入 PRIMASK 禁止所有中断

MOVS   R0, #0
MSR    PRIMASK,  R0     ; 将 0 写入 PRIMASK 使能中断
```

### 2.2 开中断和关中断
#### 2.2.1 关中断 CPU_SR_Save()（cpu_a.asm）

该函数完成的事情：
- **存储中断状态**：通过 MRS 指令将特殊寄存器 PRIMASK 寄存器的值存储到通用寄存器 R0。（当在 C 中调用汇编的子程序返回时，会将 R0 作为函数的返回值。所以在 C 中调用 CPU\_SR\_Save() 的时候，需要事先声明一个变量用来存储 CPU\_SR\_Save() 的返回值，即 R0 寄存器的值，也就是 PRIMASK 的值。）
- **关闭中断**：即使用 CPS 指令将 PRIMASK 寄存器的值置 1。
- 子程序返回。

```
; CPU_SR  CPU_SR_Save (void);  （临界段关中断，R0 为返回值）
CPU_SR_Save		
	MRS		R0, PRIMASK		; 将 PRIMASK 寄存器的值存入 R0 中
	CPSID 	I	; 关中断
	BX		LR
```

#### 2.2.2 开中断 CPU_SR_Restore()（cpu_a.asm）

该函数完成的事情：
- **恢复中断状态**：通过 MSR 指令将通用寄存器 R0 的值存储到特殊寄存器 PRIMASK。（当在 C 中调用汇编的子程序返回时，会将第一个形参传入到通用寄存器 R0。所以在 C 中调用 CPU\_SR\_Restore() 的时候，需要传入一个形参，该形参是进入临界段之前保存的 PRIMASK 的值。）
- 子程序返回。


```
; void CPU_SR_Restore (CPU_SR  cpu_sr);   （临界段开中断，R0 为形参）
CPU_SR_Restore	
	MSR		PRIMASK, R0		; 将 R0 的值存入 PRIMASK 寄存器中
	BX		LR
```

为什么开中断不直接使用 CPS 指令呢？待会在应用那节（2.3.2 节）你就会明白了。

#### 2.2.3 宏定义封装（cpu.h）

最后，在 cpu.h 中，将开中断和关中断的函数封装成一个宏，方便调用。

```c
/*********************************CPU寄存器数据类型定义*********************************/
typedef volatile 		CPU_INT32U		CPU_REG32;
typedef 				CPU_REG32		CPU_SR;

/*********************************临界段定义*********************************/

#define CPU_SR_ALLOC()			CPU_SR	cpu_sr = (CPU_SR)0      // 用于存放中断状态
#define CPU_INT_DIS()			do { cpu_sr = CPU_SR_Save(); } while(0)     // 关闭中断，存储中断状态
#define CPU_INT_EN()			do { CPU_SR_Restore(cpu_sr); } while(0)     // 恢复中断状态
#define CPU_CRITICAL_ENTER()	do { CPU_INT_DIS(); } while(0)
#define CPU_CRITICAL_EXIT()		do { CPU_INT_EN();  } while(0)

/*********************************函数声明(cpu_a.asm)*********************************/
void 	CPU_IntDis 		(void);
void 	CPU_IntEn		(void);
CPU_SR 	CPU_SR_Save 	(void);
void 	CPU_SR_Restore 	(CPU_SR  cpu_sr);
```

### 2.3 临界段保护的应用
#### 2.3.1 一层临界段的应用

如果有这么一段临界段代码：
```c
/* 临界段代码保护 */
{
    /* 临界段开始 */
    {
        /* 执行临界段代码，不可中断 */
    }
    /* 临界段结束 */
}
```

那么使用以上宏定义的格式为：
```c
/* 临界段代码保护 */
{
    CPU_SR_ALLOC();         /* cpu_sr = 0 */
    CPU_INT_DIS();          /* 关中断 */
    /* 临界段开始 */
    {
        /* 执行临界段代码，不可中断 */
    }
    /* 临界段结束 */
    CPU_INT_EN();           /* 开中断 */
}
```

若将其展开，则变成：
```c
/* 临界段代码保护 */
{
    CPU_SR	cpu_sr = (CPU_SR)0;     /* (a) 定义一个变量，用于存放中断状态，初始化 cpu_sr = 0 */
    cpu_sr = CPU_SR_Save();         /* (b) cpu_sr 保存当前中断状态，然后关中断 */
    /* 临界段开始 */
    {
        /* 执行临界段代码，不可中断 */
    }
    /* 临界段结束 */
    CPU_SR_Restore(cpu_sr);         /* (c) cpu_sr 写入中断状态，恢复之前的中断状态 */
}
```

这个过程如下：
- （a）临界段开始前，定义一个变量 cpu_sr，用于存储 PRIMASK 的值，即用于保存中断状态。
- （b）先将当前中断状态（即 PRIMASK）的值存储在 cpu\_sr 中。因为当前未关闭中断，所以 PRIMASK = 0，cpu\_sr = 0。然后关闭中断，使 PRIMASK = 1。
- （c）执行完临界段代码后，恢复此前的中断状态，将 cpu\_sr 写入到 PRIMASK 中。当前 PRIMASK = 1，cpu\_sr = 0，写入后，PRIMASK = 0，恢复此前中断状态。

#### 2.3.2 多层临界段的应用

如果是两层临界段代码的嵌套：

```c
/* 临界段代码保护 */
{
    /* 临界段 1 开始 */
    {
        /* 临界段 2 开始 */
        {
            /* 执行临界段代码，不可中断 */
        }
        /* 临界段 2 结束 */
    }
    /* 临界段 1 结束 */
}
```

进入临界段 1 前，需要关闭中断；进入临界段 2 前，也需要关闭中断，那么使用宏定义的格式为：
```c
/* 临界段代码保护 */
{
    CPU_SR_ALLOC();         /* cpu_sr = 0 */
    CPU_INT_DIS();          /* 关中断（临界段 1） */
    /* 临界段 1 开始 */
    {
        CPU_SR_ALLOC();         /* cpu_sr = 0 */
        CPU_INT_DIS();          /* 关中断（临界段 2） */
        /* 临界段 2 开始 */
        {
            /* 执行临界段代码，不可中断 */
        }
        /* 临界段 2 结束 */
        CPU_INT_EN();           /* 开中断（临界段 2） */
    }
    /* 临界段 1 结束 */
    CPU_INT_EN();           /* 开中断（临界段 1） */
}
```

展开宏定义，代码可等效为：
```c
/* 临界段代码保护 */
{
    CPU_SR	cpu_sr1 = (CPU_SR)0;     /* (a) 定义一个变量，用于存放中断状态，初始化 cpu_sr1 = 0 */
    cpu_sr1 = CPU_SR_Save();         /* (b) cpu_sr1 保存当前中断状态，然后关中断 */
    /* 临界段 1 开始 */
    {
        CPU_SR	cpu_sr2 = (CPU_SR)0;     /* (c) 定义一个变量，用于存放中断状态，初始化 cpu_sr2 = 0 */
        cpu_sr2 = CPU_SR_Save();         /* (d) cpu_sr2 保存当前中断状态，然后关中断 */
        /* 临界段 2 开始 */
        {
            /* 执行临界段代码，不可中断 */
        }
        /* 临界段 2 结束 */
        CPU_SR_Restore(cpu_sr2);         /* (e) cpu_sr2 写入中断状态，恢复之前的中断状态 */
    }
    /* 临界段 1 结束 */
    CPU_SR_Restore(cpu_sr1);            /* (f) cpu_sr1 写入中断状态，恢复之前的中断状态 */
}
```

这个过程如下：
- （a）临界段 1 开始前，定义一个变量 cpu\_sr1 用于存储中断状态。
- （b）先将当前中断状态（即 PRIMASK）的值存储在 cpu\_sr1 中。因为当前未关闭中断，所以 PRIMASK = 0，cpu\_sr1 = 0。然后关闭中断，使 PRIMASK = 1。
- （c）临界段 2 开始前，再定义一个变量 cpu\_sr2 用于存储中断状态。
- （d）先将当前中断状态（即 PRIMASK）的值存储在 cpu\_sr2 中。因为当前已经关闭中断，所以 PRIMASK = 1，cpu\_sr2 = 1。然后关闭中断，使 PRIMASK = 1（其实在之前 PRIMASK 就已经为 1 了）。
- （e）执行完临界段 2 后，恢复此前的中断状态，将 cpu\_sr2 写入到 PRIMASK 中。当前 PRIMASK = 1，cpu\_sr2 = 1，写入后，PRIMASK = 1，恢复此前中断状态，即还是关中断状态。（**这一步算是关键了，也彻底弄明白开中断为何不直接用 CPS 指令的原因。若使用 CPS 直接开中断，则会出现：回到临界段 1 时，发现中断居然开着！**）
- （f）执行完临界段 1 后，恢复此前的中断状态，将 cpu\_sr1 写入到 PRIMASK 中。当前 PRIMASK = 1，cpu\_sr1 = 0，写入后，PRIMASK = 0，恢复此前中断状态，也就是开中断状态。

## 3 测量关中断时间

本部分先忽略，因为还不是我重点关注的部分。待有时间再来研究。

