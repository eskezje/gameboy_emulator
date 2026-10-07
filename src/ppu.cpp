#include <stdint.h>
#include <ppu.h>
#include <memory_bus.h>

gb_ppu_registers* ppu_registers = (gb_ppu_registers*)(&memory[0xFF40]);

uint16_t ppu_line_cycles = 0;

void ppu_advance_clocks(uint8_t cycles)
{
  for (uint8_t c = 0; c< cycles; c++)
  {
    ppu_line_cycles++;
    if (ppu_line_cycles == 456)  {
      ppu_registers->ly++;
      ppu_line_cycles = 0;
      if (ppu_registers->ly == 154) {
        ppu_registers->ly = 0;
      }
      if (ppu_registers->ly >= 144) {
        ppu_set_mode(1);
      }
      else {
      
        if (ppu_line_cycles >=0 && ppu_line_cycles <=79) {
          ppu_set_mode(2);
        }

        else if (ppu_line_cycles >= 80 && ppu_line_cycles <= 251) {
          ppu_set_mode(3);
        }

        else if (ppu_line_cycles >= 252 && ppu_line_cycles <= 455) {
          ppu_set_mode(0);
        }
      }


    }
  }
}


void ppu_set_mode(uint8_t mode)
{
  uint8_t set_value = mode & 0b00000011;
  uint8_t val_without = ppu_registers->stat & 0b11111100;
  ppu_registers->stat = set_value | val_without;
}
