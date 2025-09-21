#include <x86.h>
#include <stdio.h>
#include <boot/bootparams.h>

#define MAX_REGIONS 256

MemoryRegion g_MemRegions[MAX_REGIONS];
int g_MemRegionCount;

// Variable estática para evitar múltiples ejecuciones
static int memory_detection_done = 0;

void Memory_Detect(MemoryInfo* memoryInfo)
{
    // SI YA SE EJECUTÓ, NO HACER NADA MÁS
    if (memory_detection_done) {
        printf("Memory_Detect: Already executed, returning cached results\n");
        memoryInfo->RegionCount = g_MemRegionCount;
        memoryInfo->Regions = g_MemRegions;
        return;
    }
    
    printf("Memory_Detect: First execution, detecting memory...\n");
    
    E820MemoryBlock block;
    uint32_t continuation = 0;
    int ret;
    
    g_MemRegionCount = 0;
    
    do {
        ret = x86_E820GetNextBlock(&block, &continuation);
        
        if (ret > 0) {
            // Verificar límites del array
            if (g_MemRegionCount >= MAX_REGIONS) {
                printf("WARNING: Maximum memory regions (%d) exceeded!\n", MAX_REGIONS);
                break;
            }
            
            g_MemRegions[g_MemRegionCount].Begin = block.Base;
            g_MemRegions[g_MemRegionCount].Length = block.Length;
            g_MemRegions[g_MemRegionCount].Type = block.Type;
            g_MemRegions[g_MemRegionCount].ACPI = block.ACPI;
            g_MemRegionCount++;

            // Solo mostrar las primeras veces para reducir spam
            if (g_MemRegionCount < 10) {
                printf("E820: base=0x%llx length=0x%llx type=0x%x\n", 
                       block.Base, block.Length, block.Type);
            }
        }
        
    } while (ret > 0 && continuation != 0);

    printf("Memory detection completed: %d regions found\n", g_MemRegionCount);
    
    // Marcar como completado
    memory_detection_done = 1;

    // fill meminfo structure
    memoryInfo->RegionCount = g_MemRegionCount;
    memoryInfo->Regions = g_MemRegions;
    
    printf("Memory_Detect: Function completed, continuing boot...\n");
}