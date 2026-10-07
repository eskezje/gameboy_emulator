#include <stdint.h>
#include <ppu.h>
#include <memory_bus.h>
#include <interrupts.h>

gb_ppu_registers* ppu_registers = (gb_ppu_registers*)(&memory[0xFF40]);

uint16_t ppu_line_cycles = 0;
bool ppu_lcd_enabled = false;

void ppu_set_mode(uint8_t mode)
{
  uint8_t set_value = mode & 0b00000011;
  uint8_t val_without = ppu_registers->stat & 0b11111100;
  ppu_registers->stat = set_value | val_without;
}


void ppu_advance_clocks(uint8_t cycles)
{
  bool lcd_enabled = CHECK_BIT(ppu_registers->lcdc, 7);

  if (!lcd_enabled) {
      ppu_line_cycles = 0;
      ppu_registers->ly = 0;
      ppu_set_mode(0);

      ppu_lcd_enabled = false;
      return;
  }

  if (!ppu_lcd_enabled) {
    // lcd was just turned on
    ppu_line_cycles = 0;
    ppu_registers->ly = 0;
    ppu_set_mode(2);

    ppu_lcd_enabled = true;
  }
  for (uint8_t c = 0; c< cycles; c++)
  {
    ppu_line_cycles++;
    // visiable scanlines
    if (ppu_registers->ly < 144) {
      if (ppu_line_cycles == 80) {
        // we go from OAM search to pixel transfer
        ppu_set_mode(3);
      }
      else if (ppu_line_cycles == 252) {
        // we go from pixel transfer to HBlank
        ppu_set_mode(0);

      }
    }
 
    // end of scanlines
    if (ppu_line_cycles == 456) {
      ppu_line_cycles = 0;
      ppu_registers->ly++;

      if (ppu_registers->ly == 144) {
        // enter vblank mode 
        ppu_set_mode(1);
        interrupt_raise_flag(INTERRUPT_FLAG_V_BLANK);
      }
      else if (ppu_registers->ly == 154) {
        // end of vblank so we start a new frame 
        ppu_registers->ly = 0;
        ppu_set_mode(2);
      }
      else if (ppu_registers->ly < 144) {
        // we start another visiable scanline
        ppu_set_mode(2);
      }
    }

  }
}


void ppu_init()
{
    ppu_line_cycles = 0;
    ppu_registers->ly = 0;
    ppu_set_mode(0);
    ppu_lcd_enabled = false;
}
