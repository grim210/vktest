#include <iostream>
#include <sstream>

#include <SDL2/SDL.h>
#include "global.h"
#include "renderer.h"
#include "timer.h"

void parse_cli(struct Renderer::CreateInfo* ci, int argc, char* argv[]);
void print_help(void);
void print_version(void);

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
        ptr = std::strstr(argv[i], "--help");
        if (ptr != nullptr) {
            print_help();
            std::exit(EXIT_SUCCESS);
        }

        ptr = std::strstr(argv[i], "--version");
        if (ptr != nullptr) {
            print_version();
            std::exit(EXIT_SUCCESS);
        }

        ptr = std::strstr(argv[i], "--fullscreen");
        if (ptr != nullptr) {
            ci->flags = static_cast<Renderer::Flags>(
              static_cast<int>(Renderer::FULLSCREEN) |
              static_cast<int>(ci->flags));
            std::cerr << "CLI: Fullscreen toggled." << std::endl;
        }

        ptr = std::strstr(argv[i], "--debug=");
        if (ptr != nullptr) {
            int value = atoi(ptr + 8);
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

void print_help(void)
{
    std::stringstream out;
    out << "Usage:" << std::endl;
    out << "\tvktest.exe [OPTIONS]" << std::endl << std::endl;
    out << "Options:" << std::endl;
    out << "\t--debug=X\tDebug levels from 0-4, least to most verbose.";
    out << std::endl;
    out << "\t--fullscreen\tFull screen rendering." << std::endl;
    out << "\t--help\t\tPrint this help message." << std::endl;
    out << "\t--version\tPrint version information and exit." << std::endl;
    out << "\t--vsync\t\tTurn on vsync (locked to 60 FPS max framerate.";
    out << std::endl;

    std::cout << out.str();
}

void print_version(void)
{
    std::cout << "VkTest v0.0.1" << std::endl;
}
