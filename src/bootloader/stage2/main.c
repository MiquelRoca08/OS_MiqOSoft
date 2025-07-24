#include "stdint.h"
#include "stdio.h"

void _cdecl cstart_(uint16_t bootDrive)
{
    (void)bootDrive; // Unused parameter, silence compiler warning
    puts("Hello world from C!\r\n");
    for (;;);
};
