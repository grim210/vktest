#ifndef VKATTEMPT_RENDERER_H
#define VKATTEMPT_RENDERER_H

#include <cstring>
#include <array>
#include <fstream>
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

#define RENDERER_DEFAULT_WIDTH      (800)
#define RENDERER_DEFAULT_HEIGHT     (600)
#define RENDERER_WINDOW_NAME        ("Vulkan Renderer")

#include "global.h"

class Renderer {
public:
    enum Flags {
        NONE        = 0x00,
        FULLSCREEN  = 0x01,
        RESIZABLE   = 0x02,
        VSYNC_ON    = 0x04,
        FPS_ON      = 0x08
    };

    struct CreateInfo {
        uint16_t width, height;
        Flags flags;
        int dlevel;
    };

    /* static initializers so I can have a bit more control */
    static Renderer* Init(CreateInfo* info);
    static void Release(Renderer* renderer);

    /* Typical render-loop type stuff. */
    void PushEvent(SDL_WindowEvent event);
    void RecreateSwapchain(void);
    void Render(void);
    void Update(double elapsed);

private:
    SDL_Window* m_window;
    std::queue<SDL_WindowEvent> m_events;
    CreateInfo m_cinfo;
    struct FPSInfo {
        int framecount;
        double last;
    } m_fpsinfo;

    VkInstance m_instance;
    VkDevice m_device;

    std::vector<VkFramebuffer> m_fbuffers;
    VkQueue m_renderqueue;

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
    } m_swapchain;

    struct PhysicalDevice {
        uint32_t queue_idx;
        VkPhysicalDevice device;
        VkPhysicalDeviceFeatures features;
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceMemoryProperties memory_properties;
        VkQueueFamilyProperties queue_properties;
    } m_gpu;

    struct GraphicsPipline {
        VkRenderPass renderpass;
        VkPipeline gpipeline;
        VkPipelineLayout layout;
        VkShaderModule vshadermodule;
        VkShaderModule fshadermodule;
    } m_pipeline;

    VkCommandPool m_cmdpool;
    std::vector<VkCommandBuffer> m_cmdbuffers;

    std::vector<Vertex> m_vertices;
    std::vector<uint16_t> m_indices;
    VkBuffer m_vbuffer;
    VkBuffer m_ibuffer;
    VkDeviceMemory m_vbuffermem;
    VkDeviceMemory m_ibuffermem;

    /* Private helper functions. */
    VkResult create_vertexbuffer(void);
    VkResult create_indexbuffer(void);
    VkResult copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
    VkResult create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
      VkMemoryPropertyFlags properties, VkBuffer* buffer,
      VkDeviceMemory* buffer_memory);
    uint32_t find_memory_type(uint32_t filter, VkMemoryPropertyFlags flags);

    /* Only initialization functions. Look in renderer_init.cpp */
    VkResult create_cmdpool(void);
    VkResult create_cmdbuffers(void);
    VkResult create_device(void);
    VkResult create_framebuffers(void);
    VkResult create_instance(void);
    VkResult create_pipeline(void);
    VkResult create_renderpass(void);
    VkResult create_surface(void);
    VkResult create_swapchain(void);
    VkResult create_synchronizers(void);
    SDL_Window* create_window(void);

    /* Vulkan object releasing functions.  Look in renderer_release.cpp */
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
    VkDebugReportCallbackEXT m_callback;
    std::fstream m_log;
public:
    std::fstream* _get_log_file(void);
#endif
};

#endif  /* VKTEST_RENDERER_H */
