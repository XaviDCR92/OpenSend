#ifndef FONT_H
#define FONT_H

/* *************************************
 * 	Includes
 * *************************************/

#include <stdarg.h>
#include <stdbool.h>

enum
{
    FONT_DEFAULT_CHAR_SIZE = 16,
    FONT_DEFAULT_INIT_CHAR = '!'
};

enum t_fontflags
{
	FONT_NOFLAGS,
	FONT_CENTERED		= 0x01,
	FONT_WRAP_LINE		= 0x02,
	FONT_BLEND_EFFECT	= 0x04,
	FONT_1HZ_FLASH		= 0x08,
	FONT_2HZ_FLASH		= 0x10,
    FONT_H_CENTERED     = 0x20
};

struct font
{
	GsSprite spr;
	short char_spacing;
	short char_w;
    short char_w_bitshift;
	short char_h;
	char init_ch;
	uint8_t char_per_row;
	uint8_t max_ch_wrap;
	enum font_flags flags;
	short spr_w;
	short spr_h;
	short spr_u;
	short spr_v;
};

bool FontLoadImage(const char *path, struct font *font);
void FontSetSize(struct font *font, short size, short bitshift);
void FontPrintText(struct font *font, short x, short y, char* str, ...);
void FontSetInitChar(struct font *font, char c);
void FontSetFlags(struct font *font, enum font_flags flags);
void FontCyclic(void);
void FontSetSpacing(struct font *font, short spacing);

/* *************************************
 * 	Global variables
 * *************************************/

extern struct font RadioFont;
extern struct font SmallFont;

#endif /* FONT_H */
