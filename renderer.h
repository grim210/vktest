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

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define RENDERER_DEFAULT_WIDTH      (800)
#define RENDERER_DEFAULT_HEIGHT     (600)
#define RENDERER_WINDOW_NAME        ("Vulkan Renderer")

#include "global.h"
#include "swapchain.h"

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
    VkSurfaceKHR m_surface;

    std::vector<VkFramebuffer> m_fbuffers;
    VkQueue m_renderqueue;

    Swapchain* m_swapchain;
    VkSemaphore m_swapready;
    VkSemaphore m_swapfinished;

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

    struct Box {
        std::vector<Vertex> vertices;
        std::vector<uint16_t> indices;
        VkBuffer vbuffer;                 // vertex buffer
        VkBuffer ibuffer;                 // index buffer
        VkBuffer ubuffer;                 // uniform buffer
        VkBuffer usbuffer;                // uniform staging buffer
        VkDeviceMemory vbuffermem;
        VkDeviceMemory ibuffermem;
        VkDeviceMemory ubuffermem;
        VkDeviceMemory usbuffermem;
        VkDescriptorSetLayout dslayout;
        VkDescriptorPool dpool;
        VkDescriptorSet dset;
    } m_box;

    struct Texture {
        VkImage image;
        VkImageView view;
        VkDeviceMemory memory;
        VkSampler sampler;
    } m_texture;

    VkImage m_depthimage;
    VkImageView m_depthview;
    VkDeviceMemory m_depthmem;

    VkResult create_depthresources(void);
    VkResult create_descriptorset_layout(void);
    VkResult create_descriptorpool(void);
    VkResult create_descriptorset(void);
    VkResult create_vertexbuffer(void);
    VkResult create_indexbuffer(void);
    VkResult create_sampler(void);
    VkResult create_texture(void);
    VkResult create_textureimageview(void);
    VkResult create_uniformbuffer(void);

    VkResult begin_single_time_commands(VkCommandBuffer* cbuff);
    VkResult end_single_time_commands(VkCommandBuffer cbuff);

    VkResult copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
    VkResult copy_image(VkImage src, VkImage dst, uint32_t w, uint32_t h);
    VkResult create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
      VkMemoryPropertyFlags properties, VkBuffer* buffer,
      VkDeviceMemory* buffer_memory);
    VkResult create_image(uint32_t w, uint32_t h, VkFormat fmt,
      VkImageTiling tiling, VkImageUsageFlags usage,
      VkMemoryPropertyFlags properties, VkImage* img, VkDeviceMemory* mem);
    VkResult create_imageview(VkImage image, VkFormat format,
      VkImageAspectFlags aflags, VkImageView* view);
    VkResult find_depth_format(VkFormat* format);
    uint32_t find_memory_type(uint32_t filter, VkMemoryPropertyFlags flags);
    VkResult find_supported_format(VkPhysicalDevice gpu,
      std::vector<VkFormat> candidates, VkImageTiling tiling,
      VkFormatFeatureFlags features, VkFormat *out);
    VkResult transition_image_layout(VkImage img, VkImageLayout old,
      VkImageLayout _new);

    /* Only initialization functions. Look in renderer_init.cpp */
    VkResult create_cmdpool(void);
    VkResult create_cmdbuffers(void);
    VkResult create_device(void);
    VkResult create_framebuffers(void);
    VkResult create_instance(void);
    VkResult create_pipeline(void);
    VkResult create_renderpass(void);
    VkResult create_surface(void);
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
