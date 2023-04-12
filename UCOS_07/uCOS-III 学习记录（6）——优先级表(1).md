参考内容：《[野火]uCOS-III内核实现与应用开发实战指南——基于STM32》第 11 章。

[toc]

## 1 优先级表（OSPrioTbl）的定义（os_prio.c） 

在文件 os\_prio.c 中定义优先级表，它是一个数组：
```c
CPU_DATA OSPrioTbl[OS_PRIO_TBL_SIZE];  /* 定义优先级表 */
```

接下来着重理解 CPU\_DATA 和 OS\_PRIO\_TBL\_SIZE 的含义。

### 1.1 CPU_DATA——一个数组元素的数据长度为多少？（cpu.h）

在 cpu.h 中定义了 CPU\_DATA 的类型为 unsigned int，也就是一个数组元素为 32 位：
```c
typedef unsigned int  	CPU_INT32U;

typedef  CPU_INT32U     CPU_DATA;
```

如果 MCU 的（字长）类型是 16 位、8 位或者 64 位，只需要把优先级表的数据类型 CPU\_DATA 改成相应的位数即可。

### 1.2 OS_PRIO_TBL_SIZE——数组有多大？（os.h）

这个问题将会关系到优先级表有多大。在 os.h 中宏定义了 OS\_PRIO\_TBL\_SIZE：
```c
#define  OS_PRIO_TBL_SIZE		( ((OS_CFG_PRIO_MAX - 1u) / DEF_INT_CPU_NBR_BITS) + 1u )
```

接下来，我们来讲讲 OS\_CFG\_PRIO\_MAX 和 DEF\_INT\_CPU\_NBR\_BITS 的来由。

#### 1.2.1 OS_CFG_PRIO_MAX——支持多少个优先级？（os_cfg.h）

在 os\_cfg.h 中宏定义了 OS\_CFG\_PRIO\_MAX。我们设置为 32，即最大支持 32 个优先级：
```c
/* 支持最大的优先级 */
#define OS_CFG_PRIO_MAX		32u
```

#### 1.2.2 DEF_INT_CPU_NBR_BITS——CPU 整型数据有多少位？（cpu_def.h）

DEF\_INT\_CPU\_NBR\_BITS 表示一个整型数据有多少位，在 cpu\_def.h 定义：
```c
/* CPU整型数位定义 */
#define DEF_INT_CPU_NBR_BITS	(DEF_OCTET_NBR_BITS * CPU_CFG_DATA_SIZE)
```

- DEF\_OCTET\_NBR\_BITS 表示一个字节长度是多少位，这里默认设置为 8 位，在 cpu\_def.h 中已定义：
```c
/* 一个字节长度为8位 */
#define DEF_OCTET_NBR_BITS		8u
```

- CPU\_CFG\_DATA\_SIZE 表示 CPU 数据的字长相当于多少个字节。类似地，CPU\_CFG\_ADDR\_SIZE 表示 CPU 地址的字长相当于多少个字节。在 cpu.h 中已定义：
```c
/*********************************CPU字长配置**********************************/

#define  CPU_CFG_ADDR_SIZE              CPU_WORD_SIZE_32
#define  CPU_CFG_DATA_SIZE              CPU_WORD_SIZE_32
#define  CPU_CFG_DATA_SIZE_MAX          CPU_WORD_SIZE_64
```

- 而 CPU\_WORD\_SIZE\_32 又在 cpu\_def.h 中已定义，表示 32 位长度在这种设置下被定义为 4 个字节：
```c
#define CPU_WORD_SIZE_08		1u
#define CPU_WORD_SIZE_16		2u
#define CPU_WORD_SIZE_32		4u
#define CPU_WORD_SIZE_64		8u
```

因此，<code>DEF\_INT\_CPU\_NBR\_BITS = DEF\_OCTET\_NBR\_BITS * CPU\_CFG\_DATA\_SIZE = 8（一个字节有 8 位） * 4（4 个字节） = 32</code>，说明 CPU 整型数据有 32 位，即字长为 32。

> 【恶补概念！】
> - 位（bit）：位表示的是二进制位，一般称为比特，是计算机存储的最小单位。
> - 字节（byte）：字节是计算机中数据处理的基本单位。计算机中以字节为单位存储和解释信息，规定一个字节由八个二进制位构成，即 1 个字节等于 8 个比特（1Byte = 8bit）。
> - 字（word）：计算机进行数据处理时，一次存取、加工和传送的数据长度称为字（word）。一个字通常由一个或多个（一般是字节的整数位）字节构成。例如 286 微机的字由 2 个字节组成，它的字长为 16；486 微机的字由 4 个字节组成，它的字长为 32。计算机的字长决定了其 CPU 一次操作处理实际位数的多少，由此可见计算机的字长越大，其性能越优越。
> - 字长：计算机的每个字所包含的位数称为字长。例如 286 微机的字由 2 个字节组成，它的字长为 16；486 微机的字由 4 个字节组成，它的字长为 32。

### 1.3 优先级表的结构——定义好了以后？

由以上讨论可知，我们定义的 CPU 字长为 32，支持最大优先级为 32，则<code>DEF\_INT\_CPU\_NBR\_BITS = 32</code>，<code>OS\_CFG\_PRIO\_MAX = 32</code>，所以数组元素一共有：<code>( (OS\_CFG\_PRIO\_MAX - 1u) / DEF\_INT\_CPU\_NBR\_BITS ) + 1u = ( (32 - 1) / 32 ) + 1 = 0 + 1 = 1</code>。优先级表里只有一个元素。

如果我们需要支持 64 个优先级，CPU 字长为 32，那么<code>DEF\_INT\_CPU\_NBR\_BITS = 32</code>，<code>OS\_CFG\_PRIO\_MAX = 64</code>，所以数组元素一共有：<code>( (OS\_CFG\_PRIO\_MAX - 1u) / DEF\_INT\_CPU\_NBR\_BITS ) + 1u = ( (64 - 1) / 32 ) + 1 = 1 + 1 = 2</code>。优先级表里两个元素。

那么优先级表和优先级的关系是什么？

【图片】


## 2 初始化优先级表 OS_PrioInit()（os_prio.c）
