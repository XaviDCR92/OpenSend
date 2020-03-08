#include "Serial.h"
#include "Interrupts.h"
#include <psxsio.h>
#include <stdarg.h>
#include <stdio.h>
#include <psxbios.h>
#include <psx.h>
#include <stddef.h>

enum
{
    FIFO_SZ = 128
};

typedef volatile struct
{
    unsigned char buf[FIFO_SZ];
    size_t pending, processed;
    bool full;
} fifo;

static fifo rx;

void sio_handler_callback(void)
{
    while (SIOCheckInBuffer())
    {
        const unsigned char in = SIOReadByte();
        size_t aux = rx.pending;

        if (++aux >= (sizeof rx.buf / sizeof *rx.buf))
            aux = 0;

        if (aux != rx.processed)
        {
            rx.buf[aux] = in;
            rx.pending = rx.processed;
        }
        else
        {
            rx.full = true;
        }
    }

    SIOAcknowledgeInterrupt();
}

void SerialInit(void)
{
    enum
    {
        SPEC_COUNTER_BECOMES_ZERO = 0x0001,
        SPEC_INTERRUPTED = 0x0002,
        SPEC_EOF = 0x0004,
        SPEC_FILE_WAS_CLOSED = 0x0008,
        SPEC_CMD_ACK = 0x0010,
        SPEC_CMD_COMPLETED = 0x0020,
        SPEC_DATA_READY = 0x0040,
        SPEC_DATA_END = 0x0080,
        SPEC_TIMEOUT = 0x0100,
        SPEC_UNKNOWN_CMD = 0x0200,
        SPEC_END_READ_BUFFER = 0x0400,
        SPEC_END_WRITE_BUFFER = 0x0800,
        SPEC_GENERAL_INTERRUPT = 0x1000,
        SPEC_NEW_DEVICE = 0x2000,
        SPEC_SYS_CALL_INSTR = 0x4000,
        SPEC_ERROR = 0x8000,
        SPEC_PREV_WRITE_ERROR = 0x8001,
        SPEC_DOMAIN_ERROR_LIBMATH = 0x0301,
        SPEC_RANGE_ERROR_LIBMATH = 0x0302
    };

    enum
    {
        SIO_CLASS = 0xF000000B,
        TRIGGER_CALLBACK = 0x1000,
        BAUD_RATE = 115200
    };

    const int not_crit = EnterCriticalSection();
    void sio_handler(void);

    SIOReset();
    SIOStart(BAUD_RATE);

    const int sio_handle = OpenEvent(SIO_CLASS, SPEC_GENERAL_INTERRUPT, TRIGGER_CALLBACK, sio_handler);

    if (sio_handle != 0xFFFFFFFF)
        EnableEvent(sio_handle);

    IMASK |= 1 << INT_SIO;

    SIOEnableRXInterrupt();

    if (not_crit)
    {
        ExitCriticalSection();
    }
}
