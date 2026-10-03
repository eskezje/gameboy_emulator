#include "interrupts.h"
#include "timer.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory_bus.h>
#include <cart.h>

// https://gbdev.io/pandocs/Memory_Map.html
// 0x0000-0x3FFF is 16 KiB ROM bank 00 - From cartridge, usually a fixed bank
//
// 0x4000-0x7FFF is 16 KiB ROM Bank 01–NN - From cartridge, switchable bank via mapper (if any)
//
// 0x8000-0x9FFF is 8 KiB Video RAM (VRAM) - In CGB mode, switchable bank 0/1
//
// 0xA000-0xBFFF is 8 KiB External RAM - From cartridge, switchable bank if any
//
// 0xC000-0xCFFF is 4 KiB Work RAM (WRAM)
//
// 0xD000-0xDFFF is 4 KiB Work RAM (WRAM)  - In CGB mode, switchable bank 1–7
//
// 0xE000-0xFDFF is Echo RAM (mirror of C000–DDFF) - Nintendo says use of this area is prohibited.
//
// 0xFE00-0xFE0F is Object attribute memory (OAM)
//
// 0xFEA0-0xFEFF is Not Useable - Nintendo says use of this area is prohibited.
//
// 0xFF00-0xFF7F is I/O registers
//
// 0xFF80-0xFFFE is High RAM (HRAM)
//
// 0xFFFF-0xFFFF is Interrupt Enable register (IE)

uint8_t memory[MEMORY_SIZE];
uint8_t eram[ERAM_SIZE];

uint8_t memory_bus_read(const uint16_t addr)
{
  if (addr >= 0x0000 && addr <= 0x3FFF) {   // Read from ROM bank 00
    return cartridge_data[addr];
  }

  if (addr >= 0x4000 && addr <= 0x7FFF) {   // Read from ROM bank 01-NN
    return cartridge_data[addr];    // For no MBC cartridges
  }

  if (addr >= 0x8000 && addr <= 0x9FFF) {   // VRAM
    return memory[addr];
  }

  if (addr >= 0xA000 && addr <= 0xBFFF) {   // External RAM
    const uint16_t eram_address = (addr - 0xA000);
    return eram[eram_address];
  }

  if (addr >= 0xC000 && addr <= 0xCFFF) {   // Work RAM 1
    return memory[addr];
  }

  if (addr >= 0xD000 && addr <= 0xDFFF) {   // Work RAM 2
    return memory[addr];
  }

  if (addr >= 0xE000 && addr <= 0xFDFF) {   // Echo RAM (mirror of C000-DDFF)
    return memory[addr - 0x2000];
  }

  if (addr >= 0xFE00 && addr <= 0xFE0F) {   // Object attribute memory (40 sprites)
    // TODO: If PPU mode == 2 return 0xFF
    return memory[addr];
  }

  if (addr >= 0xFEA0 && addr <= 0xFEFF) {   // Unuseable memory area
    // TODO: If PPU mode == 3 return 0xFF
    return 0x00;
  }

  if (addr >= 0xFF00 && addr <= 0xFF7F) {   // I/O registers
    return memory[addr];
  }

  if (addr >= 0xFF80 && addr <= 0xFFFE) {   // High RAM
    return memory[addr];
  }
  
  if (addr >= 0xFFFF && addr <= 0xFFFF) {   // Interrupt enable reigster
    return memory[addr];
  }

    return memory[addr];
}

void memory_bus_write(const uint16_t addr, const uint8_t value)
{
  if (addr >= 0x8000 && addr <= 0x9FFF) {   // VRAM
    // TODO: If PPU is in mode 3 the cpu cannot access VRAM
    memory[addr] = value;
  }

  if (addr >= 0xA000 && addr <= 0xBFFF) {   // External RAM
    const uint16_t eram_address = (addr - 0xA000);
    eram[eram_address] = value;
  }

  if (addr >= 0xC000 && addr <= 0xCFFF) {   // Work RAM 1
    memory[addr] = value;
  }

  if (addr >= 0xD000 && addr <= 0xDFFF) {   // Work RAM 2
    memory[addr] = value;
  }

  if (addr >= 0xE000 && addr <= 0xFDFF) {   // Echo RAM (mirror of C000-DDFF)
    memory[addr - 0x2000] = value;
  }

  if (addr >= 0xFE00 && addr <= 0xFE0F) {   // Object attribute memory (40 sprites)
    // TODO: If PPU mode == 2 or mode == 3 then the CPU cannot access OAM
    memory[addr] = value;
  }

  if (addr >= 0xFEA0 && addr <= 0xFEFF) {   // Unuseable memory area
    // nothing happens here, its all ignored
  }

  if (addr >= 0xFF00 && addr <= 0xFF7F) {   // I/O registers
    if (addr == 0xFF02 && value == 0x81) // blargg tests serial output
    {
      char c = memory[0xFF01];
      printf("%c", c);
    }
    else if (addr == 0xFF04) {   // timer div
      timer_on_div_write(value);
    }
    else if (addr == 0xFF07) {  // Timer control
      memory[addr] = value & 0b00000111;
    }
    else if (addr == 0xFF0F) {
      interrupt_flag_write(value);
    }
    else if (addr == 0xFF46) {  // OAM DMA
      uint16_t source_addr = (value * 0x100);
      const uint16_t dest = 0xFE00;
      for (uint8_t i = 0 ; i < 160; i++) {
        memory[dest + i] = memory_bus_read(source_addr + i);
      }
    }
    else {
      memory[addr] =value;
    }

  }

  if (addr >= 0xFF80 && addr <= 0xFFFE) {   // HRAM
    memory[addr] = value;
  }

  if (addr >= 0xFFFF && addr <= 0xFFFF) {   // Interrupt enable 
    interrupt_enable_write(value);
  }


}
