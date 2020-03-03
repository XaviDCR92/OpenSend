/* *************************************
 *  Includes
 * *************************************/

#include "Serial.h"
#include "Interrupts.h"
#include <psxsio.h>
#include <stdarg.h>
#include <stdio.h>

/* *************************************
 *  Defines
 * *************************************/

#define SERIAL_BAUDRATE 115200
#define SERIAL_TX_RX_TIMEOUT 20000
#define SERIAL_RX_FIFO_EMPTY 0
#define SERIAL_TX_NOT_READY 0
#define SERIAL_PRINTF_INTERNAL_BUFFER_SIZE 256

/* **************************************
 *  Structs and enums                   *
 * *************************************/

/* *************************************
 *  Local Variables
 * *************************************/

/* *************************************
 *  Local Prototypes
 * *************************************/

void SerialInit(void)
{
    SIOStart(115200);
}

void SerialRead(uint8_t *ptrArray, size_t nBytes)
{
    if (nBytes)
    {
        InterruptsDisableInt(INT_SOURCE_VBLANK);

        do
        {
            while ( (SIOCheckInBuffer() == SERIAL_RX_FIFO_EMPTY)); // Wait for RX FIFO not empty

            *(ptrArray++) = SIOReadByte();
        } while (--nBytes);

        InterruptsEnableInt(INT_SOURCE_VBLANK);
    }
}

void SerialWrite(const void* ptrArray, size_t nBytes)
{
    if (nBytes)
    {
        InterruptsDisableInt(INT_SOURCE_VBLANK);
        do
        {
            while ( (SIOCheckOutBuffer() == SERIAL_TX_NOT_READY)); // Wait for TX FIFO empty.

            SIOSendByte(*(uint8_t*)ptrArray++);

        } while (--nBytes);

        InterruptsEnableInt(INT_SOURCE_VBLANK);
    }
    else
    {
    }
}

#ifdef SERIAL_INTERFACE
void Serial_printf(const char* const str, ...)
{
    va_list ap;
    int result;
    static char internal_buffer[SERIAL_PRINTF_INTERNAL_BUFFER_SIZE];

    va_start(ap, str);

    result = vsnprintf( internal_buffer,
                        SERIAL_PRINTF_INTERNAL_BUFFER_SIZE,
                        str,
                        ap  );

    SerialWrite(internal_buffer, result);
}
#endif // SERIAL_INTERFACE
