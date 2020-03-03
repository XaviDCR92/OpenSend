/*******************************************************************//**
*
* \file     IO.c
*
* \author   Xavier Del Campo
*
* \brief    Implementation of IO module.
*
************************************************************************/

/* *************************************
 * Includes
 * *************************************/

#include "IO.h"
#include "Interrupts.h"
#include "Gfx.h"
#include "Serial.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

/* *************************************
 * Defines
 * *************************************/

enum
{
    FILE_BUFFER_SIZE = 182 << 10
};

/* *************************************
 * Types definition
 * *************************************/

/* *************************************
 * Global variables definition
 * *************************************/

/* *************************************
 * Local variables definition
 * *************************************/

/* *************************************
 *  Local prototypes declaration
 * *************************************/

/* *************************************
 * Functions definition
 * *************************************/

#ifndef SERIAL_INTERFACE

static const uint8_t *IOLoadFileFromCd(char* const buffer, size_t* const fileSize, uint8_t *const fileBuffer)
{
    InterruptsDisableInt(INT_SOURCE_VBLANK);

    /* Get file data from input file path. */
    FILE* pFile = fopen((char*)buffer, "rb");

    InterruptsEnableInt(INT_SOURCE_VBLANK);

    if (pFile != NULL)
    {
        /* Move file pointer to end of file. */
        if (fseek(pFile, 0, SEEK_END) == 0 /* Success code. */)
        {
            if (fileSize != NULL)
            {
                /* File pointer could be successfully
                 * moved to the new position. */

                /* Return file size in bytes to upper layers. */
                *fileSize = ftell(pFile);

                if (*fileSize <= FILE_BUFFER_SIZE)
                {
                    /* Buffer was successfully allocated according
                     * to file size. Now read file data into buffer. */

                    /* Reset file pointer iterator position first. */
                    if (fseek(pFile, 0, SEEK_SET) == 0 /* Sucess code. */)
                    {
                        /* Read file data into newly allocated buffer. */
                        const size_t eReadBytes = fread(fileBuffer, sizeof (uint8_t), *fileSize, pFile);

                        /* Close input opened file first. */
                        fclose(pFile);

                        if (eReadBytes == *fileSize)
                        {
                            /* All bytes could be read from input file successfully. */

                            /* Finally, return address to buffer so it can be
                             * used by external modules. */

                            printf("%s has been successfully uploaded\n", buffer);
                            return fileBuffer;
                        }
                        else
                        {
                            /* Not all bytes from file were read.
                             * Fall through. */
                        }
                    }
                    else
                    {
                        /* Something went wrong with fseek().
                         * Fall through. */
                    }
                }
                else
                {
                    /* Buffer cannot hold such amount of data.
                     * Fall through. */
                    printf("%s does not fit into internal buffer (%u / %u bytes)\n",
                            buffer, *fileSize, FILE_BUFFER_SIZE);
                }

                /* Set file size to an invalid value. */
                *fileSize = IO_INVALID_FILE_SIZE;
            }
            else
            {
                /* Invalid pointer to file size. */
            }
        }
        else
        {
            /* Something went wrong with fseek().
             * Fall through. */
        }
    }
    else
    {
        /* File does not exist. Fall through. */
        printf("File %s does not exist\n", buffer);
    }

    return NULL;
}

#else /* SERIAL_INTERFACE */

static const uint8_t *IOLoadFileFromSerial(char* const buffer, size_t* const fileSize, uint8_t *const fileBuffer)
{
    uint8_t receivedSizeb[sizeof (uint32_t)] = {0};
    size_t receivedSize = 0;
    size_t i;

    Serial_printf("#%s@", buffer);

    SerialRead(receivedSizeb, sizeof (uint32_t));

    for (i = 0; i < sizeof (uint32_t); i++)
    {
        receivedSize |= receivedSizeb[i] << (i << 3); // (i << 3) == (i * 8)
    }

    SerialWrite(ACK_BYTE_STRING, sizeof (uint8_t));

    if (receivedSize <= FILE_BUFFER_SIZE)
    {
        for (i = 0; i < receivedSize; i += SERIAL_DATA_PACKET_SIZE)
        {
            size_t bytes_to_read;

            // Read actual EXE data into proper RAM address.

            if ( (i + SERIAL_DATA_PACKET_SIZE) >= receivedSize)
            {
                bytes_to_read = receivedSize - i;
            }
            else
            {
                bytes_to_read = SERIAL_DATA_PACKET_SIZE;
            }

            SerialRead(&fileBuffer[i], bytes_to_read);

            SerialWrite(ACK_BYTE_STRING, sizeof (uint8_t)); // Write ACK
        }

        *fileSize = receivedSize;

        return fileBuffer;
    }
    else
    {
        printf("Input file %s cannot be stored into internal buffer\n", buffer);
    }

    *fileSize = 0;

    return NULL;
}

#endif /* SERIAL_INTERFACE */

/*******************************************************************//**
*
* \brief    Loads a file with absolute file path indicated by
*           strFileName.
*
*           File data is stored into a statically-allocated buffer
*           which can then be handled by external modules.
*
* \param    strFilePath
*               Relative file path e.g.:
*               "DATA\\SPRITES\\TILESET1.TIM".
*
* \param    fileSize
*               Pointer to size_t variable where file size will
*               be stored.
*
* \return   Address to a read-only buffer with file data if successful,
*           NULL pointer otherwise.
*
* \return   fileSize is assigned to actual file size in bytes if
*           successful, \ref IO_INVALID_FILE_SIZE otherwise.
*
************************************************************************/
const uint8_t *IOLoadFile(const char* const strFilePath, size_t* const fileSize)
{
    if (strFilePath)
    {
        enum
        {
            MAX_FILE_PATH_LENGTH = 128
        };

        /* This buffer shall be used to concatenate "cdrom:\\"
         * and ";1" substrings along with indicated file path. */
        static char buffer[MAX_FILE_PATH_LENGTH];

        /* Create absolute file path from indicated path. */
        snprintf(buffer, sizeof (buffer), "cdrom:\\%s;1", strFilePath);

        if (buffer != NULL)
        {
            /* This buffer holds file data read from CD-ROM or serial port.
             * It is cleared out on each call to IOLoadFile(),
             * so copy its contents into an auxilar buffer if needed. */
            static uint8_t fileBuffer[FILE_BUFFER_SIZE];

#ifdef SERIAL_INTERFACE
            return IOLoadFileFromSerial(buffer, fileSize, fileBuffer);
#else /* SERIAL_INTERFACE */
            return IOLoadFileFromCd(buffer, fileSize, fileBuffer);
#endif /* SERIAL_INTERFACE */
        }
        else
        {
            /* File path could not be stored. */
        }
    }
    else
    {
        /* Invalid pointer to file path. */
    }

    /* Return failure code if this was reached. */
    return NULL;
}
