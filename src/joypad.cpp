#include <cstdint>
#include <stdint.h>
#include <joypad.h>
#include <memory_bus.h>

uint8_t buttons_pressed = 0;
bool select_dpad = false;
bool select_buttons = false;

void joypad_init()
{
  buttons_pressed = 0;
  select_dpad = false;
  select_buttons = false;

}

void joypad_set_button(JoypadButton button, bool pressed)
{
  if (pressed) {
    buttons_pressed |= BIT((uint8_t)button);
  }
  else {
    CLEAR_BIT(buttons_pressed, uint8_t(button));
  }
}

void joypad_write(uint8_t value)
{
  select_dpad = (value & BIT(4)) == 0;
  select_buttons = (value & BIT(5)) == 0;
}

uint8_t joypad_read()
{
  uint8_t result = 0xFF;
  if (select_dpad) {
    result &= ~BIT(4);
    result &= ~(buttons_pressed & 0x0F);
  }
  if (select_buttons) {
    result &= ~BIT(5);
    result &= ~(buttons_pressed >> 4);
  }
  return result;
}
