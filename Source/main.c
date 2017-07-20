/* *************************************
 * 	Includes
 * *************************************/

#include "Global_Inc.h"
#include "System.h"
#include "Serial.h"
#include "LoadMenu.h"
#include "EndAnimation.h"

/* *************************************
 * 	Defines
 * *************************************/

#define PSX_EXE_HEADER_SIZE 2048
#define EXE_DATA_PACKET_SIZE 8

/* *************************************
 * 	Local Prototypes
 * *************************************/

/* *************************************
 * 	Local Variables
 * *************************************/

 /* Untitled1 (10/07/2017 18:57:47)
   StartOffset: 00000000, EndOffset: 0000002F, Length: 00000030 */

/* Untitled2 (10/07/2017 21:10:19)
   StartOffset: 00000000, EndOffset: 000357FF, Length: 00035800 */

extern void _start(void);



int main(void)
{
    uint8_t* inBuffer = SystemGetBufferAddress();
   // int (*exeAddress)(void);

	//System initialization
    dprintf("SystemInit()\n");
	SystemInit();

    dprintf("LoadMenuInit()\n");

    LoadMenuInit();

    if(1)
    {
        uint32_t initPC_Address;
        uint32_t RAMDest_Address;
        uint32_t ExeSize = 0;
        uint32_t i;
        void (*exeAddress)(void);

        SerialInit();

        // Read PSX-EXE header (32 bytes will be enough).

        SerialSetState(SERIAL_STATE_READING_HEADER);

        SerialRead(inBuffer, 32);

        // Get initial program counter address from PSX-EXE header.

        initPC_Address = (inBuffer[0x10] | (inBuffer[0x11] << 8) | (inBuffer[0x12] << 16) | (inBuffer[0x13] << 24) );

        SerialSetPCAddress(initPC_Address);

        //dprintf("initPC_Address = 0x%08X\n", initPC_Address);

        // Get destination address in RAM from PSX-EXE header.

        RAMDest_Address = (inBuffer[0x18] | (inBuffer[0x19] << 8) | (inBuffer[0x1A] << 16) | (inBuffer[0x1B] << 24) );

        SerialSetRAMDestAddress(RAMDest_Address);

        //dprintf("RAMDest_Address = 0x%08X\n", RAMDest_Address);

        // We have received all data correctly. Send ACK.

        memset(inBuffer, 0, SystemGetBufferSize());

        SerialSetState(SERIAL_STATE_WRITING_ACK);

        SerialWrite(ACK_BYTE_STRING, sizeof(uint8_t)); // Write ACK

        // Get PSX-EXE size, without header, in hexadecimal, little-endian format;

        SerialSetState(SERIAL_STATE_READING_EXE_SIZE);

        SerialRead(inBuffer, sizeof(uint32_t) );

        for(i = 0; i < sizeof(uint32_t); i++)
        {
            ExeSize |= inBuffer[i] << (i << 3); // (i << 3) == (i * 8)
        }

        SerialSetExeSize(ExeSize);

        //DEBUG_PRINT_VAR(ExeSize);

        SerialSetState(SERIAL_STATE_CLEANING_MEMORY);

        exeAddress = (void*)initPC_Address;

        // Clean memory where EXE data will be loaded, just in case...

        memset((void*)RAMDest_Address, 0, (uint32_t)((uint32_t)(&_start) - (uint32_t)(RAMDest_Address) ) );

        SerialSetState(SERIAL_STATE_WRITING_ACK);

        // We have received PSX-EXE size (without header) correctly. Send ACK.

        SerialWrite(ACK_BYTE_STRING, sizeof(uint8_t)); // Write ACK

        SerialSetState(SERIAL_STATE_READING_EXE_DATA);

        SystemDisableVBlankInterrupt();

        while(GfxIsGPUBusy() == true);

        for(i = 0; i < ExeSize; i += EXE_DATA_PACKET_SIZE)
        {
            uint32_t bytes_to_read;

            // Read actual EXE data into proper RAM address.

            if( (i + EXE_DATA_PACKET_SIZE) >= ExeSize)
            {
                bytes_to_read = ExeSize - i;
            }
            else
            {
                bytes_to_read = EXE_DATA_PACKET_SIZE;
            }

            SerialRead((uint8_t*)RAMDest_Address + i, bytes_to_read);

            SerialSetExeBytesReceived(bytes_to_read);

            SerialWrite(ACK_BYTE_STRING, sizeof(uint8_t)); // Write ACK
        }

        SystemEnableVBlankInterrupt();

        //SystemLoadFileToBuffer("cdrom:\\AIRPORT.EXE;1", 2048, (uint8_t*)0x80010000, (uint32_t) (0x801A0000 - 0x80010000) );

        SetVBlankHandler(&ISR_SystemDefaultVBlank);

        // Make a pretty animation before exeting OpenSend application.

        EndAnimation();

        // PSX-EXE has been successfully loaded into RAM. Run executable!

        //dprintf("Entering exe...\n");
        
        exeAddress();
    }
		
	return 0;
}
