#include "System.h"
#include "Serial.h"
#include "Gfx.h"
#include <psx.h>
#include <stdio.h>

void SystemInit(void)
{
#if 0
	PSX_InitEx(PSX_INIT_SAVESTATE | PSX_INIT_CD);
#else
	PSX_InitEx(0);
#endif
	SerialInit();
	GfxInit();
}
