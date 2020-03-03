/* *************************************
 * 	Includes
 * *************************************/

#include "Font.h"

/* *************************************
 * 	Defines
 * *************************************/

#define FONT_INTERNAL_TEXT_BUFFER_MAX_SIZE 200

/* *************************************
 * 	Local Prototypes
 * *************************************/

/* *************************************
 * 	Local Variables
 * *************************************/

static char _internal_text[FONT_INTERNAL_TEXT_BUFFER_MAX_SIZE];
static unsigned char _blend_effect_lum;

bool FontLoadImage(char *path, struct font *font)
{
	if (GfxSpriteFromFile(path, &font->spr) == false)
	{
		return false;
	}

	font->spr_w = font->spr.w;
	font->spr_h = font->spr.h;
	font->spr_u = font->spr.u;
	font->spr_v = font->spr.v;

	/* Now set default values to font */

	font->char_w = FONT_DEFAULT_CHAR_SIZE;
	font->char_h = FONT_DEFAULT_CHAR_SIZE;

	font->spr.attribute |= COLORMODE(COLORMODE_4BPP);
	font->spr.attribute &= COLORMODE(~(COLORMODE_8BPP | COLORMODE_16BPP | COLORMODE_24BPP));
	font->spr.r = NORMAL_LUMINANCE;
	font->spr.g = NORMAL_LUMINANCE;
	font->spr.b = NORMAL_LUMINANCE;

	/* At this point, spr.w and spr.h = real w/h */
	font->char_per_row = (uint8_t)(font->spr_w / font->char_w);
	font->max_ch_wrap = 0;

	font->spr.w = font->char_w;
	font->spr.h = font->char_h;

	font->flags = FONT_NOFLAGS;

	font->init_ch = FONT_DEFAULT_INIT_CHAR;

	dprintf("Sprite CX = %d, sprite CY = %d\n",font->spr.cx, font->spr.cy);

	return true;
}

void FontSetInitChar(struct font *font, char c)
{
	font->init_ch = c;
}

void FontSetFlags(struct font *font, enum font_flags flags)
{
	font->flags = flags;
}

void FontSetSize(struct font *font, short size, short bitshift)
{
	font->char_w = size;
	font->char_h = size;

    font->char_w_bitshift = bitshift;

	/* At this point, spr.w and spr.h = real w/h */
	font->char_per_row = (uint8_t)(font->spr_w / font->char_w);
	font->max_ch_wrap = 0;

	font->spr.w = font->char_w;
	font->spr.h = font->char_h;
}

void FontSetSpacing(struct font *font, short spacing)
{
    font->char_spacing = spacing;
}

void FontCyclic(void)
{
	_blend_effect_lum -= 8;
}

void FontPrintText(struct font *font, short x, short y, char* str, ...)
{
	uint16_t i;
	uint16_t line_count = 0;
	int result;
	short orig_x = x;

	va_list ap;

	va_start(ap, str);

	result = vsnprintf(	_internal_text,
						FONT_INTERNAL_TEXT_BUFFER_MAX_SIZE,
						str,
						ap	);

    if (font->flags & FONT_H_CENTERED)
    {
        x = (X_SCREEN_RESOLUTION >> 1) - ((result >> 1) << font->char_w_bitshift);
        orig_x = x;
    }

	for (i = 0; i < result ; i++)
	{
		char _ch = _internal_text[i];

		if (_ch == '\0')
		{
			/* End of string */
			break;
		}

		switch(_ch)
		{
			case ' ':
				x += font->char_w;
				continue;
			case '\n':
				x = orig_x;
				y += font->char_h;
			break;
			default:
				if (	(font->flags & FONT_WRAP_LINE) && (font->max_ch_wrap != 0) )
				{
					if (++line_count >= font->max_ch_wrap)
					{
						line_count = 0;
						x = orig_x;
						y += font->char_h;
					}
				}

				font->spr.x = x;
				font->spr.y = y;
				font->spr.w = font->char_w;
				font->spr.h = font->char_h;
				font->spr.u = (short)( (_ch - font->init_ch) % font->char_per_row) *font->char_w;
				font->spr.u += font->spr_u; /* Add original offset for image */
				font->spr.v = (short)( (_ch - font->init_ch) / font->char_per_row) *font->char_h;
				font->spr.v += font->spr_v; /* Add original offset for image */

				if (font->flags & FONT_BLEND_EFFECT)
				{
					font->spr.r += 8;
					font->spr.g += 8;
					font->spr.b += 8;
				}
				else
				{
					font->spr.r = NORMAL_LUMINANCE;
					font->spr.g = NORMAL_LUMINANCE;
					font->spr.b = NORMAL_LUMINANCE;
				}
				/*dprintf("char_w = %d, char_h = %d, char_per_row = %d, init_ch: %c\n",
						font->char_w,
						font->char_h,
						font->char_per_row,
						font->init_ch);
				dprintf("Char: %c, spr.u = %d, spr.v = %d\n",str[i],font->spr.u, font->spr.v);
				dprintf("Sprite CX = %d, sprite CY = %d\n",font->spr.cx, font->spr.cy);*/
				/* dprintf("Sprite rgb={%d,%d,%d}\n",font->spr.r, font->spr.g, font->spr.b); */

				GfxSortSprite(&font->spr);
				x += font->char_spacing;
			break;
		}
	}

	if (font->flags & FONT_BLEND_EFFECT)
	{
		font->spr.r = _blend_effect_lum;
		font->spr.g = _blend_effect_lum;
		font->spr.b = _blend_effect_lum;
	}

	va_end(ap);
}
