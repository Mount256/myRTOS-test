参考内容：《[野火]uCOS-III内核实现与应用开发实战指南——基于STM32》第 9 章。

[toc]

## 1 时间戳
### 1.1 时间戳简介

在 uCOS 中，如果要测量一段代码 A 的时间，那么可以在代码段 A 运行前记录一个时间点 TimeStart，在代码段 A 运行完记录一个时间点 TimeEnd，那么代码段 A 的运行时间 TimeUse 就等于 TimeEnd 减去 TimeStart。这里面的两个时间点 TimeEnd 和 TimeStart，就叫作时间戳，时间戳实际上就是一个时间点。

### 1.2 DWT 外设简介

在 uCOS 中，我们已经使用了 SysTick 作为系统的时间片，所以不能再使用 SysTick 来实现时间戳了。在 Cortex-M3 中有一个调试组件，其中有一个组件是跟踪组件，叫数据观察点与跟踪（Data Watchpoint and Trace，DWT）外设，该外设有一个 32 位寄存器 CYCCNT，它是一个向上的计数器，记录的是内核时钟 HCLK 运行的个数，当 CYCCNT 溢出之后，会清零重新开始向上计数。该计数器在 uCOS 中正好被用来实现时间戳的功能。

在 STM32F103 系列的单片机中，HCLK 时钟最高为 72M，单个时钟的周期为 1/72us = 0.0139us = 14ns，CYCCNT 总共能记录的时间为 2^32 * 14 = 60s。在 uCOS 中，要测量的时间都是很短的，都是 ms 级别，根本不需要考虑定时器溢出的问题。如果内核代码执行的时间超过 s 的级别，那就背离了实时操作系统实时的设计初衷了，没有意义。

### 1.3 初始化 DWT 的步骤

- 使能 DWT 外设：由内核调试寄存器 DEMCR（地址：0xE000EDFC）的位 24 控制，写入 1 表示开启 DWT 外设。 
- 初始化 DWT\_CYCCNT 寄存器：将 DWT\_CYCCNT（地址：0xE000 1004）寄存器清零。
- 启用 DWT\_CYCCNT 寄存器：由 DWT\_CTRL（地址：0xE0001000）寄存器的位 0（CYCCNTENA）控制，写入 1 表示开启 DWT\_CYCCNT 寄存器。

## 2 初始化 CPU
