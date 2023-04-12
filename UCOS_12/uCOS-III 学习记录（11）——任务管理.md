参考内容：《[野火]uCOS-III内核实现与应用开发实战指南——基于STM32》第 15、16 和 21 章。

[toc]

## 1 任务状态

在 uCOS 中，任务状态分为以下几种：

- **就绪（<code>OS\_TASK\_STATE\_RDY</code>）**：该任务在就绪列表中，就绪的任务已经具备执行的能力，只等待调度器进行调度，新创建的任务会初始化为就绪态。
- **延时（<code>OS\_TASK\_STATE\_DLY</code>）**：该任务处于延时调度状态。
- **等待（<code>OS\_TASK\_STATE\_PEND</code>）**：任务调用 OSQPend()、OSSemPend() 这类等待函数，系统
就会设置一个超时时间让该任务处于等待状态，如果超时时间设置为 0，任务的状态，无限期等下去，直到事件发生。如果超时时间为 N(N>0)，在 N 个时间内任务等待的事件或信号都没发生，就退出等待状态转为就绪状态。**（现阶段忽视）**
- **运行（<code>OS\_TASK\_STATE\_PEND\_TIMEOUT</code>）**：该状态表明任务正在执行，此时它占用处理器，uCOS 调度器选择运行的永远是处于最高优先级的就绪态任务，当任务被运行的一刻，它的任务状态就变成了运行态，其实运行态的任务也是处于就绪列表中的。
- **挂起（<code>OS\_TASK\_STATE\_SUSPENDED</code>）**：任务通过调用 OSTaskSuspend() 函数能够挂起自己或其他任务，调用 OSTaskResume() 是使被挂起的任务回复运行的唯一的方法。挂起一任务意味着该任务再被恢复运行以前不能够取得 CPU 的使用权，类似强行暂停一个任务。
- **延时+挂起（<code>OS\_TASK\_STATE\_DLY\_SUSPENDED</code>）**：任务先产生一个延时，延时没结束的时候被其他任务挂起，挂起的效果叠加，当且仅当延时结束并且挂起被恢复了，该任务才能够再次运行。
- **等待+挂起（<code>OS\_TASK\_STATE\_PEND\_SUSPENDED</code>）**：任务先等待一个事件或信号的发生（无限期等待），还没等待到就被其他任务挂起，挂起的效果叠加，当且仅当任务等待到事件或信号并且挂起被恢复了，该任务才能够再次运行。**（现阶段忽视）**
- **超时等待+挂起（<code>OS\_TASK\_STATE\_PEND\_TIMEOUT\_SUSPENDED</code>）**：任务在指定时间内等待事件或信号的产生，但是任务已经被其他任务挂起。**（现阶段忽视）**
- **删除（<code>OS\_TASK\_STATE\_DEL</code>）**：任务被删除后的状态，任务被删除后将不再运行，除非重新创建任务。

在 os.h 中宏定义了任务的状态值：

```c
/* 系统状态 */
#define  OS_STATE_OS_STOPPED                    (OS_STATE)(0u)
#define  OS_STATE_OS_RUNNING                    (OS_STATE)(1u)
	
/* 任务状态 */
#define	 OS_TASK_STATE_BIT_DLY					(OS_STATE)(0x01u)	/* 挂起位      				*/
#define	 OS_TASK_STATE_BIT_PEND					(OS_STATE)(0x02u)	/* 等待位      				*/
#define	 OS_TASK_STATE_BIT_SUSPENDED			(OS_STATE)(0x04u)	/* 延时/超时位 				*/
	
#define  OS_TASK_STATE_RDY						(OS_STATE)(   0u)	/* 0 0 0  就绪 				*/
#define  OS_TASK_STATE_DLY						(OS_STATE)(   1u)	/* 0 0 1  延时/超时 			*/
#define  OS_TASK_STATE_PEND						(OS_STATE)(   2u)	/* 0 1 0  等待	 			*/
#define  OS_TASK_STATE_PEND_TIMEOUT				(OS_STATE)(   3u)	/* 0 1 1  等待+超时 			*/
#define  OS_TASK_STATE_SUSPENDED				(OS_STATE)(   4u)	/* 1 0 0  挂起 				*/
#define  OS_TASK_STATE_DLY_SUSPENDED			(OS_STATE)(   5u)	/* 1 0 1  挂起+延时/超时 	*/
#define  OS_TASK_STATE_PEND_SUSPENDED			(OS_STATE)(   6u)	/* 1 1 0  挂起+等待		 	*/
#define  OS_TASK_STATE_PEND_TIMEOUT_SUSPENDED	(OS_STATE)(   7u)	/* 1 1 1  挂起+超时+等待 	*/
#define  OS_TASK_STATE_DEL						(OS_STATE)( 255u)	
```

## 2 修改和添加相关代码
### 2.1 修改 TCB（os.h）

TCB 中增加两个成员：
- TaskState：标志任务的状态。
- SuspendCtr：记录任务被挂起了几次。一个任务挂起多少次就要被恢复多少次才能重新运行。

```c
struct os_tcb{
	CPU_STK			*StkPtr;
	CPU_STK_SIZE	StkSize;
	
	OS_PRIO			Prio;				/* 任务优先级 */
	
	OS_TCB			*NextPtr;			/* 就绪列表双向链表的下一个指针 */
	OS_TCB			*PrevPtr;			/* 就绪列表双向链表的前一个指针 */
	
	OS_TCB			*TickNextPtr;		/* 指向链表的下一个 TCB 节点 */
	OS_TCB			*TickPrevPtr;		/* 指向链表的上一个 TCB 节点 */
	OS_TICK_SPOKE	*TickSpokePtr;		/* 用于回指到链表根部 */
	OS_TICK			TickCtrMatch;		/* 该值等于时基计数器 OSTickCtr 的值加上 TickRemain 的值 */
	OS_TICK			TickRemain;			/* 设置任务还需要等待多少个时钟周期 */
	
	OS_TICK			TimeQuanta;			/* 任务需要多少个时间片 */
	OS_TICK			TimeQuantaCtr;		/* 任务剩余的时间片个数 */
	
	OS_STATE		TaskState;			/* 表示任务的状态 */
	
#if OS_CFG_TASK_SUSPENDED_EN > 0u
	OS_NESTING_CTR	SuspendCtr;			/* 任务挂起函数 OSTaskSuspend() 计数器 */
#endif
};
```

### 2.2 添加宏定义和数据类型

在 os_cfg.h 中添加宏定义，用于使能任务挂起功能，该功能可以开启也可以关闭：

```c
/* 使能任务挂起功能 */
#define OS_CFG_TASK_SUSPENDED_EN          	1u
```

在 os_type.h 中增加数据类型：

```c
typedef   CPU_INT08U      OS_NESTING_CTR;
```

## 3 任务管理的函数
### 3.1 任务挂起函数

### 3.2 任务恢复函数

### 3.3 任务删除函数

