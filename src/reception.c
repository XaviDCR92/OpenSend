#include "reception.h"
#include "Serial.h"
#include <stdbool.h>

static volatile bool rx;

static enum
{

} state;

void reception_ev(void)
{
    rx = true;
}

void reception_loop(void)
{
    for (;;)
    {
        if (rx)
        {

        }
    }
}
