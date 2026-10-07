#include <ppu.h>
#include <memory_bus.h>

gb_ppu_registers* ppu_registers = (gb_ppu_registers*)(&memory[0xFF40]);