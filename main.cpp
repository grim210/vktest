#include <iostream>

#include <SDL2/SDL.h>
#include "global.h"
#include "renderer.h"
#include "timer.h"

int main(int argc, char* argv[])
{
    Renderer* rend = nullptr;

    SDL_Init(SDL_INIT_EVERYTHING);

    struct Renderer::CreateInfo info = {};
    info.width = 1024;
    info.height = 768;
    info.flags = Renderer::RESIZABLE;

    rend = Renderer::Init(&info);
    if (!rend) {
        std::cerr << "Failed to initialize Vulkan library." << std::endl;
        exit(-1);
    }

    Timer t;
    double fstart = t.Elapsed();
    int framecount = 0;

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

        double current = t.Elapsed();

        rend->Update(current);
        rend->Render();

        framecount++;
        if (t.Elapsed() - fstart >= 1.0) {
            std::cout << "FPS: " << framecount << std::endl;
            framecount = 0;
            fstart = t.Elapsed();
        }
    }

    Renderer::Release(rend);
    SDL_Quit();
    return 0;
}

