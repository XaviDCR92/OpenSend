/* *************************************
 * 	Includes
 * *************************************/

#include "Serial.h"

/* *************************************
 * 	Defines
 * *************************************/

#define SERIAL_BAUDRATE 115200
#define SERIAL_TX_RX_TIMEOUT 20000
#define SERIAL_RX_FIFO_EMPTY 0
#define SERIAL_TX_NOT_READY 0

/* *************************************
 * 	Local Variables
 * *************************************/

static volatile SERIAL_STATE SerialState;
static volatile size_t bytesRead;
static volatile uint32_t initPC_Address;
static volatile uint32_t RAMDest_Address;
static volatile size_t ExeSize;
static volatile size_t totalBytes;
static volatile size_t exeBytesRead;
static volatile bool serial_busy;

/* *************************************
 * 	Local Prototypes
 * *************************************/

void ISR_Serial(void)
{
    enum
    {
        SERIAL_BG_X0 = 0,
        SERIAL_BG_X1 = X_SCREEN_RESOLUTION - SERIAL_BG_X0,
        SERIAL_BG_X2 = SERIAL_BG_X0,
        SERIAL_BG_X3 = SERIAL_BG_X1,

        SERIAL_BG_Y0 = 0,
        SERIAL_BG_Y1 = SERIAL_BG_Y0,
        SERIAL_BG_Y2 = Y_SCREEN_RESOLUTION - SERIAL_BG_Y0,
        SERIAL_BG_Y3 = SERIAL_BG_Y2,

        SERIAL_BG_R = 0,
        SERIAL_BG_G = NORMAL_LUMINANCE,
        SERIAL_BG_B = NORMAL_LUMINANCE,
    };

    static GsGPoly4 SerialBg = {    .x[0] = SERIAL_BG_X0,
                                    .x[1] = SERIAL_BG_X1,
                                    .x[2] = SERIAL_BG_X2,
                                    .x[3] = SERIAL_BG_X3,

                                    .y[0] = SERIAL_BG_Y0,
                                    .y[1] = SERIAL_BG_Y1,
                                    .y[2] = SERIAL_BG_Y2,
                                    .y[3] = SERIAL_BG_Y3,

                                    .r[0] = 0,
                                    .r[1] = 0,
                                    .r[2] = SERIAL_BG_R,
                                    .r[3] = SERIAL_BG_R,

                                    .g[0] = 0,
                                    .g[1] = 0,
                                    .g[2] = SERIAL_BG_G,
                                    .g[3] = SERIAL_BG_G,

                                    .b[0] = 0,
                                    .b[1] = 0,
                                    .b[2] = SERIAL_BG_B,
                                    .b[3] = SERIAL_BG_B, };

    enum
    {
        SERIAL_STATE_TEXT_X = 148,
        SERIAL_STATE_TEXT_Y = Y_SCREEN_RESOLUTION >> 1,
    };

    SystemIncreaseGlobalTimer();

    if( (GfxIsGPUBusy() == true) || (SystemIsBusy() == true) )
    {
        return;
    }

    FontSetFlags(&SmallFont, FONT_BLEND_EFFECT | FONT_H_CENTERED);

    if(SerialState == SERIAL_STATE_READING_EXE_DATA)
    {
        if(System1SecondTick() == false)
        {
            return;
        }
        else
        {
            FontSetFlags(&SmallFont, FONT_H_CENTERED);
        }
    }

    GsSortGPoly4(&SerialBg);

    switch(SerialState)
    {
        case SERIAL_STATE_INIT:
            FontPrintText(&SmallFont, SERIAL_STATE_TEXT_X, SERIAL_STATE_TEXT_Y, "Serial initialization");
        break;

        case SERIAL_STATE_STANDBY:
            FontPrintText(&SmallFont, SERIAL_STATE_TEXT_X, SERIAL_STATE_TEXT_Y, "Waiting for PC...");
        break;

        case SERIAL_STATE_WRITING_ACK:
            FontPrintText(&SmallFont, SERIAL_STATE_TEXT_X, SERIAL_STATE_TEXT_Y, "Writing ACK");
        break;

        case SERIAL_STATE_READING_HEADER:
            FontPrintText(&SmallFont, SERIAL_STATE_TEXT_X, SERIAL_STATE_TEXT_Y, "Reading data from header (%d/%d bytes)...", bytesRead, totalBytes);
        break;

        case SERIAL_STATE_READING_EXE_SIZE:
            FontPrintText(&SmallFont, SERIAL_STATE_TEXT_X, SERIAL_STATE_TEXT_Y, "Getting PSX-EXE size from PC...");
        break;

        case SERIAL_STATE_READING_EXE_DATA:
            FontPrintText(&SmallFont, SERIAL_STATE_TEXT_X, SERIAL_STATE_TEXT_Y, "Reading PSX-EXE data (%d/%d bytes)...", exeBytesRead, ExeSize);
        break;

        case SERIAL_STATE_WAITING_USER_INPUT:
            FontPrintText(&SmallFont, SERIAL_STATE_TEXT_X, SERIAL_STATE_TEXT_Y, "Press any key to continue");
        break;

        case SERIAL_STATE_CLEANING_MEMORY:
            FontPrintText(&SmallFont, SERIAL_STATE_TEXT_X, SERIAL_STATE_TEXT_Y, "Cleaning RAM before EXE data transfer...");
        break;
        
        default:
            FontPrintText(&SmallFont, SERIAL_STATE_TEXT_X, SERIAL_STATE_TEXT_Y, "Unknown state");
        break;
    }

    FontSetFlags(&SmallFont, FONT_H_CENTERED);

    if(RAMDest_Address != 0)
    {
        FontPrintText(&SmallFont, SERIAL_STATE_TEXT_X, SERIAL_STATE_TEXT_Y + 16, "RAM Dest address: 0x%08X", RAMDest_Address);
    }

    if(initPC_Address != 0)
    {
        FontPrintText(&SmallFont, SERIAL_STATE_TEXT_X, SERIAL_STATE_TEXT_Y + 32, "Init PC address: 0x%08X", initPC_Address);
    }
    
    if(ExeSize != 0)
    {
        FontPrintText(&SmallFont, SERIAL_STATE_TEXT_X, SERIAL_STATE_TEXT_Y + 48, "PSX-EXE size: 0x%08X", ExeSize);
    }

    GfxDrawScene_Fast();
}

void SerialSetState(SERIAL_STATE state)
{
    SerialState = state;
}

void SerialSetPCAddress(uint32_t addr)
{
    initPC_Address = addr;
}

void SerialSetRAMDestAddress(uint32_t addr)
{
    RAMDest_Address = addr;
}

void SerialSetExeSize(size_t size)
{
    ExeSize = size;
}

void SerialInit(void)
{
    uint8_t receivedBytes;

    SetVBlankHandler(&ISR_Serial);

    SerialState = SERIAL_STATE_INIT;

    SIOStart(SERIAL_BAUDRATE);

    SerialState = SERIAL_STATE_STANDBY;

    // ------------------------------------
    //  Protocol description
    // ------------------------------------

    // 1. Wait to receive magic byte "99" from PC.

    SerialRead(&receivedBytes, sizeof(uint8_t) );

    if(receivedBytes != 99)
    {
        dprintf("Did not receive input magic number!\n");
        return;
    }

    // 2. Send ACK (magic byte is ASCII code for 'b').

    SerialState = SERIAL_STATE_WRITING_ACK;

    SerialWrite(ACK_BYTE_STRING, sizeof(uint8_t) );
}

void SerialSetExeBytesReceived(uint32_t bytes_read)
{
    exeBytesRead += bytes_read;
}

bool SerialRead(uint8_t* ptrArray, size_t nBytes)
{
    bytesRead = 0;
    totalBytes = nBytes;

    serial_busy = true;

    if(nBytes == 0)
    {
        SerialWrite("SerialRead: invalid size %d\n", strnlen("SerialRead: invalid size %d\n", 30));
        return false;
    }

    do
    {
        //uint16_t timeout = SERIAL_TX_RX_TIMEOUT;
        
        while( (SIOCheckInBuffer() == SERIAL_RX_FIFO_EMPTY)); // Wait for RX FIFO not empty

        *(ptrArray++) = SIOReadByte();
        bytesRead++;
    }while(--nBytes);

    serial_busy = false;

    return true;
}

bool SerialWrite(void* ptrArray, size_t nBytes)
{
    serial_busy = true;

    SystemDisableVBlankInterrupt();

    if(nBytes == 0)
    {
        SerialWrite("SerialRead: invalid size %d\n", strnlen("SerialRead: invalid size %d\n", 30));
        return false;
    }

    do
    {
        //uint16_t timeout = SERIAL_TX_RX_TIMEOUT;

        while( (SIOCheckOutBuffer() == SERIAL_TX_NOT_READY)); // Wait for TX FIFO empty.

        SIOSendByte(*(uint8_t*)ptrArray++);

    }while(--nBytes);

    SystemEnableVBlankInterrupt();

    serial_busy = false;

    return true;
}
