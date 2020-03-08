#include "System.h"
#include "reception.h"

#define PSX_EXE_HEADER_SIZE 2048
#define EXE_DATA_PACKET_SIZE 8

int main(void)
{
    SystemInit();
    reception_loop();
    return 0;
}
