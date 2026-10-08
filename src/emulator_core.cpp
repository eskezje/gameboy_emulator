#include <joypad.h>
#include <cstdio>
#include <memory_bus.h>
#include <cart.h>
#include <cpu.h>
#include <cstdint>
#include <emulator_core.h>
#include <timer.h>
#include <interrupts.h>
#include <ppu.h>
#include <SDL3/SDL_events.h>
#include <frontend.h>
#include <chrono>
#include <thread>

const char *tetris_path = "../roms/Super Mario Land (World) (Rev 1).gb";
uint32_t core_clock_counter = 0;
bool core_quit_requested = false;

int core_init() {

  if (!cart_load(tetris_path)) {
    return -1;
  }

  timer_init();
  ppu_init();
  joypad_init();

  cart_print_info();

  return 0;
}

void core_run() {

  SDL_Event event;
  uint32_t last_event_poll = 0;

  cpu_reset();

  using Clock = std::chrono::steady_clock;
  const auto frame_duration = std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(70224.0 / 4194304.0));
  auto next_frame_time = Clock::now() + frame_duration;

  while (!core_quit_requested) {
    cpu_tick();

    if (core_clock_counter - last_event_poll >= 1024) {
      last_event_poll = core_clock_counter;
      while (SDL_PollEvent(&event)) {
        switch (event.type) {
          case SDL_EVENT_QUIT:
            core_quit_requested = true;
            break;
          case SDL_EVENT_KEY_DOWN:
            if (event.key.scancode == SDL_SCANCODE_Z) {
              joypad_set_button(JoypadButton::A, true);
            }
            else if (event.key.scancode == SDL_SCANCODE_X) {
              joypad_set_button(JoypadButton::B, true);
            }
            else if (event.key.scancode == SDL_SCANCODE_RETURN) {
              joypad_set_button(JoypadButton::Start, true);
            }
            else if (event.key.scancode == SDL_SCANCODE_LSHIFT) {
              joypad_set_button(JoypadButton::Select, true);
            }
            else if (event.key.scancode == SDL_SCANCODE_RSHIFT) {
              joypad_set_button(JoypadButton::Select, true);
            }
            else if (event.key.scancode == SDL_SCANCODE_UP) {
              joypad_set_button(JoypadButton::Up, true);
            }
            else if (event.key.scancode == SDL_SCANCODE_DOWN) {
              joypad_set_button(JoypadButton::Down, true);
            }
            else if (event.key.scancode == SDL_SCANCODE_LEFT) {
              joypad_set_button(JoypadButton::Left, true);
            }
            else if (event.key.scancode == SDL_SCANCODE_RIGHT) {
              joypad_set_button(JoypadButton::Right, true);
            }
            break;


          case SDL_EVENT_KEY_UP:
            if (event.key.scancode == SDL_SCANCODE_Z) {
              joypad_set_button(JoypadButton::A, false);
            }
            else if (event.key.scancode == SDL_SCANCODE_X) {
              joypad_set_button(JoypadButton::B, false);
            }
            else if (event.key.scancode == SDL_SCANCODE_RETURN) {
              joypad_set_button(JoypadButton::Start, false);
            }
            else if (event.key.scancode == SDL_SCANCODE_LSHIFT) {
              joypad_set_button(JoypadButton::Select, false);
            }
            else if (event.key.scancode == SDL_SCANCODE_RSHIFT) {
              joypad_set_button(JoypadButton::Select, false);
            }
            else if (event.key.scancode == SDL_SCANCODE_UP) {
              joypad_set_button(JoypadButton::Up, false);
            }
            else if (event.key.scancode == SDL_SCANCODE_DOWN) {
              joypad_set_button(JoypadButton::Down, false);
            }
            else if (event.key.scancode == SDL_SCANCODE_LEFT) {
              joypad_set_button(JoypadButton::Left, false);
            }
            else if (event.key.scancode == SDL_SCANCODE_RIGHT) {
              joypad_set_button(JoypadButton::Right, false);
            }
            break;
        }
      }
    }
    if (ppu_frame_ready) {
      // then we render it with SDL
      render_frame_from_ppu_framebuffer();
      ppu_frame_ready = false;

      std::this_thread::sleep_until(next_frame_time);
      next_frame_time += frame_duration;

      if (Clock::now() > next_frame_time) {
        next_frame_time = Clock::now() + frame_duration;
      }

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

