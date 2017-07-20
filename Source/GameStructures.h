#ifndef __GAME_STRUCTURES__HEADER__
#define __GAME_STRUCTURES__HEADER__

/* *************************************
 * 	Defines
 * *************************************/

#define CHEAT_ARRAY_SIZE 16

/* *************************************
 * 	Structs and enums
 * *************************************/

typedef enum t_fontflags
{
	FONT_NOFLAGS		= 0,
	FONT_CENTERED		= 0x01,
	FONT_WRAP_LINE		= 0x02,
	FONT_BLEND_EFFECT	= 0x04,
	FONT_1HZ_FLASH		= 0x08,
	FONT_2HZ_FLASH		= 0x10,
    FONT_H_CENTERED     = 0x20
}FONT_FLAGS;

typedef struct t_Font
{
	GsSprite spr;
	short char_spacing;
	short char_w;
    short char_w_bitshift;
	short char_h;
	char init_ch;
	uint8_t char_per_row;
	uint8_t max_ch_wrap;
	FONT_FLAGS flags;
	short spr_w;
	short spr_h;
	short spr_u;
	short spr_v;
}TYPE_FONT;

typedef struct t_Timer
{
	uint32_t time;
	uint32_t orig_time;
	bool repeat_flag;
	bool busy;
	void (*Timeout_Callback)(void);
}TYPE_TIMER;

typedef struct t_Cheat
{
	unsigned short Combination[CHEAT_ARRAY_SIZE];
	void (*Callback)(void);
}TYPE_CHEAT;

#endif // __GAME_STRUCTURES__HEADER__
