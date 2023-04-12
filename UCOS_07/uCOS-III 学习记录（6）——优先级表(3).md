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

如下图所示，这是 CPU 字长为 32 的优先级表结构：

【图片】

- 若最大支持 32 个优先级，那么说明优先级一共有 0-31 的编号，没有 32 的编号。
- 优先级表 OSPrioTbl[0] 记录了优先级 0-31 的情况，OSPrioTbl[1] 记录了优先级 32-63 的情况，以此类推。
- 数据的每一位表示该优先级的任务是否存在，置 1 表示该优先级存在任务，置 0 表示该优先级没有任务。
- **数据的高位对应的是更低的优先级，低位对应的是更高的优先级**。比如，OSPrioTbl[0] 的位 31 对应的是优先级 0，而位 30 对应的是优先级 1。对于我们定义的字长和最大优先级，若要创建一个优先级为 prio 的任务，那么就在 OSPrioTbl[0] 的位 [31-prio] 置 1 即可。这与我们的认知正好相反。
- **总结：若要创建一个优先级为 prio 的任务，那么就在<code>OSPrioTbl[prio/字长]</code>的位<code>[字长-prio-1]</code>置 1 即可。**

## 2 初始化优先级表 OS_PrioInit()（os_prio.c）

该函数的功能是：
- 初始化优先级表 OSPrioTbl，将每个数组元素全部置 0。

```c
/* 初始化优先级表 */
void OS_PrioInit (void)
{
	CPU_DATA	i;
	
	/* 全部初始化为 0 */
	for (i = 0u; i < OS_PRIO_TBL_SIZE; i++)
	{
		OSPrioTbl[i] = (CPU_DATA)0;
	}
}
```

该函数需要被 OSInit() 调用。初始化后的优先级表如下所示：

【图片】

## 3 置位优先级表中的相应位 OS_PrioInsert()（os_prio.c）

该函数的功能是：
- 在优先级表中，将相应的位置位。（形参是优先级）

```c
/* 在优先级表的相应位置置位 */
void OS_PrioInsert (OS_PRIO prio)
{
	CPU_DATA	bit;
	CPU_DATA	bit_nbr;
	OS_PRIO		ix;
	
	/* 求模，获取优先级表数组的下标 */
	ix 		= prio / DEF_INT_CPU_NBR_BITS;
	
	/* 求余，将优先级限制在 DEF_INT_CPU_NBR_BITS 内 */
	bit_nbr = (CPU_DATA)prio & (DEF_INT_CPU_NBR_BITS - 1u);		/* 等价于 prio % DEF_INT_CPU_NBR_BITS */
																/* X % (2^N) = X & (2^N-1) */
	bit 	= 1u;
	
	/* 获取优先级在优先级表中对应的位的位置 */
	bit   <<= (DEF_INT_CPU_NBR_BITS - 1u) - bit_nbr;
	
	/* 将优先级在优先级表中对应的位置 1 */
	OSPrioTbl[ix] |= bit;
}
```

我们定义的 CPU 字长为 32，支持最大优先级为 32，则<code>DEF\_INT\_CPU\_NBR\_BITS = 32</code>，<code>OS\_CFG\_PRIO\_MAX = 32</code>，则优先级表中只有一个元素。那么假设现在 prio = 3：
- 求模：ix = 3 / 32 = 0，说明优先级为 3 在第一个数组元素 OSPrioTbl[0] 中。
- 求余：bit_nbr = 3 & 31 = 3。
- 获取位置：bit <<= 31 - 3 = 28，在 OSPrioTbl[0] 的位 28 置 1 即可。

若 prio = 35：
- 求模：ix = 35 / 32 = 1，说明优先级为 35 在第二个数组元素 OSPrioTbl[1] 中。
- 求余：bit_nbr = 35 & 31 = 4。
- 获取位置：bit <<= 31 - 4 = 27，在 OSPrioTbl[1] 的位 27 置 1 即可。

由上面的例子可以看到，该函数没有防止优先级溢出的情况发生。目前我不太清楚 uCOS 有没有一种机制可以防止这种非法的情况发生。

在优先级最大是 32，DEF\_INT\_CPU\_NBR\_BITS 等于 32 的情况下，如果分别创建了优先级 3、5、8 和 11 这四个任务，任务创建成功后，优先级表的设置情况可见下图。有一点要注意的是，在 uCOS 中，**最高优先级和最低优先级是留给系统任务使用的，用户任务不能使用。**

> 【位运算求余——为什么求余操作可以这么写？】
> 
> 应该有同学注意到了，求余的那行代码居然不用 % ，用的是 & 运算符，这是什么原理呢？先提前声明：**这个原理仅适用于 2 的幂次方。**
> 
> 我们知道，一个数除以一个形如 2^N 的数，可等价于这个数右移 N 位：
> 
> <code>X / 2^N = X >> N</code>
> 
> - 比如 11 / 2 = 5，11 的二进制为 1011，右移一位为 0101，也就是 5。现在请注意，被移走的消失不见的数是什么？是二进制下的 1，也就是十进制的 1，它就是余数；
> 
> - 再比如 11 / 8 = 1，11 的二进制为 1011，右移三位为 0001，也就是 1。被移走消失不见的数是二进制下的 011，也就是十进制的 3，它就是余数；
> 
> - 再比如 14 / 4 = 3，15 的二进制为 1110，右移两位为 0011，也就是 3。被移走消失不见的数是二进制下的 10，也就是十进制的 2，它就是余数。
> 
> 你发现了什么吗？发现其实我们移走的数字就是余数。现在我们求余，想要保留的就是这个余数。那如何得到呢？还需要仔细观察上面这三个例子。
> 
> - 第一个例子：对 1011 右移一位，仅最后一位保留，保留的是 1；
> - 第二个例子：对 1011 右移三位，最后三位保留，保留的是 011；
> - 第三个例子：对 1110 右移两位，最后两位保留，保留的是 10。
> 
> 注意，消失的数是我们想要保留的数，因此，可以将其等价为：
> 
> - 第一个例子： 1011 & 0001，仅最后一位保留，保留的是 1；
> - 第二个例子： 1011 & 0111，最后三位保留，保留的是 011；
> - 第三个例子： 1110 & 0011，最后两位保留，保留的是 10。
> 
> 也就是可以看做：
> 
> - 第一个例子： 1011 & (2-1)，仅最后一位保留，保留的是 1；
> - 第二个例子： 1011 & (8-1)，最后三位保留，保留的是 011；
> - 第三个例子： 1110 & (4-1)，最后两位保留，保留的是 10。
> 
> 你看懂了么？所以对 2 的幂次方求余，**可以等价为：X & (2^N - 1)**。这样做能加快运算速度。记住，该结论只对 2 的幂次方适用！！！
> 