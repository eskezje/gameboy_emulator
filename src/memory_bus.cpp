#include <cstdint>
#include <cstdio>
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

uint8_t memory_bus_read(const uint16_t addr)
{
  if (addr >= 0x0000 && addr <= 0x3FFF) {   // read from rom bank 00
    return cartridge_data[addr];
  }
    return cartridge_data[addr];
}

void memory_bus_write(const uint16_t addr, const uint8_t value)
{
  if (addr == 0xFF02 && value == 0x81) // blargg tests serial output
  {
    char c = memory[0xFF01];
    printf("%c", c);
  }
  memory[addr] =value;
}
