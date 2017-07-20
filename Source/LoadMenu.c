/* **************************************
 * 	Includes							*	
 * *************************************/

#include "LoadMenu.h"

/* **************************************
 * 	Defines								*	
 * *************************************/

/* **************************************
 * 	Structs and enums					*
 * *************************************/

enum
{
	SMALL_FONT_SIZE = 8,
	SMALL_FONT_SIZE_BITSHIFT = 3,
    SMALL_FONT_SPACING = 6
};

enum
{
	BG_BLUE_TARGET_VALUE = 0xC0,
	BG_WHITE_TARGET_VALUE = /*0x40*/ 0,
	BG_INCREASE_STEP = 0x10
};

enum
{
	LOADING_BAR_X = 64,
	LOADING_BAR_Y = 200,
	LOADING_BAR_N_LINES = 4,
	
	LOADING_BAR_WIDTH = 256,
	LOADING_BAR_HEIGHT = 16,
	
	LOADING_BAR_LUMINANCE_TARGET = NORMAL_LUMINANCE,
	LOADING_BAR_LUMINANCE_STEP = 10
};

enum
{
	LOADING_TITLE_CLUT_X = 384,
	LOADING_TITLE_CLUT_Y = 496,
	LOADING_TITLE_X = 128,
	LOADING_TITLE_Y = 32,
	
	LOADING_TITLE_U = 0,
	LOADING_TITLE_V = 0,
	
	LOADING_TITLE_LUMINANCE_STEP = 10,
	LOADING_TITLE_LUMINANCE_TARGET = NORMAL_LUMINANCE
};

enum
{	
	PLANE_START_X = 56,
	PLANE_START_Y = 200,
	
	PLANE_U = 0,
	PLANE_V = 32,
	PLANE_SIZE = 16,
	
	PLANE_LUMINANCE_STEP = 0x10,
	PLANE_LUMINANCE_TARGET_VALUE = NORMAL_LUMINANCE
};

/* *************************************
 * 	Local Prototypes
 * *************************************/
 
static void LoadMenuLoadFileList(	char* fileList[], 	void * dest[], 
									uint8_t szFileList, uint8_t szDestList);

/* *************************************
 * 	Local Variables
 * *************************************/

static char* LoadMenuFiles[] = { "cdrom:\\DATA\\FONTS\\FONT_2.FNT;1"	};

static void * LoadMenuDest[] = { (TYPE_FONT*)&SmallFont		};

static char* strCurrentFile;

// Flags to communicate with ISR state
// 	*	startup_flag: background fades in from black to blue.
// 	*	end_flag: tells the background to fade out to black.
//	*	isr_ended: background has totally faded out to black.
//	*	isr_started: tells the ISR has finished starting up.
static volatile bool startup_flag;
static volatile bool isr_started;
static volatile bool end_flag;
static volatile bool isr_ended;
// Set to true when LoadMenuInit() has been called, and set to false
// once LoadMenuEnd() is called.
// It's used when multiple modules call LoadMenu() at the same time,
// so load menu does not have to be initialised each time;
static bool load_menu_running;

void LoadMenuInit(void)
{
	static bool first_load = false;
	
	if(first_load == false)
	{
		first_load = true;
		LoadMenuLoadFileList(	LoadMenuFiles,
								LoadMenuDest,
								sizeof(LoadMenuFiles) / sizeof(char*),
								sizeof(LoadMenuDest)	/ sizeof(void*));
	}
	
	FontSetSize(&SmallFont, SMALL_FONT_SIZE, SMALL_FONT_SIZE_BITSHIFT);
    FontSetSpacing(&SmallFont, SMALL_FONT_SPACING);

	SmallFont.spr.r = 0;
	SmallFont.spr.g = 0;
	SmallFont.spr.b = 0;
	
	GfxSetGlobalLuminance(NORMAL_LUMINANCE);
}

void LoadMenu(	char*	fileList[], 
				void * dest[],
				uint8_t szFileList	, uint8_t szDestList)
{
	
	if(load_menu_running == false)
	{
		LoadMenuInit();
	}
	
	LoadMenuLoadFileList(fileList,dest,szFileList,szDestList);
}

void LoadMenuLoadFileList(	char* fileList[], 	void * dest[], 
							uint8_t szFileList, uint8_t szDestList)
{
	char aux_file_name[100];
	char* extension;
	uint8_t fileLoadedCount;
	
	if(szFileList != szDestList)
	{
		dprintf("File list size different from dest list size! %d vs %d\n",
				szFileList, szDestList);
		return;
	}
	
	for(fileLoadedCount = 0; fileLoadedCount < szFileList ; fileLoadedCount++)
	{
		if(fileList[fileLoadedCount] == NULL)
		{
			continue;
		}
		
		strCurrentFile = fileList[fileLoadedCount];
						
		//dprintf("Files %d / %d loaded. New plane X = %d.\n",fileLoadedCount,szFileList,LoadMenuPlaneSpr.x);
		
		// Backup original file path
		strncpy(aux_file_name,fileList[fileLoadedCount],100);
		
		//We want to get file extension, so split into tokens
		strtok(fileList[fileLoadedCount],".;");
		extension = strtok(NULL,".;");
		
		dprintf("File extension: .%s\n",extension);
		//Restore original file path in order to load file
		strncpy(fileList[fileLoadedCount],aux_file_name,100);
		
		if(strncmp(extension,"TIM",3) == 0)
		{
			if(GfxSpriteFromFile(fileList[fileLoadedCount], dest[fileLoadedCount]) == false)
			{
				dprintf("Could not load image file \"%s\"!\n",fileList[fileLoadedCount]);
			}
		}
		else if(strncmp(extension,"CLT",3) == 0)
		{
			if(dest[fileLoadedCount] != NULL)
			{
				dprintf("WARNING: File %s linked to non-NULL destination pointer!\n", dest[fileLoadedCount]);
			}
			
			if(GfxCLUTFromFile(fileList[fileLoadedCount]) == false)
			{
				dprintf("Could not load CLUT file \"%s\"!\n",fileList[fileLoadedCount]);
			}
		}
		else if(strncmp(extension,"FNT",3) == 0)
		{
			if(FontLoadImage(fileList[fileLoadedCount], dest[fileLoadedCount]) == false)
			{
				dprintf("Could not load font file \"%s\"!\n",fileList[fileLoadedCount]);
			}
		}
		else
		{
			dprintf("LoadMenu does not recognize following extension: %s\n",extension);
		}
	}
}
