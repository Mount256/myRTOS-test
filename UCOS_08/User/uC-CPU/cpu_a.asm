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
;                                    ʧ��/ʹ���ж�
;********************************************************************************************************

; void 	CPU_IntDis		(void);
CPU_IntDis		; ʧ���ж�
	CPSID 	I	; ���ж�
	BX		LR
	
; void 	CPU_IntEn		(void);
CPU_IntEn		; ʹ���ж�
	CPSIE	I	; ���ж�
	BX	LR
		
;********************************************************************************************************
;                                      �ٽ�ο���/�ر��ж�
;
; 		PRIMASK �Ǹ�ֻ�е�һ���صļĴ������������� 1 �󣬾͹ص����п����ε��쳣��ֻʣ�� NMI ��
; 		Ӳ�� FAULT ������Ӧ������Ĭ��ֵ�� 0����ʾû�й��жϡ�
;********************************************************************************************************
	
; CPU_SR  CPU_SR_Save (void);  ���ٽ�ι��жϣ�R0 Ϊ����ֵ��
CPU_SR_Save		
	MRS		R0, PRIMASK		; �� PRIMASK �Ĵ�����ֵ���� R0 ��
	CPSID 	I	; ���ж�
	BX		LR
	
; void CPU_SR_Restore (CPU_SR  cpu_sr);   ���ٽ�ο��жϣ�R0 Ϊ�βΣ�
CPU_SR_Restore	
	MSR		PRIMASK, R0		; �� R0 ��ֵ���� PRIMASK �Ĵ�����
	BX		LR


;********************************************************************************************************
;                                    ǰ����/���㺯��
;********************************************************************************************************

; CPU_DATA CPU_CntLeadZeros (CPU_DATA val);  ��ǰ���㣩
CPU_CntLeadZeros
	CLZ		R0, R0
	BX		LR
	
; CPU_DATA CPU_CntTrailZeros (CPU_DATA val);  �����㣩
CPU_CntTrailZeros
	RBIT	R0, R0
	CLZ 	R0, R0
	BX 		LR

	END
