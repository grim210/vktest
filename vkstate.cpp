#include "vkstate.h"

VkState* VkState::Init(SDL_Window* win)
{
    if (!win) {
        return nullptr;
    }

    VkState* ret = new VkState();
    ret->window = win;
    ret->queue_idx = UINT32_MAX;

    VkResult result = VK_SUCCESS;

    result = ret->create_instance();
    ret->_assert(result, "Failed to create Vulkan instance.  Do you have a "
      "compatible system with up-to-date drivers?");

    result = ret->create_device();
    ret->_assert(result, "Failed to create a rendering device.  Do you have "
      "up-to-date drivers?");

    ret->_info("Successfully created Vulkan instance and device.");

    return ret;
}

void VkState::Release(VkState* state)
{
    vkDestroySurfaceKHR(state->instance, state->swapchain.surface, NULL);
    vkDestroyDevice(state->device, NULL);
    vkDestroyInstance(state->instance, NULL);
    delete(state);
}

VkResult VkState::create_device(void)
{
    VkResult result = VK_SUCCESS;

    /*
     * First, we must get all the physical devices that are compatible with
     * Vulkan on the machine.  This is done by first getting the number of
     * devices with vkEnumeratePhysicalDevices() with no VkPhysicalDevice
     * array (notice the nullptr) to determine the count.  After you allocate
     * enough memory, you call the same function with the VkPhysicalDevice
     * array as the third parameter.
     */
    uint32_t device_count = 0;
    std::vector<VkPhysicalDevice> physical_devices;
    result = vkEnumeratePhysicalDevices(this->instance, &device_count, nullptr);

    physical_devices.resize(device_count);
    result = vkEnumeratePhysicalDevices(this->instance, &device_count,
      physical_devices.data());

    /*
     * Next, we have to create a VkSurfaceKHR, which is part of the WSI
     * extension for Vulkan.  With a valid surface, you are able to query
     * your physical device to determine which device queue can actually
     * present to the surface that's been created from your window.
     */

    /* This SDL call lets you grab the raw platform window handles */
    SDL_SysWMinfo info = {};
    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(this->window, &info) != SDL_TRUE) {
        this->_assert(VK_INCOMPLETE, SDL_GetError());
    }

#ifdef __linux__
    VkXcbSurfaceCreateInfoKHR si = {};
    si.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    si.pNext = nullptr;
    si.flags = 0;
    si.connection = XGetXCBConnection(info.info.x11.display);
    si.window = info.info.x11.window;
    result = vkCreateXcbSurfaceKHR(this->instance, &si, nullptr,
      &this->swapchain.surface);
    this->_assert(result, "vkCreateXcbSurfaceKHR");
#elif _WIN32
    VkWin32SurfaceCreateInfoKHR si = {};
    si.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    si.pNext = nullptr;
    si.flags = 0;
    si.hinstance = GetModuleHandle(NULL);
    si.hwnd = info.info.win.window;
    result = vkCreateWin32SurfaceKHR(this->instance, &si, nullptr,
      &this->swapchain.surface);
    this->_assert(result, "vkCreateWin32SurfaceKHR");
#else
    this->_assert(VK_ERROR_FEATURE_NOT_PRESENT, "Unable to detect platform for "
      "VkSurfaceKHR creation.");
#endif

    /* Identify a physical device that can render
     * and present on the same queue. */
    this->queue_idx = UINT32_MAX;

    for (uint32_t i = 0; i < device_count; i++) {
        uint32_t queue_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i],
          &queue_count, NULL);

        std::vector<VkQueueFamilyProperties> queue_properties;
        queue_properties.resize(queue_count);

        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i],
          &queue_count, queue_properties.data());

        for (uint32_t j = 0; j < queue_count; j++) {
            VkPhysicalDevice target_device = physical_devices[i];
            VkQueueFamilyProperties target_queue = queue_properties[j];

            VkResult tr = VK_SUCCESS;
            if (target_queue.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                VkBool32 support = VK_FALSE;
                tr = vkGetPhysicalDeviceSurfaceSupportKHR(target_device,
                  j, this->swapchain.surface, &support);
                this->_assert(tr, "vkGetPhysicalDeviceSurfaceSupportKHR");

                if (support == VK_TRUE) {
                    this->gpu.device = physical_devices[i];
                    this->queue_idx = j;

                    break;
                }
            }
        }

        if (this->queue_idx != UINT32_MAX) {
            break;
        }
    }

    float queue_priority = 1.0f;

    VkDeviceQueueCreateInfo q_create_info = {};
    q_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    q_create_info.pNext = nullptr;
    q_create_info.flags = 0;
    q_create_info.queueFamilyIndex = this->queue_idx;
    q_create_info.queueCount = 1;
    q_create_info.pQueuePriorities = &queue_priority;

    VkDeviceCreateInfo d_create_info = {};
    d_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    d_create_info.pNext = nullptr;
    d_create_info.flags = 0;
    d_create_info.queueCreateInfoCount = 1;
    d_create_info.pQueueCreateInfos = &q_create_info;
    d_create_info.enabledLayerCount = 0;
    d_create_info.ppEnabledLayerNames = nullptr;
    d_create_info.enabledExtensionCount = 0;
    d_create_info.ppEnabledExtensionNames = nullptr;
    d_create_info.pEnabledFeatures = nullptr;

    result = vkCreateDevice(this->gpu.device, &d_create_info, NULL, 
      &this->device);
    this->_assert(result, "vkCreateDevice");

    return result;
}

VkResult VkState::create_instance(void)
{
    /* in a sane application, you would verify these extensions are present
     * before barreling away creating your instance.  But because I know
     * they exist on my development machine (just run vulkaninfo.. ) I'm
     * going to skip that step to save myself a lot of code. */
    VkApplicationInfo application_info = {};
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    application_info.pApplicationName = "Vulkan Test";
    application_info.engineVersion = VK_MAKE_VERSION(0, 0, 1);
    application_info.pEngineName = "Vulkan Test Engine";
    application_info.apiVersion = VK_MAKE_VERSION(1, 0, 0);

    /*
     * This code is super-nasty.  But I'm not sure of a better way to do it.
     */
#ifdef VKTEST_DEBUG
    int layer_count = 1;
    int extension_count = 3;
    const char* layers[] = { "VK_LAYER_LUNARG_standard_validation" };
    #ifdef __linux__
    const char* extensions[] = { "VK_KHR_surface", "VK_KHR_xcb_surface",
    #elif _WIN32
    const char* extensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface",
    #endif
      "VK_EXT_debug_report" };
#else   /* in 'release' mode */
    int layer_count = 0;
    int extension_count = 2;
    const char* layers[] = { nullptr };
    const char* extensions[] = { "VK_KHR_surface",
    #ifdef __linux__
      "VK_KHR_xcb_surface" };
    #elif _WIN32
      "VK_KHR_win32_surface" };
    #endif
#endif

    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &application_info;
    create_info.enabledLayerCount = layer_count;
    create_info.ppEnabledLayerNames = layers;
    create_info.enabledExtensionCount = extension_count;
    create_info.ppEnabledExtensionNames = extensions;

    return vkCreateInstance(&create_info, NULL, &this->instance);
}

VkResult VkState::init_debug(void)
{
    return VK_INCOMPLETE;
}

