#include "System.h"
#include "Serial.h"
#include "Gfx.h"
#include <psx.h>
#include <stdio.h>

void SystemInit(void)
{
	redirect_stdio_to_sio();
	PSX_InitEx(0);
	SerialInit();
	GfxInit();
}

void SystemDeinit(void)
{
	PSX_DeInit();
}
