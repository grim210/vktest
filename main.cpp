#include <iostream>

#include <SDL2/SDL.h>
#include "global.h"
#include "renderer.h"
#include "timer.h"

void parse_cli(struct Renderer::CreateInfo* ci, int argc, char* argv[]);

int main(int argc, char* argv[])
{
    Renderer* rend = nullptr;

    SDL_Init(SDL_INIT_EVERYTHING);

    struct Renderer::CreateInfo info = {};
    info.width = 1024;
    info.height = 768;
    info.flags = Renderer::RESIZABLE;

    parse_cli(&info, argc, argv);

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

void parse_cli(struct Renderer::CreateInfo* ci, int argc, char* argv[])
{
    //char buff[80] = {};
    const char* ptr = nullptr;

    for (int i = 1; i < argc; i++) {
        ptr = std::strstr(argv[i], "-d");
        if (ptr != nullptr) {
            int value = atoi(ptr + 2);
            ci->dlevel = value;
            std::cerr << "CLI: Debug level " << value << std::endl;
        }

        ptr = std::strstr(argv[i], "--vsync");
        if (ptr != nullptr) {
            ci->flags = static_cast<Renderer::Flags>(
              static_cast<int>(Renderer::VSYNC_ON) |
              static_cast<int>(ci->flags));
            std::cerr << "CLI: VSync toggled." << std::endl;
        }
    }
}

