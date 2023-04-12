参考内容：《[野火]uCOS-III内核实现与应用开发实战指南——基于STM32》第 11 章。

[toc]

## 1 就绪列表和任务控制块的定义（os.h）
### 1.1 任务控制块链表 OS_TCB

在定义就绪列表之前，先修改一下 TCB 的内容。

TCB 是一条双向链表，每个节点都包含以下内容：
- 任务栈指针 StkPtr；
- 任务栈大小 StkSize；
- 任务阻塞时长 TaskDelayTicks；
- 任务的优先级 Prio；
- 前向指针域 PrevPtr 和后向指针域 NextPtr。

```c
/*----------------------TCB---------------------------*/
/* TCB 重命名为大写字母格式 */
typedef struct os_tcb	OS_TCB;

/* TCB 数据类型声明 */
struct os_tcb{
	CPU_STK			*StkPtr;
	CPU_STK_SIZE	StkSize;
	
	OS_TICK			TaskDelayTicks;		/* 任务延时多少个 Ticks，注意 1 个 Ticks 为 10ms */
	
	OS_PRIO			Prio;
	
	OS_TCB			*NextPtr;
	OS_TCB			*PrevPtr;
};
```

### 1.2 就绪列表 OS_RDY_LIST

和 TCB 不同，os\_rdy\_list 不是一个链表，它只是一个纯粹的结构体，存储着：
- TCB 链表的头结点 HeadPtr；
- TCB 链表的尾结点 TailPtr；
- TCB 链表的节点个数 NbrEntries，即有多少个任务。

```c
/*---------------------OS_RDY_LIST----------------------------*/
/* 就绪列表重命名为大写字母格式 */
typedef struct os_rdy_list	OS_RDY_LIST;

/* 就绪列表数据类型声明，将 TCB 串成双向链表 */
struct os_rdy_list{
	OS_TCB		*HeadPtr;
	OS_TCB		*TailPtr;
	OS_OBJ_QTY	NbrEntries;		/* 同一个索引下有多少个任务 */
};
```

### 1.3 全局变量定义

- 就绪列表数组 OSRdyList：数组元素个数是我们定义好的最大支持优先级。比如我们定义了支持的优先级是 32，则就绪列表数组有 32 个元素；

这意味着，每个数组元素都将存放与 TCB 相关的信息：TCB 链表的头指针、尾指针以及节点个数，我们可以视为每个数组元素存放着一条 TCB 双向链表。

- 当前优先级 OSPrioCur；
- 最高优先级 OSPrioHighRdy；
- 当前任务 OSTCBCurPtr；
- 最高优先级任务 OSTCBHighRdyPtr。

```c
OS_EXT 	OS_TCB			*OSTCBCurPtr;
OS_EXT	OS_TCB			*OSTCBHighRdyPtr;

OS_EXT	OS_PRIO			OSPrioCur;		/* 当前优先级 */
OS_EXT	OS_PRIO			OSPrioHighRdy;	/* 最高优先级 */

OS_EXT	OS_RDY_LIST		OSRdyList[OS_CFG_PRIO_MAX];
```

### 1.4 结构全图

如图所示，每个优先级对应一个双向链表，执行任务时，当最高优先级上有多个就绪任务时，执行的顺序是从双向链表的头到尾。

【图片】

## 2 初始化就绪列表 OS_RdyListInit()（os_core.c）

该函数的功能：
- 将就绪列表初始化为空：所有信息全部清零。

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
		p_rdy_list->NbrEntries = (OS_OBJ_QTY)0;
	}
}
```

## 3 将 TCB 节点从链表头部移到链表尾部 OS_RdyListMoveHeadToTail()（os_core.c）

该函数的功能：
- 将任务 TCB 插入到就绪列表中。

所以要完成的步骤为：
- 根据任务的优先级，将优先级表中的相应位置置位；
- 根据优先级的大小，插入到就绪列表的相应位置：如果优先级等于当前优先级，则将当前任务 TCB 插入链表尾部；否则，将其插入到链表头部。

```c
/* 在就绪列表中插入一个 TCB */
void OS_RdyListInsert (OS_TCB *p_tcb)
{	
	OS_PrioInsert (p_tcb->Prio);  /* 将优先级插入当前优先级中 */
	
	if (p_tcb->Prio == OSPrioCur)
	{
		OS_RdyListInsertTail (p_tcb);  /* 如果优先级等于当前优先级，则将当前任务插入链表尾部 */
	}
	else
	{
		OS_RdyListInsertHead (p_tcb);	/* 否则将当前任务插入链表头部 */
	}
}
```

为此，需要实现插入链表头部和插入链表尾部的函数。

### 3.1 链表头部插入一个 TCB 节点 OS_RdyListInsertHead()（os_core.c）

```c
/* 链表头部插入一个 TCB */
void OS_RdyListInsertHead (OS_TCB *p_tcb)
{
	OS_RDY_LIST	*p_rdy_list;
	OS_TCB		*p_tcb2;
	
	p_rdy_list = &OSRdyList[p_tcb->Prio];
	
	if (p_rdy_list->NbrEntries == (OS_OBJ_QTY)0)	/* 若链表为空 */
	{
		p_rdy_list->NbrEntries = (OS_OBJ_QTY)1;
		p_rdy_list->HeadPtr = p_tcb;
		p_rdy_list->TailPtr = p_tcb;
		p_tcb->NextPtr = (OS_TCB *)0;
		p_tcb->PrevPtr = (OS_TCB *)0;
	}
	else		/* 若链表不为空 */
	{
		p_rdy_list->NbrEntries++;
		p_tcb2 = p_rdy_list->HeadPtr;		/* p_tcb2 = 原先 TCB 链表的头结点 */
		
		p_tcb->PrevPtr = (OS_TCB *)0;		/* 新的 p_tcb 的前指针为 0 */
		p_tcb->NextPtr = p_rdy_list->HeadPtr;
		p_tcb2->PrevPtr = p_tcb;			/* p_tcb2 的前指针指向 p_tcb，即新的 p_tcb 加入 TCB 链表中，成为新的头结点 */
		
		p_rdy_list->HeadPtr = p_tcb;		/* 新加入一个 TCB 后，就绪列表的 HeadPtr指向新的头结点 */
	}
}
```

### 3.2 链表尾部插入一个 TCB 节点 OS_RdyListInsertTail()（os_core.c）

```c
/* 链表尾部插入一个 TCB */
void OS_RdyListInsertTail (OS_TCB *p_tcb)
{
	OS_RDY_LIST	*p_rdy_list;
	OS_TCB		*p_tcb2;
	
	p_rdy_list = &OSRdyList[p_tcb->Prio];
	
	if (p_rdy_list->NbrEntries == (OS_OBJ_QTY)0)	/* 若链表为空 */
	{
		p_rdy_list->NbrEntries = (OS_OBJ_QTY)1;
		p_rdy_list->HeadPtr = p_tcb;
		p_rdy_list->TailPtr = p_tcb;
		p_tcb->NextPtr = (OS_TCB *)0;
		p_tcb->PrevPtr = (OS_TCB *)0;
	}
	else		/* 若链表不为空 */
	{
		p_rdy_list->NbrEntries++;
		p_tcb2 = p_rdy_list->TailPtr;		/* p_tcb2 = 原先 TCB 链表的尾结点 */
		
		p_tcb->PrevPtr = p_tcb2;			/* 新的 p_tcb 的前指针指向 p_tcb2 */
		p_tcb->NextPtr = (OS_TCB *)0;		/* 新的 p_tcb 的后指针为 0 */
		p_tcb2->NextPtr = p_tcb;			/* p_tcb2 的后指针指向 p_tcb，即新的 p_tcb 加入 TCB 链表中，成为新的尾结点 */
		
		p_rdy_list->TailPtr = p_tcb;		/* 新加入一个 TCB 后，就绪列表的 HeadPtr指向新的尾结点 */
	}
}
```

## 4 链表移除一个 TCB 节点 OS_RdyListRemove()（os_core.c）
