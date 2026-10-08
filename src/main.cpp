#include <emulator_core.h>
#define SDL_MAIN_HANDLED

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <cart.h>
#include <cpu.h>

SDL_Window* window;
SDL_Renderer* renderer;

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
    return SDL_APP_FAILURE;
  }

  renderer = SDL_CreateRenderer(window, NULL);

  if (!renderer) {
    SDL_Log("Error creating renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  int error = core_init();
  if (error != 0) {
    return error;
  }

  core_run();

  SDL_DestroyWindow(window);
  SDL_DestroyRenderer(renderer);
  SDL_Quit();
  return 0;
}
