#ifndef LOAD_MENU_H
#define LOAD_MENU_H

/* *************************************
 * 	Includes
 * *************************************/

#include <stdint.h>

/* *************************************
 * 	Defines
 * *************************************/

/* *************************************
 * 	Global prototypes
 * *************************************/

void LoadMenuInit(void);

void LoadMenu(	const char *const *fileList,
				void *const *dest,
				uint8_t szFileList, uint8_t szDestList);

void LoadMenuEnd(void);

#endif /* LOAD_MENU_H */
