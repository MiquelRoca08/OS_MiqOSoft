#include <stdint.h>
#include <stdio.h>
#include <x86.h>
#include <disk.h>
#include <fat.h>
#include <memdefs.h>
#include <memory.h>
#include <mbr.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include <memdetect.h>
#include <boot/bootparams.h>

uint8_t* KernelLoadBuffer = (uint8_t*)MEMORY_LOAD_KERNEL;
uint8_t* Kernel = (uint8_t*)MEMORY_KERNEL_ADDR;

BootParams g_BootParams;

typedef void (*KernelStart)(BootParams* bootParams);

void __attribute__((cdecl)) start(uint16_t bootDrive, void* partition)
{
    static int boot_count = 0;
    boot_count++;
    
    printf("=== BOOTLOADER START (execution #%d) ===\n", boot_count);
    
    if (boot_count > 1) {
        printf("WARNING: Bootloader restarted! Previous kernel execution failed.\n");
        printf("Press any key to continue or wait 5 seconds...\n");
        
        for (volatile int i = 0; i < 50000000; i++) { /* delay */ }
    }
    
    clrscr();

    printf("Step 1: Initializing disk...\n");
    DISK disk;
    if (!DISK_Initialize(&disk, bootDrive))
    {
        printf("Disk init error\r\n");
        goto end;
    }

    printf("Step 2: Detecting partition...\n");
    Partition part;
    MBR_DetectPartition(&part, &disk, partition);

    printf("Step 3: Initializing FAT...\n");
    if (!FAT_Initialize(&part))
    {
        printf("FAT init error\r\n");
        goto end;
    }

    printf("Step 4: Preparing boot params...\n");
    g_BootParams.BootDevice = bootDrive;
    
    printf("Step 5: Detecting memory...\n");
    Memory_Detect(&g_BootParams.Memory);
    printf("Memory detection completed.\n");

    printf("Step 6: Loading kernel ELF...\n");
    KernelStart kernelEntry;
    if (!ELF_Read(&part, "/boot/kernel.elf", (void**)&kernelEntry))
    {
        printf("ELF read failed, booting halted!");
        goto end;
    }

    printf("Step 7: Kernel loaded at address: 0x%x\n", (uint32_t)kernelEntry);
    
    if ((uint32_t)kernelEntry < 0x100000) {
        printf("WARNING: Kernel address 0x%x seems too low! Expected >= 0x100000\n", (uint32_t)kernelEntry);
    }

    printf("Checking kernel memory at 0x%x:\n", (uint32_t)kernelEntry);
    uint32_t* kernelPtr = (uint32_t*)kernelEntry;
    
    printf("  First 16 bytes: ");
    for (int i = 0; i < 4; i++) {
        printf("0x%x ", kernelPtr[i]);
    }
    printf("\n");
    
    bool allZeros = true;
    for (int i = 0; i < 16; i++) {
        if (((uint8_t*)kernelPtr)[i] != 0) {
            allZeros = false;
            break;
        }
    }
    
    if (allZeros) {
        printf("ERROR: Kernel memory is all zeros! ELF loading failed.\n");
        printf("Possible causes:\n");
        printf("  1. kernel.elf file is empty or corrupted\n");
        printf("  2. ELF_Read() function has a bug\n");
        printf("  3. Wrong memory address calculation\n");
        goto end;
    }
    
    printf("Step 8: Kernel data looks valid, proceeding...\n");
    
    printf("Step 9: Executing kernel NOW!\n");
    kernelEntry(&g_BootParams);
    
    printf("ERROR: Kernel returned to bootloader! This should not happen.\n");

end:
    printf("Bootloader ended. System halted.\n");
    for (;;);
}