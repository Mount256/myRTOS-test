#include "os.h"

void OSTimeTick (void)
{
	/* 每间隔一定的 Tick 会进行一次任务切换，因此每个任务平等享有 CPU 控制权 */
	OSSched();  
}
