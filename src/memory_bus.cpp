#include <ppu.h>
#include <cart.h>
#include <cart_type.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <interrupts.h>
#include <memory_bus.h>
#include <timer.h>

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
// 0xFE00-0xFE9F is Object attribute memory (OAM)
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

uint16_t rom_bank_number = 0;
uint16_t ram_bank_number = 0;
bool ram_enable = false;
bool rom_ram_mode_select = false;

uint8_t memory_bus_read(const uint16_t addr)
{
  if (addr >= 0x0000 && addr <= 0x3FFF) {   // Read from ROM bank 00
    return cartridge_data[addr];
  }

  const cart_type_info& cart_info = cart_type_data[cartridge_header->cartridge_type];
  uint8_t ppu_mode = ppu_registers->stat & 0b11;

  if (addr >= 0x4000 && addr <= 0x7FFF) {   // Read from ROM bank 01-NN
    if (cart_info.type == CART_TYPE::NO_MBC) {
      return cartridge_data[addr];    // For no MBC cartridges
    }
    else if (cart_info.type == CART_TYPE::MBC1) {
      const uint16_t ROM_BANK_SIZE = 0x4000; // 16k per ROM bank
      uint8_t rom_bank = rom_bank_number > 0 ? rom_bank_number : 1;
      const uint32_t offset = ROM_BANK_SIZE * (rom_bank - 1);
      return cartridge_data[offset + addr];
    }
  }

  if (addr >= 0x8000 && addr <= 0x9FFF) {   // VRAM
    if (ppu_mode == 3) {
      return 0xFF;
    }
    return memory[addr];
  }

  if (addr >= 0xA000 && addr <= 0xBFFF) {   // External RAM
    if (cart_info.type == CART_TYPE::NO_MBC) {
      const uint16_t eram_address = (addr - 0xA000);
      return eram[eram_address];
    }
    else if (cart_info.type == CART_TYPE::MBC1) {
      if (ram_enable) {
        const uint16_t bank_offset = ram_bank_number * 0x2000;
        const uint16_t eram_address = (addr - 0xA000) + bank_offset;
        return eram[eram_address];
      }
      else {
        return 0xFF;
      }
    }
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

  if (addr >= 0xFE00 && addr <= 0xFE9F) {   // Object attribute memory (40 sprites)
    if (ppu_mode == 2 || ppu_mode == 3) {
      return 0xFF;
    }
    return memory[addr];
  }

  if (addr >= 0xFEA0 && addr <= 0xFEFF) {   // Unuseable memory area
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
  const cart_type_info& cart_info = cart_type_data[cartridge_header->cartridge_type];
  uint8_t ppu_mode = ppu_registers->stat & 0b11;
  if (cart_info.type == CART_TYPE::MBC1) {
    if (addr >= 0x0000 && addr <= 0x1FFF) {   // RAM enable
      ram_enable = (value & 0x0F) == 0x0A;
    }
    if (addr >= 0x2000 && addr <= 0x3FFF) {
      rom_bank_number = value & 0b00011111; // this is a 5 bit register
      const uint32_t rom_size = 32 * (1 << cartridge_header->rom_size);
      const uint8_t number_of_rom_banks = rom_size / 16;
      rom_bank_number = rom_bank_number % number_of_rom_banks;
    }
    if (addr >= 0x4000 && addr <= 0x5FFF) {
      if (rom_ram_mode_select) {
        ram_bank_number = value & 0b00000011; // this is a 2 bit register
      }
      else {
        rom_bank_number = (rom_bank_number & 0b00011111) | ((value & 0b00000011) << 5);
      }
    }
    if (addr >= 0x6000 && addr <= 0x7FFF) {
      rom_ram_mode_select = value & 1; // this is a 1 bit reigster
    }
  }


  if (addr >= 0x8000 && addr <= 0x9FFF) {   // VRAM
    if (ppu_mode == 3) {
      return;
    }
    memory[addr] = value;
  }

  if (addr >= 0xA000 && addr <= 0xBFFF) {   // External RAM
    if (cart_info.type == CART_TYPE::NO_MBC) {
      eram[addr - 0xA000] = value;
    }
    else if (cart_info.type == CART_TYPE::MBC1) {
      if (ram_enable) {
        const uint16_t RAM_BANK_SIZE = 0x2000;    // 8k per ROM bank
        const uint32_t offset = RAM_BANK_SIZE * ram_bank_number;
        const uint16_t eram_address = offset + (addr - 0xA000);
        eram[eram_address] = value;
      }
      else {
        // do nothing
      }
    }
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

  if (addr >= 0xFE00 && addr <= 0xFE9F) {   // Object attribute memory (40 sprites)
    if (ppu_mode == 2 || ppu_mode == 3) {
      return;
    }
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
    else if (addr == 0xFF41)
    {
      uint8_t ppu_bits = memory[addr] & 0b00000111;
      uint8_t writable_bits = value & 0b01111000;
      memory[addr] = ppu_bits | writable_bits | 0b10000000;

      ppu_update_stat_interrupt();
    }
    else if (addr == 0xFF44) {
      // this is read only
      return;
    }
    else if (addr == 0xFF45) {
      memory[addr] = value;
      ppu_update_lyc_flag();
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
