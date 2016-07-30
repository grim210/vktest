#include <iostream>

#include <SDL2/SDL.h>
#include "vkstate.h"

#define WINDOW_WIDTH        (1024)
#define WINDOW_HEIGHT       (768)

int main(int argc, char* argv[])
{
    VkState* state = nullptr;
    SDL_Init(SDL_INIT_EVERYTHING);

    SDL_Window* win = SDL_CreateWindow("Vulkan Test",
      SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
      WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (win == NULL) {
        std::cerr << "Failed to create window." << std::endl;
        exit(-1);
    }

    state = VkState::Init(win);
    if (!state) {
        std::cerr << "Failed to initialize Vulkan library." << std::endl;
        exit(-1);
    }

    VkState::Release(state);

    SDL_Quit();
    return 0;
}

