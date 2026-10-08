#include <memory_bus.h>
#include <cart.h>
#include <cpu.h>
#include <cstdint>
#include <emulator_core.h>
#include <timer.h>
#include <interrupts.h>
#include <ppu.h>

const char *tetris_path = "../roms/cpu_instrs/individual/02-interrupts.gb";
uint32_t core_clock_counter = 0;
bool core_quit_requested = false;

int core_init() {

  if (!cart_load(tetris_path)) {
    return -1;
  }

  timer_init();
  ppu_init();

  cart_print_info();

  return 0;
}

void core_run() {

  cpu_reset();

  while (!core_quit_requested) {
    cpu_tick();
    // check if ppu has completed a frame
    if (ppu_frame_ready) {
      // then we render it with SDL
      ppu_frame_ready = false;
    }
  }
}

void core_shutdown() {}



void core_advance_cpu_clocks(uint8_t clocks) {
  timer_advance_clocks(clocks);
  ppu_advance_clocks(clocks);

  core_clock_counter += clocks;

  if (interrupt_enable_ime_delay > 0) {
    for (uint8_t c = 0; c < clocks; c++)
    {
      interrupt_enable_ime_delay--;
      if (interrupt_enable_ime_delay == 0) {
        interrupt_master_enable = true;
        break;
      }
    }
  }
}

