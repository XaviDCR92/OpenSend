#include "System.h"
#include "Serial.h"
#include "Gfx.h"
#include <psx.h>

void SystemInit(void)
{
	PSX_InitEx(PSX_INIT_SAVESTATE | PSX_INIT_CD);
	SerialInit();
	GfxInit();
}
