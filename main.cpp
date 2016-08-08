#include <iostream>

#include <SDL2/SDL.h>
#include "global.h"
#include "renderer.h"

int main(int argc, char* argv[])
{
    Renderer* rend = nullptr;

    SDL_Init(SDL_INIT_EVERYTHING);

    SDL_Window* window = SDL_CreateWindow("VkTest Window",
      SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600,
      SDL_WINDOW_RESIZABLE);

    rend = Renderer::Init(window);
    if (!rend) {
        std::cerr << "Failed to initialize Vulkan library." << std::endl;
        exit(-1);
    }

    int framecount = 0;
    uint32_t start = SDL_GetTicks();

    bool done = false;
    while (!done) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                done = true;
            }
            if (ev.type == SDL_WINDOWEVENT) {
                rend->PushEvent(ev.window);
            }
        }

        rend->Update(0.0);
        rend->Render();

        framecount++;
        if (SDL_GetTicks() - start >= 1000) {
            std::cout << "FPS: " << framecount << std::endl;
            framecount = 0;
            start = SDL_GetTicks();
        }
    }

    Renderer::Release(rend);

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

