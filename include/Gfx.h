#ifndef GFX_H
#define GFX_H

#include <psxgpu.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define SPRITE_INDEX_INVALID    (size_t)(0xFFFFFFFF)
#define MAX_SIZE_FOR_GSSPRITE   ((short)256)

enum
{
    GFX_TPAGE_WIDTH = 64,
    GFX_TPAGE_WIDTH_BITSHIFT = __builtin_ctz(GFX_TPAGE_WIDTH)
};

/* *************************************
 * Public types definition
 * *************************************/

enum
{
    X_SCREEN_RESOLUTION = 368,
    Y_SCREEN_RESOLUTION = 240
};

/* *************************************
 * Public variables declaration
 * *************************************/

/* *************************************
 * Public functions declaration
 * *************************************/

void GfxInit(void);
bool GfxSpriteFromFile(const char* path, GsSprite *spr);
void GfxSortSprite(GsSprite *spr);
bool GfxIsInsideScreenArea(short x, short y, short w, short h);
bool GfxIsSpriteInsideScreenArea(const GsSprite *spr);
void GfxDrawScene(void);
void GfxClear(void);
int GfxToDegrees(int rotate);
int GfxFromDegrees(int degrees);
bool GfxIsBusy(void);
void GfxDrawRectangle(GsRectangle *rect);
void GfxSaveDisplayData(GsSprite *const spr);

/** \} */

#ifdef __cplusplus
}
#endif

#endif /* GFX_H */
