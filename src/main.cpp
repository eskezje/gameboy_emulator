#include "ppu.h"
#include <cstdint>
#include <cstdio>
#include <emulator_core.h>
#include <frontend.h>
#include <ios>
#define SDL_MAIN_HANDLED

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <cart.h>
#include <cpu.h>

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* texture = nullptr;

int SDLCALL goodbye_runapp_callback(int argc, char *argv[]);

int main(int argc, char *argv[]) {
  return SDL_RunApp(argc, argv, goodbye_runapp_callback, NULL);
}

int SDLCALL goodbye_runapp_callback(int argc, char *argv[]) {

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Error initializing SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  window = SDL_CreateWindow("Gameboy emulator", 160*4, 144*4, 0);

  if (!window) {
    SDL_Log("Error creating window: %s", SDL_GetError());
    SDL_Quit();
    return SDL_APP_FAILURE;
  }

  renderer = SDL_CreateRenderer(window, NULL);

  if (!renderer) {
    SDL_Log("Error creating renderer: %s", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return SDL_APP_FAILURE;
  }

  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 160, 144);

  if (!texture) {
    SDL_Log("Error creating texture: %s", SDL_GetError());
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return SDL_APP_FAILURE;
  }

  if (!SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST)) {
      SDL_Log("Error setting texture scale mode: %s", SDL_GetError());
  }

  int error = core_init();
  if (error != 0) {
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return error;
  }



  core_run();

  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}



void render_frame_from_ppu_framebuffer()
{
  uint32_t SDL_pixel_array[144][160];
  DmgShade shade;
  uint32_t  color;
  for (uint16_t y = 0; y<144; y++) {
    for (uint16_t x = 0; x <160; x++) {
      shade = (DmgShade)ppu_framebuffer[y][x];
      if (shade == DmgShade::White) {
        color = 255;
      }
      else if (shade == DmgShade::LightGray) {
        color = 170;
      }
      else if (shade == DmgShade::DarkGray) {
        color = 85;
      }
      else if (shade == DmgShade::Black) {
        color = 0;
      }
      else {
        color = 200;
        printf("The color wasnt correct\n");
      }
      SDL_pixel_array[y][x] = (color << 24) | (color << 16) | (color << 8) | 0xFF;
    }
  }

  if (!SDL_UpdateTexture(texture, NULL, SDL_pixel_array, sizeof(uint32_t)*160)) {
    SDL_Log("Error updating texture: %s", SDL_GetError());
  }

  if (!SDL_RenderTexture(renderer, texture, NULL, NULL)) {
    SDL_Log("Error rendering texture: %s", SDL_GetError());
  }

  if (!SDL_RenderPresent(renderer)) {
    SDL_Log("Error presenting render: %s", SDL_GetError());
  }

}
