#ifndef OS_CFG_H
#define OS_CFG_H


/* 支持最大的优先级 */
#define OS_CFG_PRIO_MAX		32u

/* 使能时间戳 */
#define OS_CFG_TS_EN		1u

/* 使能时间片轮转 */
#define OS_CFG_SCHED_ROUND_ROBIN_EN			1u

/* 使能任务挂起功能 */
#define OS_CFG_TASK_SUSPENDED_EN          	1u

/* 使能任务删除功能 */
#define OS_CFG_TASK_DEL_EN					1u

#endif /* OS_CFG_H */
