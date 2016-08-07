#ifndef VKWINDOW_H
#define VKWINDOW_H

#define VKWINDOW_DEFAULT_WIDTH      (800)
#define VKWINDOW_DEFAULT_HEIGHT     (600)
#define VKWINDOW_DEFAULT_TITLE      ("VkWindow Default Title")

#include <string>
#include <vector>

#ifdef _WIN32
  #define VK_USE_PLATFORM_WIN32_KHR
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#elif __linux__
  #define VK_USE_PLATFORM_XCB_KHR
  #include <X11/Xlib-xcb.h>
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

class VkWindow {
public:
    enum Flags {
        VKWINDOW_NONE           = 0x00,
        VKWINDOW_FULLSCREEN     = 0x01,
        VKWINDOW_RESIZABLE      = 0x02,
        VKWINDOW_VERTICAL_SYNC  = 0x04
    };

    struct CreateInfo {
        std::string title;
        uint16_t width, height;
        Flags flags;
    };

    static VkWindow* Init(struct CreateInfo* info);
    static void Release(VkWindow* window);
    SDL_Window* GetHandle(void);
private:
    SDL_Window* handle;
};

#endif
