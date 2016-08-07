#include "vkwindow.h"

VkWindow* VkWindow::Init(struct CreateInfo* info)
{
    VkWindow* win = new VkWindow();

    uint32_t winflags = 0;
    uint16_t width, height;
    std::string title;

    if (info->width == 0 || info->width == UINT16_MAX) {
        width = VKWINDOW_DEFAULT_WIDTH;
    } else {
        width = info->width;
    }

    if (info->height == 0 || info->height == UINT16_MAX) {
        height = VKWINDOW_DEFAULT_HEIGHT;
    } else {
        height = info->height;
    }

    if (info->title.empty()) {
        title = VKWINDOW_DEFAULT_TITLE;
    } else {
        title = info->title;
    }

    if (info->flags & VKWINDOW_FULLSCREEN) {
        winflags |= SDL_WINDOW_FULLSCREEN;
    }

    if (info->flags & VKWINDOW_RESIZABLE) {
        winflags |= SDL_WINDOW_RESIZABLE;
    }

    if (info->flags & VKWINDOW_VERTICAL_SYNC) {
        // This is a Swapchain creation flag.  Coming in time.
    }

    win->handle = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED, static_cast<int>(width),
      static_cast<int>(height), winflags);
    if (!win->handle) {
        delete(win);
        return nullptr;
    }

    return win;
}

void VkWindow::Release(VkWindow* window)
{
    if (window == nullptr) {
        return;
    }

    SDL_DestroyWindow(window->handle);
    delete(window);
}

SDL_Window* VkWindow::GetHandle(void)
{
    return handle;
}
