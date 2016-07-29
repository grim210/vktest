#ifndef VKATTEMPT_VKSTATE_H
#define VKATTEMPT_VKSTATE_H

#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#ifdef __linux__
  #define VK_USE_PLATFORM_XCB_KHR
  #include <X11/Xlib-xcb.h>
#elif
  #define VK_USE_PLATFORM_WIN32_KHR
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <vulkan/vulkan.h>

class VkState {
public:
    static VkState* Init(SDL_Window* win);
    static void Release(VkState* state);
private:
    SDL_Window* window;

    VkInstance instance;
    VkDevice device;

    /*
     * This is a very important variable.  It's used to create queues for
     * the logical device created by the instance.
     */
    uint32_t queue_idx;

    struct Swapchain {
        VkSurfaceKHR surface;
    } swapchain;

    struct PhysicalDevice {
        VkPhysicalDevice device;
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceMemoryProperties memory_properties;
    } gpu;

    VkDebugReportCallbackEXT callback;

    VkResult create_device(void);
    VkResult create_instance(void);
    VkResult init_debug(void);

    void _assert(VkResult condition, std::string message);
    void _info(std::string message);
};

#endif  /* VKATTEMPT_VKSTATE_H */
