#include <cstdint>
#include <stdint.h>
#include <ppu.h>
#include <memory_bus.h>
#include <interrupts.h>

gb_ppu_registers* ppu_registers = (gb_ppu_registers*)(&memory[0xFF40]);

uint16_t ppu_line_cycles = 0;
bool ppu_lcd_enabled = false;

bool ppu_stat_irq_line = false;

// this is ppu_framebuffer[y][x]
// y = 0..143
// x = 0..159
// value =
// 0 = color 0
// 1 = color 1
// 2 = color 2
// 3 = color 3
uint8_t ppu_framebuffer[144][160];
bool ppu_frame_ready = false;


void ppu_update_stat_interrupt()
{
  if (!CHECK_BIT(ppu_registers->lcdc, 7)) {
      ppu_stat_irq_line = false;
      return;
  }

  bool stat_high = false;
  uint8_t mode = ppu_registers->stat & 0b11;

  if ((ppu_registers->ly == ppu_registers->lyc) &&
      CHECK_BIT(ppu_registers->stat, 6)) {
      stat_high = true;
  }
  else if (mode == 0 && CHECK_BIT(ppu_registers->stat, 3)) {
      stat_high = true;
  }
  else if (mode == 2 && CHECK_BIT(ppu_registers->stat, 5)) {
      stat_high = true;
  }
  else if (mode == 1 && CHECK_BIT(ppu_registers->stat, 4)) {
      stat_high = true;
  }

  if (stat_high && !ppu_stat_irq_line) {
      interrupt_raise_flag(INTERRUPT_FLAG_STAT);
  }

  ppu_stat_irq_line = stat_high;
}

void ppu_set_mode(uint8_t mode)
{
  uint8_t set_value = mode & 0b00000011;
  uint8_t val_without = ppu_registers->stat & 0b11111100;
  ppu_registers->stat = set_value | val_without;
  ppu_update_stat_interrupt();
}

DmgShade bgp_get_shade(uint8_t bgp, uint8_t color_id)
{
  // match the color id to the shade
  // like color id 1 matches to light gray
  uint8_t shift = color_id * 2;
  // we then get bgp shifted into the right place where we can get the right shade for that color id
  uint8_t shade = (bgp >> shift) & 0b11;

  return (DmgShade)(shade);
}


void ppu_render_scanline()
{
  uint16_t addr;
  uint8_t ly = ppu_registers->ly;
  uint8_t bg_y = ppu_registers->scy + ly;   // top left plus the offset of the scroll

  uint8_t tile_y = bg_y/8;  // each tile is 8 tall, we later have to deal with potential sprites of 8x16
  uint8_t row_in_tile = bg_y % 8;   // we get what row of the 8 rows in the tile we are drawing
  uint16_t tilemap_base = CHECK_BIT(ppu_registers->lcdc, 3) ? 0x9C00 : 0x9800;  // we figure out what base we are usign

  bool tile_data_unsigned = CHECK_BIT(ppu_registers->lcdc, 4);  // $8000 method and $8800 method
  bool bg_enabled = CHECK_BIT(ppu_registers->lcdc, 0);
  uint8_t bgp = memory[0xFF47];
  if (!bg_enabled) {
    for (uint16_t x = 0; x < 160; x++) {
      ppu_framebuffer[ly][x] = 0;
    }
    return;
  }

  for (uint16_t x = 0; x < 160; x++) {
    uint8_t bg_x = ppu_registers->scx + x; // we offset it by the scrolls
    uint8_t tile_x = bg_x / 8;
    uint8_t pixel_x_inside_tile = bg_x % 8;
    uint16_t address_map_entry = tilemap_base + 32*tile_y + tile_x;
    uint8_t tile_number = memory[address_map_entry];
    if (tile_data_unsigned) {
      addr = 0x8000 + 16 * tile_number;
    }
    else {
      addr = 0x9000 + ((int8_t)tile_number) * 16;
    }
    uint8_t low_byte  = memory[addr + row_in_tile * 2];
    uint8_t high_byte = memory[addr + row_in_tile * 2 + 1];

    uint8_t bit = 7 - pixel_x_inside_tile;

    uint8_t low_bit = (low_byte >> bit) & 1;
    uint8_t high_bit = (high_byte >> bit) & 1;

    uint8_t color = low_bit | (high_bit << 1);
    DmgShade shade = bgp_get_shade(bgp, color);

    ppu_framebuffer[ly][x] = (uint8_t)shade;
  }
}

void ppu_advance_clocks(uint8_t cycles)
{
  bool lcd_enabled = CHECK_BIT(ppu_registers->lcdc, 7);

  if (!lcd_enabled) {
      ppu_line_cycles = 0;
      ppu_registers->ly = 0;
      ppu_set_mode(0);
      ppu_lcd_enabled = false;

      ppu_update_lyc_flag();
      return;
  }

  if (!ppu_lcd_enabled) {
    // lcd was just turned on
    ppu_line_cycles = 0;
    ppu_registers->ly = 0;
    ppu_set_mode(2);

    ppu_update_lyc_flag();
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
        // finished trasfering/rendering this visiable line
        ppu_render_scanline();
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

        // we finished a frame
        ppu_frame_ready = true;
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
      ppu_update_lyc_flag();
    }

  }
}

void ppu_update_lyc_flag()
{
  if (ppu_registers->ly == ppu_registers->lyc) {
    ppu_registers->stat |= 0b00000100;
  }
  else {
    ppu_registers->stat &= 0b11111011;
  }
  ppu_update_stat_interrupt();
}


void ppu_init()
{
    ppu_line_cycles = 0;
    ppu_registers->ly = 0;
    ppu_lcd_enabled = false;
    ppu_stat_irq_line = false;

    ppu_registers->stat |= 0b10000000;
    ppu_frame_ready = false;
    memory[0xFF40] = 0x91; // LCDC
    memory[0xFF47] = 0xFC; // BGP

    ppu_set_mode(0);
    ppu_update_lyc_flag();
}


