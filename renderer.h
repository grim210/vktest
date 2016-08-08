#ifndef VKATTEMPT_RENDERER_H
#define VKATTEMPT_RENDERER_H

#include <cstring>
#include <array>
#include <fstream>
#include <iostream>
#include <queue>
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
#include <vulkan/vulkan.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <glm/glm.hpp>

#include "global.h"

class Renderer {
public:
    static Renderer* Init(SDL_Window* win);
    static void Release(Renderer* renderer);
    void PushEvent(SDL_WindowEvent event);
    void RecreateSwapchain(void);
    void Render(void);
    void Update(double elapsed);

private:
    SDL_Window* window;
    std::queue<SDL_WindowEvent> events;
    bool firstpass;

    VkInstance instance;
    VkDevice device;
    VkFence fence;

    std::vector<VkFramebuffer> fbuffers;
    std::vector<VkQueue> queues;

    struct Swapchain {
        VkSemaphore semready;
        VkSemaphore semfinished;
        VkExtent2D extent;
        VkFormat format;
        VkColorSpaceKHR colorspace;
        VkPresentModeKHR mode;
        VkSurfaceKHR surface;
        VkSwapchainKHR chain;
        std::vector<VkImage> images;
        std::vector<VkImageView> views;
    } swapchain;

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

    struct GraphicsPipline {
        VkRenderPass renderpass;
        VkPipeline gpipeline;
        VkPipelineLayout layout;
        VkShaderModule vshadermodule;
        VkShaderModule fshadermodule;
    } pipeline;

    VkCommandPool cmdpool;
    std::vector<VkCommandBuffer> cbuffers;

    std::vector<Vertex> vertices;
    VkBuffer vbuffer;
    VkDeviceMemory vbuffermem;

    VkResult create_buffers(void);
    VkResult create_device(void);
    VkResult create_framebuffers(void);
    VkResult create_instance(void);
    VkResult create_pipeline(void);
    VkResult create_renderpass(void);
    VkResult create_swapchain(void);

    uint32_t find_memory_type(uint32_t filter, VkMemoryPropertyFlags flags);

    VkResult release_device_objects(void);
    VkResult release_instance_objects(void);
    VkResult release_render_objects(void);
    VkResult release_sync_objects(void);

    /*
    * While these are debug functions, I was trying to avoid #ifdef guards in
    * the initialization code for clarity.  However, if you look at the
    * function definitions, in #VKTEST_DEBUG mode, they're simply no-ops.
    */
    VkResult init_debug(void);
    VkResult release_debug(void);

#ifdef VKTEST_DEBUG
    VkDebugReportCallbackEXT callback;
    std::fstream log;
public:
    std::fstream* _get_log_file(void);
#endif
};

#endif  /* VKTEST_RENDERER_H */
