#include "reception.h"
#include "System.h"
#include "Serial.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static volatile bool input_available;

#define STATES \
    X(magic_byte) \
    X(psx_exe_header) \
    X(dump) \
    X(done)

static enum
{
#define X(s) state_##s,
    STATES
#undef X
} state;

int (*init_pc)(void);
unsigned char *init_addr;
size_t sz;

static void ack(void)
{
    enum
    {
        ACK = 'b'
    };

    SerialWrite(ACK);
}

static size_t done(void)
{
    return 0;
}

static size_t dump(void)
{
    size_t aux = rx.processed;
    bool done = false;

    while (aux != rx.pending)
    {
        enum
        {
            PACKET_SZ = 128
        };

        static size_t total, packet;

        if (++aux >= sizeof rx.buf / sizeof *rx.buf)
            aux = 0;

        init_addr[total] = rx.buf[aux];

        if (++packet >= PACKET_SZ)
        {
            ack();
            packet = 0;
        }

        if (++total >= sz)
        {
            done = true;
            break;
        }
    }

    if (done)
    {
        state = state_done;
    }

    return aux;
}

static size_t psx_exe_header(void)
{
    typedef const struct
    {
        unsigned char id[8];
        unsigned char unused[8];
        void *init_addr;
        void *init_gp;
        void *init_pc;
        size_t sz;
    } __attribute__((packed)) decoded_header;

    size_t aux = rx.processed;
    unsigned char header[sizeof (decoded_header)];
    bool full = false;

    while (aux != rx.pending)
    {
        static size_t i;

        if (++aux >= sizeof rx.buf / sizeof *rx.buf)
            aux = 0;

        header[i] = rx.buf[aux];

        _Static_assert (sizeof (decoded_header) == 32, "unexpected struct size");

        if (++i >= sizeof (decoded_header))
        {
            full = true;
            break;
        }
    }

    if (full)
    {
        decoded_header *const decoded = (const void *)header;

        if (!strcmp((const char *)decoded->id, "PS-X EXE"))
        {
            if (decoded->init_pc == (const void *)0x80010000)
            {
                init_pc = decoded->init_pc + 0x14800;
                init_addr = decoded->init_addr + 0x14800;
            }
            else
            {
                init_pc = decoded->init_pc;
                init_addr = decoded->init_addr;
            }

            printf("init_pc=%p,init_addr=%p,sz=%d\n", init_pc, init_addr, sz);

            sz = decoded->sz;

            ack();
            state = state_dump;
        }
    }

    return aux;
}

static size_t magic_byte(void)
{
    size_t aux = rx.processed;

    if (++aux >= sizeof rx.buf / sizeof *rx.buf)
        aux = 0;

    {
        enum
        {
            MAGIC_BYTE = 99
        };

        const unsigned char in = rx.buf[aux];

        if (in == MAGIC_BYTE)
        {
            state = state_psx_exe_header;
            ack();
        }
    }

    return aux;
}

void reception_loop(void)
{
    while (state != state_done)
    {
        if (input_available && rx.pending != rx.processed)
        {
            static size_t (*const h[])(void) =
            {
#define X(s) [state_##s] = s,
                STATES
#undef X
            };

            rx.processed = h[state]();

            input_available = false;
        }
    }

    if (init_pc)
    {
        SystemDeinit();
        init_pc();
    }
}

void reception_ev(void)
{
    input_available = true;
}
