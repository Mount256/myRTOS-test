参考内容：《[野火]uCOS-III内核实现与应用开发实战指南——基于STM32》第 13 章。

[toc]

## 1 数据类型定义和宏定义
### 1.1 时基列表相关宏定义和全局变量（os_cfg_app.h/c & os.h）

在 os\_cfg\_app.h 中，宏定义时基列表的大小，其推荐值为<code>任务数/4</code>，推荐使用质数，不推荐使用偶数。如果算出来的大小是偶数，则需加 1 变成质数。

```c
/* 时基列表大小 */
#define OS_CFG_TICK_WHEEL_SIZE			17u
```

接下来在 os\_cfg\_app.c 中，定义两个全局变量，分别是时基列表（数组）和时基列表的大小。注意需要在 os.h 中声明。

```c
OS_TICK_SPOKE		OSCfg_TickWheel[OS_CFG_TICK_WHEEL_SIZE];		/* 时基列表 */
OS_OBJ_QTY	const	OSCfg_TickWheelSize = (OS_OBJ_QTY)OS_CFG_TICK_WHEEL_SIZE;	/* 时基列表大小 */
```

在 os.h 中定义 Tick 计数器，即 SysTick 周期计数器，记录系统启动到现在或者从上一次复位到现在经过了多少个 SysTick 周期。

```c
OS_EXT	OS_TICK			OSTickCtr;		/* SysTick 周期计数器 */
```

### 1.2 时基列表定义（os.h）

在 os.h 中定义时基列表结构体，其组成为：
- FirstPtr：用于指向 TCB 单向链表的第一个节点。时基列表的每个数组元素都记录着一条 TCB 单向链表的信息，被插入该条链表的 TCB 会按照延时时间做升序排列。
- NbrEntries：记录 TCB 链表有多少个节点。
- NbrEntriesMax：记录 TCB 链表最多的时候有多少个节点，在节点增加时会刷新，在删除节点时不刷新。

```c
typedef struct os_tick_spoke 	OS_TICK_SPOKE;					/* 时基列表重命名为大写字母格式 */

struct os_tick_spoke{
	OS_TCB		*FirstPtr;			/* TCB 单向链表的头节点 */
	OS_OBJ_QTY	NbrEntries;			/* 单向链表有多少个节点 */
	OS_OBJ_QTY	NbrEntriesMax;		/* 单向链表最多时有多少个节点，该值会被刷新 */
};
```

### 1.3 修改 TCB 定义（os.h）

在 TCB 中新增加了 5 个成员，分别是：
- TickNextPtr：链表中指向下一个 TCB 节点。
- TickPrevPtr：链表中指向上一个 TCB 节点。
- TickSpokePtr：
- TickCtrMatch：
- TickRemain：

```c
struct os_tcb{
	CPU_STK			*StkPtr;
	CPU_STK_SIZE	StkSize;
	
	//OS_TICK			TaskDelayTicks;		/* 任务延时周期个数 */
	
	OS_PRIO			Prio;				/* 任务优先级 */
	
	OS_TCB			*NextPtr;			/* 就绪列表双向链表的下一个指针 */
	OS_TCB			*PrevPtr;			/* 就绪列表双向链表的前一个指针 */
	
	OS_TCB			*TickNextPtr;		/* 指向链表的下一个 TCB 节点 */
	OS_TCB			*TickPrevPtr;		/* 指向链表的上一个 TCB 节点 */
	OS_TICK_SPOKE	*TickSpokePtr;		/* 用于回指到链表根部 */
	
	OS_TICK			TickCtrMatch;		/* 该值等于时基计数器 OSTickCtr 的值加上 TickRemain 的值 */
	OS_TICK			TickRemain;			/* 设置任务还需要等待多少个时钟周期 */
};
```

## 2 时基列表的相关函数
### 2.1 初始化时基列表 OS_TickListInit()（os_tick.c）

