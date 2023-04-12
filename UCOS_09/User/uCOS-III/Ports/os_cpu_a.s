;**********����/��������**********
	IMPORT  OSTCBCurPtr
	IMPORT	OSTCBHighRdyPtr
	IMPORT	OSPrioCur
	IMPORT	OSPrioHighRdy
	EXPORT	OSStartHighRdy
	EXPORT	PendSV_Handler

;**********����**********
NVIC_INT_CTRL		EQU 	0xE000ED04		; �жϿ��Ƽ�״̬�Ĵ��� SCB_ICSR
NVIC_SYSPRI14		EQU		0xE000ED22		; ϵͳ���ȼ��Ĵ��� SCB_SHPR3��bit 16~23
NVIC_PENDSV_PRI		EQU		0xFF			; PendSV ���ȼ���ֵ(���)
NVIC_PENDSVSET		EQU		0x10000000		; ���� PendSV �쳣��ֵ Bit28��PENDSVSET
	
;************************
	PRESERVE8
	THUMB
		
	AREA CODE, CODE, READONLY

;**********��ʼ���е�һ�������л�**********
OSStartHighRdy PROC

	; ���� PendSV �����ȼ�Ϊ 0XFF������ͣ���ֹ�жϷ����������������л������жϷ�����������ж�
	LDR 	R0, = NVIC_SYSPRI14				; ϵͳ���ȼ��Ĵ��� SCB_SHPR3��bit 16~23	
	LDR		R1, = NVIC_PENDSV_PRI
	STRB	R1, [R0]

	; ���� PSP ��ֵΪ 0����ʼ��һ�������л�
	; �������У�ʹ�õ�ջָ�붼�� PSP����������жϳ� PSP Ϊ 0�����ʾ��һ�������л�
	MOVS	R0, #0		
	MSR		PSP, R0

	; ���� PendSV �쳣������ж��������б�д PendSV �쳣�������Ļ���
	; ���ں˻���Ӧ PendSV �쳣��ȥִ�� PendSV �쳣������
	LDR		R0, = NVIC_INT_CTRL			; �жϿ��Ƽ�״̬�Ĵ��� SCB_ICSR �ĵ�ַ
	LDR		R1, = NVIC_PENDSVSET		; ���� PendSV �쳣��ֵ Bit28��PENDSVSET
	STR		R1, [R0]

	; ���ж�
	CPSIE	I

	; ������Զ����ִ�е���
OSStartHang
	B		OSStartHang

	ENDP

;**********PendSVHandler�쳣**********
PendSV_Handler PROC
	
	CPSID	I					; ���жϣ���ֹ�������л�

	MRS		R0, PSP				; �� PSP ���ص� R0��MRS �� ARM 32 λ���ݼ���ָ�
								; �����Ǽ������⹦�ܼĴ�����ֵ��ͨ�üĴ���
	CBZ		R0, OS_CPU_PendSVHandler_nosave ; �ж� R0�����ֵΪ 0 ����ת�� OS_CPU_PendSVHandler_nosave
	                                        ; ���е�һ�������л���ʱ��R0 �϶�Ϊ 0
	
	STMDB	R0!, {R4-R11}		; �ֶ��洢 R4-R11 �Ĵ�������ǰ����ջ�У��������Ĵ����ᱻ CPU �Զ���ջ
	LDR		R1, = OSTCBCurPtr	; �� OSTCBCurPtr ָ��ĵ�ַ���ص� R1
	LDR		R1, [R1]			; �� OSTCBCurPtr ָ����ص� R1
	STR		R0, [R1]			; �洢 R0������ջջ������ֵ�� OSTCBCurPtr(->StkPtr) 

OS_CPU_PendSVHandler_nosave

	; ʹ OSPrioCur = OSPrioHighRdy
	LDR		R0, = OSPrioCur		; �� OSPrioCur ָ��ĵ�ַ���ص� R0
	LDR		R1, = OSPrioHighRdy	; �� OSPrioHighRdy ָ��ĵ�ַ���ص� R1
	LDR		R2, [R1]			; �� OSPrioCur ָ����ص� R2
	STR		R2, [R0]			; �� OSPrioHighRdy��R2���浽 OSPrioCur��R0��

	; ʹ OSTCBCurPtr = OSTCBHighRdyPtr 
	LDR		R0, = OSTCBCurPtr		; �� OSTCBCurPtr ָ��ĵ�ַ���ص� R0
	LDR		R1, = OSTCBHighRdyPtr	; �� OSTCBHighRdyPtr ָ��ĵ�ַ���ص� R1
	LDR		R2, [R1]			; �� OSTCBCurPtr ָ����ص� R2
	STR		R2, [R0]			; �� OSTCBHighRdyPtr��R2���浽 OSTCBCurPtr��R0��
	
	LDR     R0, [R2]            ; ���� OSTCBHighRdyPtr(->StkPtr) �� R0
	LDMIA   R0!, {R4-R11}       ; ������Ҫ�ֶ��������Ϣ�� CPU �Ĵ��� R4-R11�������Ĵ������ڷ��غ��� CPU �Զ�װ��
	
	MSR     PSP, R0             ; ����PSP��ֵ�����ʱ��PSPָ����һ��Ҫִ�е�����Ķ�ջ��ջ�ף����ջ���Ѿ����ϸո��ֶ����ص�CPU�Ĵ���R4-R11��ƫ�ƣ�
	ORR     LR, LR, #0x04       ; ȷ���쳣����ʹ�õĶ�ջָ����PSP����LR�Ĵ�����λ2ҪΪ1
	CPSIE   I                   ; ���ж�
	BX      LR                  ; �쳣���أ����ʱ�������ջ�е�ʣ�����ݽ����Զ����ص�xPSR��PC��������ڵ�ַ����R14��R12��R3��R2��R1��R0��������βΣ�
	                            ; ͬʱPSP��ֵҲ�����£���ָ�������ջ��ջ������STM32�У���ջ���ɸߵ�ַ��͵�ַ�����ġ�
	
	NOP                         ; Ϊ�˻��ָ����룬��Ȼ���о���
	
	
	ENDP  

;**********���жϣ�NMI ��ӲFAULT ���⣩**********

AllIntDis  PROC
	CPSID   I
	
	ENDP
	
	NOP                                       ; Ϊ�˻��ָ����룬��Ȼ���о���
	END                                       ; ����ļ�����
