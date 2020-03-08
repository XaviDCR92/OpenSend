#ifndef SERIAL_H
#define SERIAL_H

/* *************************************
 *  Includes
 * *************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define SERIAL_DATA_PACKET_SIZE 8
#define ACK_BYTE_STRING "b"

enum
{
    FIFO_SZ = 132
};

typedef volatile struct
{
    unsigned char buf[FIFO_SZ];
    size_t pending, processed;
    bool full;
} fifo;

extern fifo rx;

void SerialInit(void);
void SerialWrite(unsigned char byte);

#ifdef __cplusplus
}
#endif

#endif /* SERIAL_H */
