    EXPORT  CPU_IntDis
    EXPORT  CPU_IntEn
		
	EXPORT	CPU_SR_Save
	EXPORT 	CPU_SR_Restore
		
	EXPORT  CPU_CntLeadZeros
	EXPORT	CPU_CntTrailZeros

	AREA |.text|, CODE, READONLY, ALIGN=2
    THUMB
    REQUIRE8
    PRESERVE8
		
;********************************************************************************************************
;                                    失能/使能中断
;********************************************************************************************************

; void 	CPU_IntDis		(void);
CPU_IntDis		; 失能中断
	CPSID 	I	; 关中断
	BX		LR
	
; void 	CPU_IntEn		(void);
CPU_IntEn		; 使能中断
	CPSIE	I	; 开中断
	BX	LR
		
;********************************************************************************************************
;                                      临界段开启/关闭中断
;
; 		PRIMASK 是个只有单一比特的寄存器。在它被置 1 后，就关掉所有可屏蔽的异常，只剩下 NMI 和
; 		硬件 FAULT 可以响应。它的默认值是 0，表示没有关中断。
;********************************************************************************************************
	
; CPU_SR  CPU_SR_Save (void);  （临界段关中断，R0 为返回值）
CPU_SR_Save		
	MRS		R0, PRIMASK		; 将 PRIMASK 寄存器的值存入 R0 中
	CPSID 	I	; 关中断
	BX		LR
	
; void CPU_SR_Restore (CPU_SR  cpu_sr);   （临界段开中断，R0 为形参）
CPU_SR_Restore	
	MSR		PRIMASK, R0		; 将 R0 的值存入 PRIMASK 寄存器中
	BX		LR


;********************************************************************************************************
;                                    前导零/后导零函数
;********************************************************************************************************

; CPU_DATA CPU_CntLeadZeros (CPU_DATA val);  （前导零）
CPU_CntLeadZeros
	CLZ		R0, R0
	BX		LR
	
; CPU_DATA CPU_CntTrailZeros (CPU_DATA val);  （后导零）
CPU_CntTrailZeros
	RBIT	R0, R0
	CLZ 	R0, R0
	BX 		LR

	END
