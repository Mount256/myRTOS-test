;**********函数/变量声明**********
	IMPORT  OSTCBCurPtr
	IMPORT	OSTCBHighRdyPtr
	IMPORT	OSPrioCur
	IMPORT	OSPrioHighRdy
	EXPORT	OSStartHighRdy
	EXPORT	PendSV_Handler

;**********常量**********
NVIC_INT_CTRL		EQU 	0xE000ED04		; 中断控制及状态寄存器 SCB_ICSR
NVIC_SYSPRI14		EQU		0xE000ED22		; 系统优先级寄存器 SCB_SHPR3：bit 16~23
NVIC_PENDSV_PRI		EQU		0xFF			; PendSV 优先级的值(最低)
NVIC_PENDSVSET		EQU		0x10000000		; 触发 PendSV 异常的值 Bit28：PENDSVSET
	
;************************
	PRESERVE8
	THUMB
		
	AREA CODE, CODE, READONLY

;**********开始进行第一次任务切换**********
OSStartHighRdy PROC

	; 配置 PendSV 的优先级为 0XFF，即最低，防止中断服务程序进行上下文切换，即中断服务程序不允许中断
	LDR 	R0, = NVIC_SYSPRI14				; 系统优先级寄存器 SCB_SHPR3：bit 16~23	
	LDR		R1, = NVIC_PENDSV_PRI
	STRB	R1, [R0]

	; 设置 PSP 的值为 0，开始第一个任务切换
	; 在任务中，使用的栈指针都是 PSP，后面如果判断出 PSP 为 0，则表示第一次任务切换
	MOVS	R0, #0		
	MSR		PSP, R0

	; 触发 PendSV 异常，如果中断启用且有编写 PendSV 异常服务函数的话，
	; 则内核会响应 PendSV 异常，去执行 PendSV 异常服务函数
	LDR		R0, = NVIC_INT_CTRL			; 中断控制及状态寄存器 SCB_ICSR 的地址
	LDR		R1, = NVIC_PENDSVSET		; 触发 PendSV 异常的值 Bit28：PENDSVSET
	STR		R1, [R0]

	; 开中断
	CPSIE	I

	; 程序永远不会执行到这
OSStartHang
	B		OSStartHang

	ENDP

;**********PendSVHandler异常**********
PendSV_Handler PROC
	
	CPSID	I					; 关中断，防止上下文切换

	MRS		R0, PSP				; 将 PSP 加载到 R0，MRS 是 ARM 32 位数据加载指令，
								; 功能是加载特殊功能寄存器的值到通用寄存器
	CBZ		R0, OS_CPU_PendSVHandler_nosave ; 判断 R0，如果值为 0 则跳转到 OS_CPU_PendSVHandler_nosave
	                                        ; 进行第一次任务切换的时候，R0 肯定为 0
	
	STMDB	R0!, {R4-R11}		; 手动存储 R4-R11 寄存器到当前任务栈中，而其他寄存器会被 CPU 自动入栈
	LDR		R1, = OSTCBCurPtr	; 将 OSTCBCurPtr 指针的地址加载到 R1
	LDR		R1, [R1]			; 将 OSTCBCurPtr 指针加载到 R1
	STR		R0, [R1]			; 存储 R0（任务栈栈顶）的值到 OSTCBCurPtr(->StkPtr) 

OS_CPU_PendSVHandler_nosave

	; 使 OSPrioCur = OSPrioHighRdy
	LDR		R0, = OSPrioCur		; 将 OSPrioCur 指针的地址加载到 R0
	LDR		R1, = OSPrioHighRdy	; 将 OSPrioHighRdy 指针的地址加载到 R1
	LDR		R2, [R1]			; 将 OSPrioCur 指针加载到 R2
	STR		R2, [R0]			; 将 OSPrioHighRdy（R2）存到 OSPrioCur（R0）

	; 使 OSTCBCurPtr = OSTCBHighRdyPtr 
	LDR		R0, = OSTCBCurPtr		; 将 OSTCBCurPtr 指针的地址加载到 R0
	LDR		R1, = OSTCBHighRdyPtr	; 将 OSTCBHighRdyPtr 指针的地址加载到 R1
	LDR		R2, [R1]			; 将 OSTCBCurPtr 指针加载到 R2
	STR		R2, [R0]			; 将 OSTCBHighRdyPtr（R2）存到 OSTCBCurPtr（R0）
	
	LDR     R0, [R2]            ; 加载 OSTCBHighRdyPtr(->StkPtr) 到 R0
	LDMIA   R0!, {R4-R11}       ; 加载需要手动保存的信息到 CPU 寄存器 R4-R11，其他寄存器将在返回后由 CPU 自动装载
	
	MSR     PSP, R0             ; 更新PSP的值，这个时候PSP指向下一个要执行的任务的堆栈的栈底（这个栈底已经加上刚刚手动加载到CPU寄存器R4-R11的偏移）
	ORR     LR, LR, #0x04       ; 确保异常返回使用的堆栈指针是PSP，即LR寄存器的位2要为1
	CPSIE   I                   ; 开中断
	BX      LR                  ; 异常返回，这个时候任务堆栈中的剩下内容将会自动加载到xPSR，PC（任务入口地址），R14，R12，R3，R2，R1，R0（任务的形参）
	                            ; 同时PSP的值也将更新，即指向任务堆栈的栈顶。在STM32中，堆栈是由高地址向低地址生长的。
	
	NOP                         ; 为了汇编指令对齐，不然会有警告
	
	
	ENDP  

;**********关中断（NMI 和硬FAULT 除外）**********

AllIntDis  PROC
	CPSID   I
	
	ENDP
	
	NOP                                       ; 为了汇编指令对齐，不然会有警告
	END                                       ; 汇编文件结束
