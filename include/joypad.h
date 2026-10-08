#pragma once

#include <stdint.h>

enum class JoypadButton : uint8_t {
  Right  = 0,
  Left   = 1,
  Up     = 2,
  Down   = 3,
  A      = 4,
  B      = 5,
  Select = 6,
  Start  = 7
};

void joypad_init();
uint8_t joypad_read();
void joypad_write(uint8_t value);
void joypad_set_button(JoypadButton button, bool pressed);
