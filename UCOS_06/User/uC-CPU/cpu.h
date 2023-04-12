#ifndef CPU_H
#define CPU_H

/*********************************标准数据类型**********************************/

typedef unsigned short  CPU_INT16U;
typedef unsigned int  	CPU_INT32U;
typedef unsigned char 	CPU_INT08U;

typedef CPU_INT32U  	CPU_ADDR;

/*********************************CPU堆栈数据类型定义*********************************/
typedef CPU_INT32U		CPU_STK;
typedef CPU_ADDR		CPU_STK_SIZE;

/*********************************CPU寄存器数据类型定义*********************************/
typedef volatile 		CPU_INT32U		CPU_REG32;
typedef 				CPU_REG32		CPU_SR;

/*********************************临界段定义*********************************/

#define CPU_SR_ALLOC()			CPU_SR	cpu_sr = (CPU_SR)0
#define CPU_INT_DIS()			do { cpu_sr = CPU_SR_Save(); } while(0)
#define CPU_INT_EN()			do { CPU_SR_Restore(cpu_sr); } while(0)
#define CPU_CRITICAL_ENTER()	do { CPU_INT_DIS(); } while(0)
#define CPU_CRITICAL_EXIT()		do { CPU_INT_EN();  } while(0)

/*********************************函数声明(cpu_a.asm)*********************************/
void 	CPU_IntDis 		(void);
void 	CPU_IntEn		(void);
CPU_SR 	CPU_SR_Save 	(void);
void 	CPU_SR_Restore 	(CPU_SR  cpu_sr);

#endif /* CPU_H */

