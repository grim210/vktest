#ifndef VKATTEMPT_VKSTATE_H
#define VKATTEMPT_VKSTATE_H

#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#ifdef __linux__
  #define VK_USE_PLATFORM_XCB_KHR
  #include <X11/Xlib-xcb.h>
#elif _WIN32
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

    void Render(void);
private:
    SDL_Window* window;
    int width, height;
    bool first_render;

    VkInstance instance;
    VkDevice device;

    std::vector<VkQueue> queues;

    struct Swapchain {
        VkExtent2D resolution;
        VkFence fence;
        VkFormat format;
        VkColorSpaceKHR colorspace;
        VkPresentModeKHR mode;
        VkSurfaceKHR surface;
        VkSwapchainKHR chain;
    } swapchain;

    VkSemaphore semaphore;

    /*
    * The VkPhysicalDevice, after you create your VkDevice, is pretty much
    * useless to my knowledge.  That may change as I learn more about the spec.
    */
    struct PhysicalDevice {
        uint32_t qidx;
        VkPhysicalDevice device;
        VkPhysicalDeviceFeatures features;
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceMemoryProperties memory_properties;
        VkQueueFamilyProperties queue_properties;
    } gpu;

    struct BufferPool {
        VkCommandPool pool;
        std::vector<VkCommandBuffer> buffers;
    } buffpool;

    VkResult create_buffers(void);
    VkResult create_device(void);
    VkResult create_instance(void);
    VkResult create_swapchain(void);

    /*
    * While these are debug functions, I was trying to avoid #ifdef guards in
    * the initialization code for clarity.  However, if you look at the
    * function definitions, in #VKTEST_DEBUG mode, they're simply no-ops.
    */
    VkResult init_debug(void);
    VkResult release_debug(void);

    void _assert(VkResult condition, std::string message);
    void _info(std::string message);

#ifdef VKTEST_DEBUG
    VkDebugReportCallbackEXT callback;
    std::fstream log;
public:
    std::fstream* _get_log_file(void);
#endif
};

#endif  /* VKTEST_VKSTATE_H */
