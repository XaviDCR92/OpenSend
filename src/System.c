/* *************************************
 * 	Includes
 * *************************************/

#include "System.h"

/* *************************************
 * 	Defines
 * *************************************/
 
#define SYSTEM_MAX_TIMERS 16
#define FILE_BUFFER_SIZE 0xC40
#define END_STACK_PATTERN (uint32_t) 0x18022015
#define BEGIN_STACK_ADDRESS (uint32_t*) 0x801FFF00
#define STACK_SIZE 0x1000
#define I_MASK (*(volatile unsigned int*)0x1F801074)

/* *************************************
 * 	Local Prototypes
 * *************************************/

static void SystemSetStackPattern(void);

/* *************************************
 * 	Local Variables
 * *************************************/

//Buffer to store any kind of files. It supports files up to 128 kB
static uint8_t file_buffer[FILE_BUFFER_SIZE];
//Global timer (called by interrupt)
static volatile uint64_t global_timer;
//Tells whether rand seed has been set
static bool rand_seed;
//Screen refresh flag (called by interrupt)
static volatile bool refresh_needed;
//Timers
static bool one_second_timer;
//Critical section is entered (i.e.: when accessing fopen() or other BIOS functions
static volatile bool system_busy;

/* *******************************************************************
 * 
 * @name: void SystemInit(void)
 * 
 * @author: Xavier Del Campo
 * 
 * @brief: Calls main intialization routines.
 * 
 * @remarks: To be called before main loop.
 * 
 * *******************************************************************/

void SystemInit(void)
{
	//Reset global timer
	global_timer = 0;
	//Reset 1 second timer
	one_second_timer = 0;
	//PSXSDK init
	PSX_InitEx(PSX_INIT_SAVESTATE | PSX_INIT_CD);
	//Graphics init
	GsInit();
	//Clear VRAM
	GsClearMem();
	//Set Video Resolution
#ifdef _PAL_MODE_
	GsSetVideoMode(X_SCREEN_RESOLUTION, Y_SCREEN_RESOLUTION, VMODE_PAL);
#else
	GsSetVideoMode(X_SCREEN_RESOLUTION, Y_SCREEN_RESOLUTION, VMODE_NTSC);
#endif //_PAL_MODE_
	//SPU init
	SsInit();
	//Set Drawing Environment
	GfxInitDrawEnv();
	//Set Display Environment
	GfxInitDispEnv();
	//Set Primitive List
	GfxSetPrimitiveList();
	//Initial value for system_busy
	system_busy = false;
	
	GfxSetGlobalLuminance(NORMAL_LUMINANCE);

	SystemSetStackPattern();
}

size_t SystemGetBufferSize(void)
{
    return sizeof(file_buffer);
}

/* *******************************************************************
 * 
 * @name: void SystemInit(void)
 * 
 * @author: Xavier Del Campo
 * 
 * @brief:
 * 	Calls srand() while avoiding multiple calls by setting internal
 *	variable rand_seed to true. Internal variable "global_timer" is
 *	used to generate the new seed.
 * 
 * @remarks:
 * 	It is recommended to call it once user has pressed any key.
 * 
 * *******************************************************************/

void SystemSetRandSeed(void)
{
	if(rand_seed == false)
	{
		rand_seed = true;
		//Set random seed using global timer as reference
		srand((unsigned int)global_timer);
		
		dprintf("Seed used: %d\n",(unsigned int)global_timer);
	}
}

/* *******************************************************************
 * 
 * @name: bool SystemIsRandSeedSet(void)
 * 
 * @author: Xavier Del Campo
 * 
 * @brief:
 * 	Reportedly, returns whether rand seed has already been set.
 * 
 * @remarks:
 *
 * @return:
 *	 Reportedly, returns whether rand seed has already been set.
 * 
 * *******************************************************************/

bool SystemIsRandSeedSet(void)
{
	return rand_seed;
}

/* *******************************************************************
 * 
 * @name: bool SystemRefreshNeeded(void)
 * 
 * @author: Xavier Del Campo
 * 
 * @brief:
 * 
 * @remarks:
 *
 * @return:
 *	 Returns whether VSync flag has been enabled.
 * 
 * *******************************************************************/

bool SystemRefreshNeeded(void)
{
	return refresh_needed;
}

/* *******************************************************************
 * 
 * @name: void ISR_SystemDefaultVBlank(void)
 * 
 * @author: Xavier Del Campo
 * 
 * @brief:
 * 
 * @remarks:
 * 	Called from VSync interrupt. Called 50 times a second in PAL mode,
 * 	60 times a second in NTSC mode.
 * 
 * *******************************************************************/

void ISR_SystemDefaultVBlank(void)
{
	refresh_needed = true;
	SystemIncreaseGlobalTimer();
}

/* *******************************************************************
 * 
 * @name: void SystemIncreaseGlobalTimer(void)
 * 
 * @author: Xavier Del Campo
 * 
 * @brief:
 * 	Increases internal variable responsible for time handling.
 * 
 * @remarks:
 * 	Usually called from ISR_SystemDefaultVBlank().
 * 
 * *******************************************************************/

void SystemIncreaseGlobalTimer(void)
{
	global_timer++;
}

/* *******************************************************************
 * 
 * @name: uint64_t SystemGetGlobalTimer(void)
 * 
 * @author: Xavier Del Campo
 * 
 * @brief: Returns internal global timer value.
 * 
 * *******************************************************************/

uint64_t SystemGetGlobalTimer(void)
{
	return global_timer;
}

/* *******************************************************************
 * 
 * @name: void SystemDisableScreenRefresh(void)
 * 
 * @author: Xavier Del Campo
 * 
 * @brief: Resets VBlank IRQ flag.
 * 
 * *******************************************************************/

void SystemDisableScreenRefresh(void)
{
	refresh_needed = false;
}

/* *******************************************************************
 * 
 * @name: bool System1SecondTick(void)
 * 
 * @author: Xavier Del Campo
 * 
 * @return: bool variable with a 1-cycle-length pulse that gets
 * 			set each second.
 * 
 * *******************************************************************/

bool System1SecondTick(void)
{
	return !(global_timer % REFRESH_FREQUENCY);
}

/* ****************************************************************************************
 * 
 * @name	bool SystemLoadFileToBuffer(char* fname, uint8_t* buffer, uint32_t szBuffer)
 * 
 * @author: Xavier Del Campo
 * 
 * @brief:	Given an input path, it fills a buffer pointed to by "buffer" with
 * 			maximum size "szBuffer" with data from CD-ROM.
 *
 * @return:	true if file has been loaded successfully, false otherwise.
 * 
 * ****************************************************************************************/

bool SystemLoadFileToBuffer(char* fname, uint32_t init_pos, uint8_t* buffer, uint32_t szBuffer)
{
	FILE *f;
	int32_t size;
	
	// Wait for possible previous operation from the GPU before entering this section.
	while( (SystemIsBusy() == true) || (GfxIsGPUBusy() == true) );
	
	if(fname == NULL)
	{
		dprintf("SystemLoadFile: NULL fname!\n");
		return false;
	}
	
	memset(buffer,0,szBuffer);
	
	system_busy = true;
	
	SystemDisableVBlankInterrupt();
	
	f = fopen(fname, "r");
	
	if(f == NULL)
	{
		dprintf("SystemLoadFile: file could not be found!\n");
		//File couldn't be found
		return false;
	}

	fseek(f, init_pos, SEEK_END);

	size = ftell(f);
	
	if(size > szBuffer)
	{
		dprintf("SystemLoadFile: Exceeds file buffer size (%d bytes)\n",size);
		//Bigger than 128 kB (buffer's max size)
		return false;
	}
	
	fseek(f, init_pos, SEEK_SET); //f->pos = 0;
	
	fread(buffer, sizeof(char), size, f);
	
	fclose(f);
	
	SystemEnableVBlankInterrupt();
	
	system_busy = false;
	
	dprintf("File \"%s\" loaded successfully!\n",fname);
	
	return true;
}

void SystemSetBusyFlag(bool value)
{
    system_busy = value;
}

/* ****************************************************************************************
 * 
 * @name	bool SystemLoadFile(char*fname)
 * 
 * @author: Xavier Del Campo
 * 
 * @brief:	Given an input file name, it loads its conents into internal buffer.
 *
 * @return:	true if file has been loaded successfully, false otherwise.
 * 
 * ****************************************************************************************/

bool SystemLoadFile(char*fname)
{
	return SystemLoadFileToBuffer(fname,0,file_buffer,sizeof(file_buffer));
}

/* ******************************************************************
 * 
 * @name	uint8_t* SystemGetBufferAddress(void)
 * 
 * @author: Xavier Del Campo
 *
 * @return:	Reportedly, returns internal buffer initial address.
 * 
 * *****************************************************************/

uint8_t* SystemGetBufferAddress(void)
{
	return file_buffer;
}

/* ******************************************************************
 * 
 * @name	void SystemClearBuffer(void)
 * 
 * @author: Xavier Del Campo
 *
 * @return:	Fills internal buffer with zeros
 * 
 * *****************************************************************/

void SystemClearBuffer(void)
{
	memset(file_buffer, 0, sizeof(file_buffer));
}

/* ******************************************************************
 * 
 * @name	uint32_t SystemRand(uint32_t min, uint32_t max)
 * 
 * @author: Xavier Del Campo
 *
 * @return:	random number between "min" and "max".
 *
 * @remarks: rand seed must be set before using this function, or
 * 			 you will predictable values otherwise!
 * 
 * *****************************************************************/

uint32_t SystemRand(uint32_t min, uint32_t max)
{
	return rand() % (max - min + 1) + min;
}

/* ***********************************************************************
 * 
 * @name	volatile bool SystemIsBusy(void)
 * 
 * @author: Xavier Del Campo
 *
 * @return:	returns system busy flag.
 * 
 * ***********************************************************************/

volatile bool SystemIsBusy(void)
{
	return system_busy;
}

bool SystemArrayCompare(unsigned short* arr1, unsigned short* arr2, size_t sz)
{
	size_t i;
	
	for(i = 0; i < sz; i++)
	{
		if(arr1[i] != arr2[i])
		{
			return false;
		}
	}
	
	return true;
}

void SystemPrintStackPointerAddress(void)
{
#ifdef PSXSDK_DEBUG // Used to avoid unused variable warning
	void * ptr = NULL;
	fix16_t used_bytes = fix16_from_int((int)((void*)BEGIN_STACK_ADDRESS - (void*)&ptr));
	fix16_t stackPercent = fix16_sdiv(used_bytes,fix16_from_int((int)STACK_SIZE));
	
	stackPercent = fix16_smul(stackPercent, fix16_from_int((int)100));
	
	dprintf("stackPercent: %d\n", stackPercent);
	
	dprintf("Stack begin pointer: 0x%08X\n"
			"Stack pointer address: 0x%08X\n"
			"Used %d%% of stack size.\n"
			"\tUsed bytes: %d\n",
			(void*)BEGIN_STACK_ADDRESS,
			(void*)&ptr,
			fix16_to_int(stackPercent),
			fix16_to_int(used_bytes)	);
#endif // PSXSDK_DEBUG

}

void SystemCheckStack(void)
{
	uint32_t * ptrStack = BEGIN_STACK_ADDRESS;
	uint32_t data;
	
	ptrStack -= STACK_SIZE;
	data = (*ptrStack);
	
	if(data != END_STACK_PATTERN)
	{
		dprintf("Stack overflow?\n");
		
		while(1);
	}
}

void SystemSetStackPattern(void)
{
	uint32_t * ptrStack = BEGIN_STACK_ADDRESS;
	
	ptrStack -= STACK_SIZE;
	
	*ptrStack = END_STACK_PATTERN;
}

int32_t SystemIndexOfStringArray(char* str, char** array)
{
	int32_t i;
	
	for(i = 0; array[i] != NULL; i++)
	{
		dprintf("String to find: %s\nEntry: %s\n", str, array[i]);
		
		if(strcmp(str, array[i]) == 0)
		{
			dprintf("Match! Returning index %d...\n", i);
			return i;
		}
	}
	
	return -1;
}
void SystemCyclicHandler(void)
{	
	SystemDisableScreenRefresh();
	SystemCheckStack();
}

void SystemDisableVBlankInterrupt(void)
{
	I_MASK &= ~(0x00000001);
}

void SystemEnableVBlankInterrupt(void)
{
	I_MASK |= (0x00000001);
}
