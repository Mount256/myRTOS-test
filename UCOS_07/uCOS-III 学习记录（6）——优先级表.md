参考内容：《[野火]uCOS-III内核实现与应用开发实战指南——基于STM32》第 11 章。

[toc]

## 1 优先级表（OSPrioTbl）的定义（os_prio.c） 

在文件 os\_prio.c 中定义优先级表，它是一个数组：
```c
CPU_DATA OSPrioTbl[OS_PRIO_TBL_SIZE];  /* 定义优先级表 */
```

### 1.1 CPU_DATA——一个数组元素的数据长度为多少？（cpu.h）

在 cpu.h 中定义了 CPU\_DATA 的类型为 unsigned int，也就是一个数组元素为 32 位：
```c
typedef unsigned int  	CPU_INT32U;

typedef  CPU_INT32U     CPU_DATA;
```

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

其中，DEF\_OCTET\_NBR\_BITS 表示一个字节长度是多少位，这里默认设置为 8 位，在 cpu\_def.h 中已定义：
```c
/* 一个字节长度为8位 */
#define DEF_OCTET_NBR_BITS		8u
```

CPU\_CFG\_DATA\_SIZE 表示 CPU 数据的字长。类似地，CPU\_CFG\_ADDR\_SIZE 表示 CPU 地址的字长。在 cpu.h 中已定义：
```c
/*********************************CPU字长配置**********************************/

#define  CPU_CFG_ADDR_SIZE              CPU_WORD_SIZE_32
#define  CPU_CFG_DATA_SIZE              CPU_WORD_SIZE_32
#define  CPU_CFG_DATA_SIZE_MAX          CPU_WORD_SIZE_64
```

