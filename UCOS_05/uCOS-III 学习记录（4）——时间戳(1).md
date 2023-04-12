参考内容：《[野火]uCOS-III内核实现与应用开发实战指南——基于STM32》第 9 章。

[toc]

## 1 时间戳

在 uCOS 中，如果要测量一段代码 A 的时间，那么可以在代码段 A 运行前记录一个时间点 TimeStart，在代码段 A 运行完记录一个时间点 TimeEnd，那么代码段 A 的运行时间 TimeUse 就等于 TimeEnd 减去 TimeStart。这里面的两个时间点 TimeEnd 和 TimeStart，就叫作时间戳，时间戳实际上就是一个时间点。

比如，在下面的代码中，在需要开始测量的地方调用<code>OS\_TS\_GET()</code>，在需要结束测量的地方再次调用<code>OS\_TS\_GET()</code>，则两次调用之间的代码即为被测量的时间长度。

```c
TimeStart = OS_TS_GET();
OSTimeDly (20);	
TimeEnd = OS_TS_GET();
TimeUse = TimeEnd - TimeStart;
```

如果 Tick = 10ms，那么 TimeUse 测量到的是 200ms。

现在的问题是：如何实现时间戳呢？DWT 要出场了。

## 2 DWT 外设
### 2.1 DWT 外设简介

在 uCOS 中，我们已经使用了 SysTick 作为系统的时间片，所以不能再使用 SysTick 来实现时间戳了。在 Cortex-M3 中有一个调试组件，其中有一个组件是跟踪组件，叫数据观察点与跟踪（Data Watchpoint and Trace，DWT）外设，该外设有一个 32 位寄存器 CYCCNT，它是一个向上的计数器，记录的是内核时钟 HCLK 运行的个数，当 CYCCNT 溢出之后，会清零重新开始向上计数。该计数器在 uCOS 中正好被用来实现时间戳的功能。

在 STM32F103 系列的单片机中，HCLK 时钟最高为 72M，单个时钟的周期为 1/72us = 0.0139us = 14ns，CYCCNT 总共能记录的时间为 2^32 * 14 = 60s。在 uCOS 中，要测量的时间都是很短的，都是 ms 级别，根本不需要考虑定时器溢出的问题。如果内核代码执行的时间超过 s 的级别，那就背离了实时操作系统实时的设计初衷了，没有意义。

### 2.2 初始化 DWT 的步骤

- 使能 DWT 外设：由内核调试寄存器 DEMCR（地址：0xE000EDFC）的位 24 控制，写入 1 表示开启 DWT 外设。 
- 初始化 DWT\_CYCCNT 寄存器：将 DWT\_CYCCNT（地址：0xE0001004）寄存器清零。
- 启用 DWT\_CYCCNT 寄存器：由 DWT\_CTRL（地址：0xE0001000）寄存器的位 0（CYCCNTENA）控制，写入 1 表示开启 DWT\_CYCCNT 寄存器。

### 2.3 DWT 外设的宏定义（cpu_core.c）

为了提高代码的可读性和易修改性，将与 DWT 外设有关的地址和掩码定义为宏定义，如下所示：
```c
/*
*********************************************************************************************************
*                                             寄存器定义
*********************************************************************************************************
*/
#define  BSP_REG_DEM_CR                       (*(CPU_REG32 *)0xE000EDFC)    // DEMCR的地址
#define  BSP_REG_DWT_CR                       (*(CPU_REG32 *)0xE0001000)    // DWT_CTRL的地址
#define  BSP_REG_DWT_CYCCNT                   (*(CPU_REG32 *)0xE0001004)    // DWT_CYCCNT的地址
#define  BSP_REG_DBGMCU_CR                    (*(CPU_REG32 *)0xE0042004)    // 未用到，先忽略

/*
*********************************************************************************************************
*                                            寄存器位定义
*********************************************************************************************************
*/
#define  BSP_DBGMCU_CR_TRACE_IOEN_MASK                   0x10       // 这些暂时用不到，可先忽略
#define  BSP_DBGMCU_CR_TRACE_MODE_ASYNC                  0x00
#define  BSP_DBGMCU_CR_TRACE_MODE_SYNC_01                0x40
#define  BSP_DBGMCU_CR_TRACE_MODE_SYNC_02                0x80
#define  BSP_DBGMCU_CR_TRACE_MODE_SYNC_04                0xC0
#define  BSP_DBGMCU_CR_TRACE_MODE_MASK                   0xC0

#define  BSP_BIT_DEM_CR_TRCENA                          (1<<24)     // DEMCR的位24置1 

#define  BSP_BIT_DWT_CR_CYCCNTENA                       (1<<0)      // DWT_CTRL的位0置1
```

## 3 时间戳的初始化

时间戳的初始化包括两部分：

### 3.1 时间戳的相关定义（cpu_core.h）
#### 3.1.1 通过宏定义开启/关闭时间戳功能

uCOS 实现了很多功能，但很多时候，有些功能用不到，我们并不需要那么长的代码。于是，我们可以在 H 文件中通过宏定义开启或关闭某些功能，这样就能达到裁剪的目的。

在 cpu_core.h 中定义了使能时间戳的宏定义，如下所示：
```c
/**********************************开启/关闭***************************************************/
	
/* 是否开启时间戳？ */
#if ((CPU_CFG_TS_32_EN == DEF_ENABLED) || (CPU_CFG_TS_64_EN == DEF_ENABLED))
#define	CPU_CFG_TS_EN	DEF_ENABLED
#else
#define CPU_CFG_TS_EN	DEF_DISABLED
#endif

/* 是否开启时间戳定时器？ */
#if ((CPU_CFG_TS_EN == DEF_ENABLED) || defined(CPU_CFG_INT_DIS_MEAS_EN))
#define	CPU_CFG_TS_TMR_EN	DEF_ENABLED
#else
#define CPU_CFG_TS_TMR_EN	DEF_DISABLED
#endif
```

这些宏定义怎么用呢？待会可以参考下面几节的代码，你就明白了。

#### 3.1.2 时间戳的数据类型定义

```c
typedef CPU_INT32U	CPU_TS32;
typedef CPU_INT32U	CPU_TS_TMR_FREQ;
typedef	CPU_TS32	CPU_TS;
typedef	CPU_INT32U	CPU_TS_TMR;
```

#### 3.1.3 时间戳的全局变量定义——CPU_TS_TmrFreq_Hz

CPU\_TS\_TmrFreq\_Hz 是一个 32 位的全局变量，用来记录 CPU 的系统时钟频率。

```c
#if (CPU_CFG_TS_TMR_EN == DEF_ENABLED)
CPU_CORE_EXT	CPU_TS_TMR_FREQ		CPU_TS_TmrFreq_Hz;
#endif
```

### 3.2 CPU 初始化函数 CPU_Init()（cpu_core.c）

该函数实现的功能：
- 初始化时间戳。
- 初始化中断禁用时间测量。（目前未实现）
- 初始化 CPU 名字。（目前未实现）

```c
/* CPU 初始化函数 */
void CPU_Init (void)
{
#if ((CPU_CFG_TS_EN == DEF_ENABLED) || (CPU_CFG_TS_TMR_EN == DEF_ENABLED))
	CPU_TS_Init();		/* 时间戳初始化函数 */
#endif
}
```

发现时间戳初始化函数被包在了条件编译中，当我们没有定义 CPU\_CFG\_TS\_EN 为 DEF\_ENABLED 时，这段代码将不会出现在执行中，这样就能实现代码的裁剪了。

### 3.3 时间戳初始化函数 CPU_TS_Init()（cpu_core.c）

### 3.4 时间戳定时器初始化函数

### 3.5 获取 CPU 的 HCLK 时钟

### 3.6 获取 CYCCNT 计数器 
