#include "Serial.h"
#include "Interrupts.h"
#include <psxsio.h>
#include <stdarg.h>
#include <stdio.h>
#include <psxbios.h>
#include <psx.h>
#include <stddef.h>

static volatile size_t isr_calls;

void sio_handler_callback(void)
{
    printf("%s\n", __func__);
    isr_calls++;
}

void SerialInit(void)
{
    int *sio_handler(void);

    enum
    {
        SIO_CLASS = 0xF000000B
    };

    EnterCriticalSection();

    IMASK |= 1 << 8;

    const int sio_handle = OpenEvent(SIO_CLASS, 0x1000, 0x1000, sio_handler);

    if (sio_handle != 0xFFFFFFFF)
    {
        enum
        {
            BAUD_RATE = 115200
        };

        EnableEvent(sio_handle);
        SIOStart(BAUD_RATE);
        redirect_stdio_to_sio();
        printf("sio_handle = 0x%08X", sio_handle);
    }

    ExitCriticalSection();

    SIOSendByte('y');

    printf("%u\n", isr_calls);

    while (!(IPENDING & (1 << 8)));

    printf("SIO ISR triggered\n");
}

void SerialRead(uint8_t *ptrArray, size_t nBytes)
{
    if (nBytes)
    {
        InterruptsDisableInt(INT_SOURCE_VBLANK);

        do
        {
            /* Wait for RX FIFO not empty. */
            while (!SIOCheckInBuffer());

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
            /* Wait for TX FIFO empty. */
            while (!SIOCheckOutBuffer());

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
