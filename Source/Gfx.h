#ifndef __GFX_HEADER__
#define __GFX_HEADER__

/* *************************************
 * 	Includes
 * *************************************/

#include "Global_Inc.h"
#include "System.h"



/* *************************************
 * 	Defines
 * *************************************/

#define X_SCREEN_RESOLUTION 	384
#define Y_SCREEN_RESOLUTION 	240
#define VRAM_W					1024
#define VRAM_H					512
#define MAX_SIZE_FOR_GSSPRITE 	256
#define GFX_TPAGE_WIDTH 		64
#define GFX_TPAGE_HEIGHT 		256
#define GFX_1HZ_FLASH			(1<<7)
#define GFX_2HZ_FLASH			(1<<8)
#define FULL_LUMINANCE			0xFF

/* *************************************
 * 	Global prototypes
 * *************************************/

void GfxInitDrawEnv(void);
void GfxInitDispEnv(void);
void GfxSetPrimitiveList(void);

// Renders new scene. Use this function unless you know what you are doing!
void GfxDrawScene(void);

// Blocking version. Calls GfxDrawScene() and then adds a while(GfxIsBusy() )
// after it.
void GfxDrawScene_Slow(void);

void GfxDrawScene_NoSwap(void);

void GfxSwapBuffers(void);

// Only renders screen and does not update any pad data or timer data.
// To be used in ISR!
void GfxDrawScene_Fast(void);

// Repotedly, tells is GPU is ready for a DMA transfer.
bool GfxReadyForDMATransfer(void);

// Fills a GsSprite structure with information from a TIM file.
bool GfxSpriteFromFile(char* fname, GsSprite * spr);

// Reportedly, loads CLUT data from a TIM image (image data is discarded)
bool GfxCLUTFromFile(char* fname);

// Returns true if current object is within screen limits, false otherwise.
bool GfxIsInsideScreenArea(short x, short y, short w, short h);

// Function overload for GsSprite structures.
bool GfxIsSpriteInsideScreenArea(GsSprite * spr);

// Used to know whether GPU operation can be done.
bool GfxIsGPUBusy(void);

// Draws a sprite on screen. First, it checks whether sprite is inside
// screen limits.
void GfxSortSprite(GsSprite * spr);

uint8_t GfxGetGlobalLuminance(void);

void GfxSetGlobalLuminance(uint8_t value);

void GfxIncreaseGlobalLuminance(int8_t step);

void GfxButtonSetFlags(uint8_t flags);

void GfxButtonRemoveFlags(uint8_t flags);

int GfxRotateFromDegrees(int deg);

void GfxDrawButton(short x, short y, unsigned short btn);

// Sends current display data on a specific VRAM section and fills
// sprite structure pointed to by "spr".
void GfxSaveDisplayData(GsSprite *spr);

// Fills GsSprite structure pointed to by "spr" with texture page and U/V
// offset data given a position in VRAM.
bool GfxTPageOffsetFromVRAMPosition(GsSprite * spr, short x, short y);

void GfxSetSplitScreen(uint8_t playerIndex);

void GfxDisableSplitScreen(void);

void GfxDrawScene_NoSwap(void);

void GfxDevMenuEnable(void);

/* *************************************
 * 	Global variables
 * *************************************/

extern GsSprite PSXButtons;

#endif //__GFX_HEADER__
