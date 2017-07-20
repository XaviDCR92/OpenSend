#ifndef __SERIAL_HEADER__
#define __SERIAL_HEADER__

/* *************************************
 * 	Includes
 * *************************************/

#include "Global_Inc.h"
#include "System.h"
#include "Gfx.h"
#include "Font.h"

/* *************************************
 * 	Defines
 * *************************************/

#define ACK_BYTE_STRING "b"

/* **************************************
 * 	Structs and enums					*
 * *************************************/

typedef enum
{
    SERIAL_STATE_INIT = 0,
    SERIAL_STATE_STANDBY,
    SERIAL_STATE_WRITING_ACK,
    SERIAL_STATE_READING_HEADER,
    SERIAL_STATE_READING_EXE_SIZE,
    SERIAL_STATE_READING_EXE_DATA,
    SERIAL_STATE_WAITING_USER_INPUT,
    SERIAL_STATE_CLEANING_MEMORY,
}SERIAL_STATE;

/* *************************************
 * 	Global prototypes
 * *************************************/

void SerialInit(void);
bool SerialRead(uint8_t* ptrArray, size_t nBytes);
bool SerialWrite(void* ptrArray, size_t nBytes);
void ISR_Serial(void);
void SerialSetState(SERIAL_STATE state);
void SerialSetPCAddress(uint32_t addr);
void SerialSetRAMDestAddress(uint32_t addr);
void SerialSetExeSize(size_t size);
void SerialSetExeBytesReceived(uint32_t bytes_read);

#endif // __SERIAL_HEADER__
