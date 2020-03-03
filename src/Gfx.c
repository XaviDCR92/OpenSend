#include "Gfx.h"
#include "IO.h"
#include <psx.h>
#include <psxgpu.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdint.h>

/* The drawing environment points to VRAM
 * coordinates where primitive data is
 * being drawn onto. */
static GsDrawEnv sDrawEnv;

/* The display environment points to VRAM
 * coordinates where primitive data is
 * being shown on screen. */
static GsDispEnv sDispEnv;

/* This variable is set to true on VSYNC event. */
static volatile bool bSyncFlag;

static void GfxInitDrawEnv(void);
static void GfxInitDispEnv(void);
static void GfxSwapBuffers(void);
static void GfxSortBigSprite(GsSprite *const psSpr);
static void GfxSetPrimList(void);
static void ISR_VBlank(void);

/***************************************************************************//**
*
* \brief    Initialization of Gfx module.
*
* \remarks  This is where PSX GPU and its interface get initialized.
*
*******************************************************************************/
void GfxInit(void)
{
    /* Graphics synthetiser (GPU) initialization. */
    GsInit();

    /* Clear VRAM. */
    GsClearMem();

#if (VIDEO_MODE == VMODE_PAL) || (VIDEO_MODE == VMODE_NSTC)

    /* Set Video Resolution. VIDEO_MODE can be either VMODE_PAL or VMODE_NTSC */
    GsSetVideoMode(X_SCREEN_RESOLUTION, Y_SCREEN_RESOLUTION, VIDEO_MODE);

#else /* (VIDEO_MODE == VMODE_PAL) || (VIDEO_MODE == VMODE_NSTC) */

#error  "Undefined VIDEO_MODE"

#endif /* (VIDEO_MODE == VMODE_PAL) || (VIDEO_MODE == VMODE_NSTC) */

    /* Set Drawing Environment. */
    GfxInitDrawEnv();

    /* Set Display Environment. */
    GfxInitDispEnv();

    /* Set primitive list. */
    GfxSetPrimList();

    /* Set Vsync interrupt handler for screen refresh. */
    SetVBlankHandler(&ISR_VBlank);
}

/***************************************************************************//**
*
* \brief    Loads data from file indicated by strFilePath, uploads
*           it into VRAM and sets up a new GsSprite instance.
*
* \param    strFilePath
*               Absolute file path e.g.:
*               "cdrom:\\DATA\\SPRITES\\TILESET1.TIM;1".
*
* \param    pSpr
*               Pointer to sprite to be filled with image data.
*
* \return   Returns true when tasks could be made successfully,
*           false otherwise.
*
* \see      IOLoadFile() for file I/O handling implementation.
*
*******************************************************************************/
bool GfxSpriteFromFile(const char* const strFilePath, GsSprite *const pSpr)
{
    /* File size in bytes. Modified by IOLoadFile(). */
    size_t eSize;

    /* Get buffer address where file data is contained. */
    const uint8_t *const buffer = IOLoadFile(strFilePath, &eSize);

    if (buffer && (eSize != IO_INVALID_FILE_SIZE))
    {
        /* File was loaded successfully into buffer.
         * Now read buffer data and upload it to VRAM. */

        /* Declare a GsImage instance, needed by GsImageFromTim(). */
        GsImage sGsi;

        while (GsIsDrawing());

        if (GsImageFromTim(&sGsi, buffer) == 1 /* Success code. */)
        {
            enum
            {
                UPLOAD_IMAGE_FLAG = 1
            };

            /* sGsi is now filled with data. Create
             * a GsSprite instance from it. */

            /* Call PSXSDK libs to upload image data to VRAM. "const" flag must be removed. */
            if (GsSpriteFromImage(pSpr, &sGsi, UPLOAD_IMAGE_FLAG) == 1 /* Success code. */)
            {
                /* Return success code. */
                return true;
            }
            else
            {
                /* Something went wrong when obtaining data
                 * from GsImage instance. Fall through. */
            }
        }
        else
        {
            /* Something went wrong when obtaining *.TIM data.
             * Fall through. */
        }
    }
    else
    {
        /* Something went wrong when loading the file. Fall through. */
    }

    /* Return failure code if reached here. */
    return false;
}

void GfxClear(void)
{
    GsSortCls(0, 0, 0);
}

/***************************************************************************//**
*
* \brief    Draws current primitive list into screen and performs double
*           buffering.
*
* \remarks  Blocking function. This function waits for GPU to be free and GPU
*           VSYNC IRQ flag to be set.
*
*******************************************************************************/
void GfxDrawScene(void)
{
    /* Hold program execution until VSYNC flag is set
     * and GPU is ready to work. */
    while (!bSyncFlag || GsIsDrawing());

    /* Reset VSYNC event flag. */
    bSyncFlag = false;

    /* Swap drawing and display enviroments Y position. */
    GfxSwapBuffers();

    /* Draw all primitives into screen. */
    GsDrawList();
}

bool GfxIsBusy(void)
{
    return GsIsDrawing();
}

/***************************************************************************//**
*
* \brief    Indicates whether a rectangle defined by x, y, w and h whether
*           inside current drawing area.
*
* \param    x
*               Rectangle initial X offset.
*
* \param    y
*               Rectangle initial X offset.
*
* \param    w
*               Rectangle width.
*
* \param    h
*               Rectangle height.
*
* \return   Returns true if rectangle defined by input parameters
*           is inside screen area, false otherwise.
*
*******************************************************************************/
bool GfxIsInsideScreenArea(const short x, const short y, const short w, const short h)
{
    if (((x + w) >= 0)
            &&
        (x < sDrawEnv.w)
            &&
        ((y + h) >= 0)
            &&
        (y < sDrawEnv.h))
    {
        /* Rectangle is inside drawing environment area. */
        return true;
    }
    else
    {
        /* Rectangle is outside drawing environment area.
         * Fall through. */
    }

    /* Return failure code if reached here. */
    return false;
}

/***************************************************************************//**
*
* \brief    Indicates whether a tSprite instance is inside active
*           drawing environment area.
*
* \param    psSpr
*               Pointer to tSprite structure.
*
*******************************************************************************/
bool GfxIsSpriteInsideScreenArea(const GsSprite *const psSpr)
{
    /* Define X/Y and width/height parameters. */
    const short x = psSpr->x;
    const short y = psSpr->y;
    const short w = psSpr->w;
    const short h = psSpr->h;

    /* Return results. */
    return GfxIsInsideScreenArea(x, y, w, h);
}

/***************************************************************************//**
*
* \brief    Extracting information from tSprite instance, this function adds a
*           low-level GsSprite structure into internal primitive list if inside
*           drawing environment area.
*
* \param    psSpr
*               Index of low-level sprite structure inside the internal array.
*
* \remarks  Sprites bigger than 256x256 px are also supported. Internally,
*           GfxSortBigSprite() draws two primitive, so up to 512x256 px
*           primitives are supported.
*
* \see      GfxSortBigSprite() to see how big sprites are handled.
*
*******************************************************************************/
void GfxSortSprite(GsSprite *const psSpr)
{
    if (GfxIsSpriteInsideScreenArea(psSpr))
    {
        /* Small sprites can be directly drawn using PSXSDK function.
         * On the other hand, big sprites need some more processing. */
        psSpr->w > MAX_SIZE_FOR_GSSPRITE ? GfxSortBigSprite(psSpr) : GsSortSprite(psSpr);
    }
    else
    {
        /* Sprite is outside drawing environment area. Exit. */
    }
}

int GfxToDegrees(const int rotate)
{
    return rotate >> 12;
}

int GfxFromDegrees(const int degrees)
{
    return degrees << 12;
}

void GfxDrawRectangle(GsRectangle* const rect)
{
    GsSortRectangle(rect);
}

/***************************************************************************//**
*
* \brief    Processes big sprites (e.g.: more than 256 px wide) by drawing two
*           separate primitives.
*
* \param    psSpr
*               Pointer to low-level GsSprite structure (given by
*               GfxSortSprite()).
*
*******************************************************************************/
static void GfxSortBigSprite(GsSprite *const psSpr)
{
    /* On the other hand, GsSprite instances bigger than
     * 256x256 px must be split into two primitives, so
     * GsSortSprite shall be called twice. */

    /* Store original TPage, width and X data. */
    const unsigned char aux_tpage = psSpr->tpage;
    const short aux_w = psSpr->w;
    const short aux_x = psSpr->x;

    /* First primitive will be 256x256 px. */
    psSpr->w = MAX_SIZE_FOR_GSSPRITE;

    /* Render first primitive (256x256 px). */
    GsSortSprite(psSpr);

    /* Second primitive will be:
     *  Width = Original Width - 256 px.
     *  Height = Original Height - 256 px. */
    psSpr->x += MAX_SIZE_FOR_GSSPRITE;
    psSpr->w = X_SCREEN_RESOLUTION - MAX_SIZE_FOR_GSSPRITE;

    /* TPage must be increased as we are looking 256 px to
     * the right inside VRAM. Remember that TPages are 64x64 px. */
    psSpr->tpage += MAX_SIZE_FOR_GSSPRITE >> GFX_TPAGE_WIDTH_BITSHIFT;

    /* Render second primitive. */
    GsSortSprite(psSpr);

    /* Restore original TPage, width and X values. */
    psSpr->tpage = aux_tpage;
    psSpr->w = aux_w;
    psSpr->x = aux_x;
}

/*******************************************************************//**
*
* \brief    Initialization of PSX low-level drawing environment.
*
*           The drawing environment is a rectangle where primitives
*           are drawn on.
*
* \remarks  Not to be confused with display environment, which is a
*           rectangle showing VRAM active display area.
*
************************************************************************/
static void GfxInitDrawEnv(void)
{
    /* Initialize drawing environment default values. */
    sDrawEnv.w = X_SCREEN_RESOLUTION;
    sDrawEnv.h = Y_SCREEN_RESOLUTION;

    /* Initialize drawing environment. */
    GsSetDrawEnv(&sDrawEnv);
}

/*******************************************************************//**
*
* \brief    Initialization of PSX low-level display environment.
*
*           The display environment is a rectangle describing VRAM
*           (video RAM) active display area.
*
* \remarks  Not to be confused with drawing environment, which is a
*           rectangle where primitives are drawn on.
*
************************************************************************/
static void GfxInitDispEnv(void)
{
    /* Initialize display environment. */
    GsSetDispEnv(&sDispEnv);
}

/*******************************************************************//**
*
* \brief    This function sets a pointer to a buffer which holds
*           low-level primitive data, and performs double buffering
*           so a secondary buffer can be used to calculate the new scene.
*
************************************************************************/
static void GfxSetPrimList(void)
{
    enum
    {
        /* Maximum amount of each low-level primitive data buffer. */
        PRIMITIVE_LIST_SIZE = 0x400
    };

    /* Buffers that will hold all primitive low-level data. */
    static uint32_t primList[PRIMITIVE_LIST_SIZE];

    /* Set primitive list. */
    GsSetList(primList);
}

/*******************************************************************//**
*
* \brief    Performs double buffering.
*
*           Double buffering consists of swapping drawing and display
*           environments Y position, so that the display environment is
*           showing current frame, whereas the drawing environment
*           is calculating the next frame.
*
************************************************************************/
static void GfxSwapBuffers(void)
{
    enum
    {
        DOUBLE_BUFFERING_SWAP_Y = 256
    };

    if (sDispEnv.y == 0)
    {
        sDispEnv.y = DOUBLE_BUFFERING_SWAP_Y;
        sDrawEnv.y = 0;
    }
    else if (sDispEnv.y == DOUBLE_BUFFERING_SWAP_Y)
    {
        sDispEnv.y = 0;
        sDrawEnv.y = DOUBLE_BUFFERING_SWAP_Y;
    }

    /* Update drawing and display environments
     * with new calculated Y position. */
    GsSetDispEnv(&sDispEnv);
    GsSetDrawEnv(&sDrawEnv);
}

/*******************************************************************//**
*
* \brief    This function is executed on VSYNC event.
*
*           Game runs at a fixed rate of 50 Hz (if PAL) or 60 Hz (NTSC),
*           so if CPU has finished calculating the new scene, it must
*           wait for this interrupt to be triggered so the game runs
*           at desired frame rate.
*
************************************************************************/
static void ISR_VBlank(void)
{
    /* Set VSYNC flag. */
    bSyncFlag = true;
}

/*******************************************************************//**
*
* \brief    Duplicates current displayed screen as into a separate part
*           of VRAM so it can be used as a sprite.
*
************************************************************************/
void GfxSaveDisplayData(GsSprite *const spr)
{
    enum
    {
        VRAM_W = 1024,
        VRAM_H = 512,
        GFX_SECOND_DISPLAY_X = 368,
        GFX_SECOND_DISPLAY_Y = 256,

        GFX_SECOND_DISPLAY_TPAGE = (GFX_SECOND_DISPLAY_X / GFX_TPAGE_WIDTH) + ((GFX_SECOND_DISPLAY_Y / (VRAM_H / 2)) * VRAM_W / GFX_TPAGE_WIDTH),
        GFX_SECOND_DISPLAY_U = GFX_SECOND_DISPLAY_X % GFX_TPAGE_WIDTH
    };

	while (GfxIsBusy());

	MoveImage(	sDispEnv.x,
				sDispEnv.y,
				GFX_SECOND_DISPLAY_X,
				GFX_SECOND_DISPLAY_Y,
				X_SCREEN_RESOLUTION,
				Y_SCREEN_RESOLUTION);

	spr->x = 0;
	spr->y = 0;
	spr->tpage = GFX_SECOND_DISPLAY_TPAGE;
	spr->attribute |= COLORMODE(COLORMODE_16BPP);
	spr->w = X_SCREEN_RESOLUTION;
	spr->h = Y_SCREEN_RESOLUTION;
	spr->u = GFX_SECOND_DISPLAY_U;
	spr->v = 0;
	spr->r = NORMAL_LUMINANCE;
	spr->g = NORMAL_LUMINANCE;
	spr->b = NORMAL_LUMINANCE;

	while (GfxIsBusy());
}
