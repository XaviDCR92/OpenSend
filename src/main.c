#include "System.h"
#include <stdio.h>

#define PSX_EXE_HEADER_SIZE 2048
#define EXE_DATA_PACKET_SIZE 8

int main(void)
{
    SystemInit();

    for (;;);

#if 0
    {
        uint32_t initPC_Address;
        uint32_t RAMDest_Address;
        uint32_t ExeSize = 0;
        uint32_t i;
        void (*exeAddress)(void);

        GfxSetGlobalLuminance(0);

        SerialInit();

        /* Read PSX-EXE header (32 bytes will be enough). */

        SerialSetState(SERIAL_STATE_READING_HEADER);

        SerialRead(inBuffer, 32);

        /* Get initial program counter address from PSX-EXE header. */

        initPC_Address = (inBuffer[0x10] | (inBuffer[0x11] << 8) | (inBuffer[0x12] << 16) | (inBuffer[0x13] << 24) );

        SerialSetPCAddress(initPC_Address);

        /* dprintf("initPC_Address = 0x%08X\n", initPC_Address); */

        /* Get destination address in RAM from PSX-EXE header. */

        RAMDest_Address = (inBuffer[0x18] | (inBuffer[0x19] << 8) | (inBuffer[0x1A] << 16) | (inBuffer[0x1B] << 24) );

        SerialSetRAMDestAddress(RAMDest_Address);

        /* dprintf("RAMDest_Address = 0x%08X\n", RAMDest_Address); */

        /* We have received all data correctly. Send ACK. */

        memset(inBuffer, 0, SystemGetBufferSize());

        SerialSetState(SERIAL_STATE_WRITING_ACK);

        SerialWrite(ACK_BYTE_STRING, sizeof(uint8_t)); /* Write ACK */

        /* Get PSX-EXE size, without header, in hexadecimal, little-endian format; */

        SerialSetState(SERIAL_STATE_READING_EXE_SIZE);

        SerialRead(inBuffer, sizeof(uint32_t) );

        for (i = 0; i < sizeof(uint32_t); i++)
        {
            ExeSize |= inBuffer[i] << (i << 3); /* (i << 3) == (i * 8) */
        }

        SerialSetExeSize(ExeSize);

        /* DEBUG_PRINT_VAR(ExeSize); */

        SerialSetState(SERIAL_STATE_CLEANING_MEMORY);

        exeAddress = (void*)initPC_Address;

        /* Clean memory where EXE data will be loaded, just in case... */

        memset((void*)RAMDest_Address, 0, (uint32_t)((uint32_t)(&_start) - (uint32_t)(RAMDest_Address) ) );

        SerialSetState(SERIAL_STATE_WRITING_ACK);

        /* We have received PSX-EXE size (without header) correctly. Send ACK. */

        SerialWrite(ACK_BYTE_STRING, sizeof(uint8_t)); /* Write ACK */

        SerialSetState(SERIAL_STATE_READING_EXE_DATA);

        while (GfxIsGPUBusy() == true);

        for (i = 0; i < ExeSize; i += EXE_DATA_PACKET_SIZE)
        {
            uint32_t bytes_to_read;

            /* Read actual EXE data into proper RAM address. */

            if ( (i + EXE_DATA_PACKET_SIZE) >= ExeSize)
            {
                bytes_to_read = ExeSize - i;
            }
            else
            {
                bytes_to_read = EXE_DATA_PACKET_SIZE;
            }

            SerialRead((uint8_t*)RAMDest_Address + i, bytes_to_read);

            SerialSetExeBytesReceived(bytes_to_read);

            SerialWrite(ACK_BYTE_STRING, sizeof(uint8_t)); /* Write ACK */
        }

        SetVBlankHandler(&ISR_SystemDefaultVBlank);

        /* Make a pretty animation before exeting OpenSend application. */

        EndAnimation();

        PSX_DeInit();

        /* PSX-EXE has been successfully loaded into RAM. Run executable! */

        /* dprintf("Entering exe...\n"); */

        exeAddress();
    }
#endif
    return 0;
}
