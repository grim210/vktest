#include <iostream>

#include <SDL2/SDL.h>
#include "vkstate.h"
#include "vkwindow.h"

int main(int argc, char* argv[])
{
    VkState* state = nullptr;
    VkWindow* window = nullptr;

    SDL_Init(SDL_INIT_EVERYTHING);

    struct VkWindow::CreateInfo ci = {};
    ci.width = 800;
    ci.height = 600;
    ci.title = "VkTest";
    ci.flags = VkWindow::VKWINDOW_RESIZABLE;

    window = VkWindow::Init(&ci);
    if (!window) {
        std::cerr << "Failed to create window." << std::endl;
        return -1;
    }

    state = VkState::Init(window->GetHandle());
    if (!state) {
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
                if (ev.window.event == SDL_WINDOWEVENT_RESIZED) {
                    state->RecreateSwapchain();
                }
            }
        }

        state->Render();

        framecount++;
        if (SDL_GetTicks() - start >= 1000) {
            std::cout << "FPS: " << framecount << std::endl;
            framecount = 0;
            start = SDL_GetTicks();
        }
    }

    VkWindow::Release(window);
    VkState::Release(state);

    SDL_Quit();
    return 0;
}

