#pragma once

#include <stdint.h>

struct gb_ppu_registers
{
    uint8_t lcdc;
    uint8_t stat;
    uint8_t scy;
    uint8_t scx;
    uint8_t ly;
    uint8_t lyc;
};
extern gb_ppu_registers* ppu_registers;