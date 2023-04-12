    EXPORT  CPU_IntDis
    EXPORT  CPU_IntEn

	AREA |.text|, CODE, READONLY, ALIGN=2
    THUMB
    REQUIRE8
    PRESERVE8

CPU_IntDis
	CPSID 	I	; 关中断
	BX		LR
	
CPU_IntEn
	CPSIE	I	; 开中断
	BX	LR
	
	
	END
