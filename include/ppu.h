#pragma once

#include <stdint.h>

/*

    LCDC - 0xFF40
    bit 7 = LCD & PPU enabled: 0 = Off; 1= On
    bit 6 = Window tile map area: 0 = 0x9800–0x9BFF; 1 = 0x9C00–0x9FFF
    bit 5 = Window enable: 0 = Off; 1 = On
    bit 4 = BG & Window tile data area: 0 = 0x8800–0x97FF; 1 = 0x8000–0x8FFF
    bit 3 = BG tile map area: 0 = 0x9800–0x9BFF; 1 = 0x9C00–0x9FFF
    bit 2 = OBJ size: 0 = 8×8; 1 = 8×16
    bit 1 = OBJ enable: 0 = Off; 1 = On
    bit 0 = BG & Window enable / priority [Different meaning in CGB Mode]: 0 = Off; 1 = On
*/


/*

    STAT - 0xFF41
    bit 7 = unused
    bit 6 = LYC int select (Read/Write): If set, selects the LYC == LY condition for the STAT interrupt.
    bit 5 = Mode 2 int select (Read/Write): If set, selects the Mode 2 condition for the STAT interrupt.
    bit 4 = Mode 1 int select (Read/Write): If set, selects the Mode 1 condition for the STAT interrupt.
    bit 3 = Mode 0 int select (Read/Write): If set, selects the Mode 0 condition for the STAT interrupt.
    bit 2 = LYC == LY (Read-only): Set when LY contains the same value as LYC; it is constantly updated.
    bit 1 and 0 = PPU mode (Read-only): Indicates the PPU’s current status. Reports 0 instead when the PPU is disabled.
    for ppu mode:
    00 -> Mode 0 -> HBlank
    01 -> Mode 1 -> VBlank
    10 -> Mode 2 -> OAM Search
    11 -> Mode 3 -> Pixel transfer
*/


/*

    SCY - 0xFF42 & SCX - 0xFF43
    They define the top left coordinates of the visible 160x144 pixel area 
    within the 256x256 pixel BG map. Values in the range 0-255 may be used.

    The ppu calculates the bottom right coordinates of the viewpoer with these formulas:
    bottom := (SCY + 143) % 256
    right := (SCX + 159) % 256
*/


/*
 
    LY - 0xFF44
    LCD Y coordinate [read-only]
 
    LY indicates the current horizontal line, which might be about to be drawn,
    being drawn, or just been draw.
    LY can hold valyes from 0-153, with the values from 144-153 indicating the vblank period
*/

/*
 
    LYC - 0xFF45
    LY compare
 
    The gameboy constantly comapraed the value of the LYC and the LY registers.
    If they are identical, then the "LYC=LY" flag in the STAT register is set, and if enabled
    a STAT interrupt is requested.
*/


struct gb_ppu_registers
{
    uint8_t lcdc;
    uint8_t stat;
    uint8_t scy;
    uint8_t scx;
    uint8_t ly;
    uint8_t lyc;
};

void ppu_set_mode(uint8_t bits);
void ppu_init();

extern gb_ppu_registers* ppu_registers;

void ppu_advance_clocks(uint8_t cycles);

