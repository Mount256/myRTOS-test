#include "os.h"

void OSTimeTick (void)
{
	/* ÿ���һ���� Tick �����һ�������л������ÿ������ƽ������ CPU ����Ȩ */
	OSSched();  
}
