#include "vkstate.h"

VkState* VkState::Init(SDL_Window* win)
{
    if (!win) {
        return nullptr;
    }

    VkState* ret = new VkState();
    ret->window = win;
    ret->gpu.qidx = UINT32_MAX;

    VkResult result = VK_SUCCESS;

    result = ret->create_instance();
    ret->_assert(result, "Failed to create Vulkan instance.  Do you have a "
      "compatible system with up-to-date drivers?");

    result = ret->init_debug();
    ret->_assert(result, "Failed to initialize the debug extension.");

    result = ret->create_device();
    ret->_assert(result, "Failed to create a rendering device.  Do you have "
      "up-to-date drivers?");

    result = ret->create_buffers();
    ret->_assert(result, "Unable to create buffers from the Vulkan device "
      "queues.");

    result = ret->create_swapchain();
    ret->_assert(result, "Unable to create Swapchain.");

    ret->_info("Successfully created Vulkan instance.");

    return ret;
}

void VkState::Release(VkState* state)
{
    vkDestroySwapchainKHR(state->device, state->swapchain.chain, nullptr);
    vkFreeCommandBuffers(state->device, state->buffers.pool, 1,
      &state->buffers.secondary);
    vkFreeCommandBuffers(state->device, state->buffers.pool, 1,
      &state->buffers.primary);
    vkDestroyCommandPool(state->device, state->buffers.pool, nullptr);
    vkDestroyDevice(state->device, nullptr);
    vkDestroySurfaceKHR(state->instance, state->swapchain.surface, nullptr);
    state->release_debug();
    vkDestroyInstance(state->instance, nullptr);
    delete(state);
}

VkResult VkState::create_buffers(void)
{
    VkResult result = VK_SUCCESS;

    VkCommandPoolCreateInfo cpci = {};
    cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cpci.pNext = nullptr;
    cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cpci.queueFamilyIndex = this->gpu.qidx;

    result = vkCreateCommandPool(this->device, &cpci, nullptr,
      &this->buffers.pool);
    this->_assert(result, "vkCreateCommandPool");

    VkCommandBufferAllocateInfo cbai = {};
    cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.pNext = nullptr;
    cbai.commandPool = this->buffers.pool;
    cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = 1;

    result = vkAllocateCommandBuffers(this->device, &cbai,
      &this->buffers.primary);
    this->_assert(result, "vkAllocateCommandBuffers primary");

    cbai.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    result = vkAllocateCommandBuffers(this->device, &cbai,
      &this->buffers.secondary);
    this->_assert(result, "vkAllocateCommandBuffers secondary");

    return VK_SUCCESS;
}

VkResult VkState::create_device(void)
{
    VkResult result = VK_SUCCESS;

    /*
     * First, we have to create a VkSurfaceKHR, which is part of the WSI
     * extension for Vulkan.  With a valid surface, you are able to query
     * your physical devices to determine which device queue can actually
     * present to the surface that's been created from your window.
     */

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

    /*
     * Next, we must get all the physical devices that are compatible with
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
     * Then, it's necessary to cycle through all your physical devices to
     * determine which has a queue capable of both rendering and presenting.
     * I imagine that it would be possible to come across a device that can
     * do one or the other, so I'm not considering that case.
     */
    this->gpu.qidx = UINT32_MAX;
    for (uint32_t i = 0; i < device_count; i++) {
        uint32_t queue_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i],
          &queue_count, NULL);

        std::vector<VkQueueFamilyProperties> queue_properties;
        queue_properties.resize(queue_count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i],
          &queue_count, queue_properties.data());

        /* Cycling through all the returned queues from the device in this
         * loop's iteration.
         */
        for (uint32_t j = 0; j < queue_count; j++) {
            VkPhysicalDevice target_device = physical_devices[i];
            VkQueueFamilyProperties target_queue = queue_properties[j];

            /*
             * If _this_ queue on _this_ device supports rendering, next
             * determine if that queue can also present.  If so, we have
             * a winner.
             */
            if (target_queue.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                VkBool32 support = VK_FALSE;
                VkResult tr = vkGetPhysicalDeviceSurfaceSupportKHR(
                  target_device, j, this->swapchain.surface, &support);
                this->_assert(tr, "vkGetPhysicalDeviceSurfaceSupportKHR");

                /* Notice that we're saving both the physical device and the
                 * queue index that supports presenting and rendering. */
                if (support == VK_TRUE) {
                    this->gpu.device = physical_devices[i];
                    this->gpu.qidx = j;
                    break;
                }
            }
        }

        /* Used to determine if a suitable device and queue index has already
         * been found to break out of the outer VkPhysicalDevice loop. */
        if (this->gpu.qidx != UINT32_MAX) {
            break;
        }
    }

    /*
    * Now that we know which GPU we're going to be using to render with,
    * we capture some additional information that will be useful for us
    * in the future.
    */
    vkGetPhysicalDeviceFeatures(this->gpu.device, &this->gpu.features);
    vkGetPhysicalDeviceProperties(this->gpu.device, &this->gpu.properties);
    vkGetPhysicalDeviceMemoryProperties(this->gpu.device,
      &this->gpu.memory_properties);

    /*
    * In this section of code, we actually create the logical VkDevice that
    * will be used for rendering and presenting.  It is worth noting that
    * all the queue business above is necessary before we can create our
    * device.  When we create our device, we also create a queue.  Notice
    * in the VkDeviceQueueCreateInfo structure below, I'm only asking for
    * a single queue, however.
    */
    float queue_priority[] = { 1.0f };  // 0.0 to 1.0; low to high priority
    VkDeviceQueueCreateInfo q_create_info = {};
    q_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    q_create_info.pNext = nullptr;
    q_create_info.flags = 0;
    q_create_info.queueFamilyIndex = this->gpu.qidx;
    q_create_info.queueCount = 1;
    q_create_info.pQueuePriorities = queue_priority;

    const char* swap_extension[] = { "VK_KHR_swapchain" };

#ifdef VKTEST_DEBUG
    uint32_t layer_count = 1;
    const char* dev_layers[] = { "VK_LAYER_LUNARG_standard_validation" };
#else
    uint32_t layer_count = 0;
    const char* dev_layers[] = { nullptr };
#endif

    this->gpu.features.shaderClipDistance = VK_TRUE;

    VkDeviceCreateInfo d_create_info = {};
    d_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    d_create_info.pNext = nullptr;
    d_create_info.flags = 0;
    d_create_info.queueCreateInfoCount = 1;
    d_create_info.pQueueCreateInfos = &q_create_info;
    d_create_info.enabledLayerCount = layer_count;
    d_create_info.ppEnabledLayerNames = dev_layers;
    d_create_info.enabledExtensionCount = 1;
    d_create_info.ppEnabledExtensionNames = swap_extension;
    d_create_info.pEnabledFeatures = &this->gpu.features;

    result = vkCreateDevice(this->gpu.device, &d_create_info, NULL, 
      &this->device);

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
     * This code is super nasty.  But I'm not sure of a better way to do it.
     */
#ifdef VKTEST_DEBUG
    int layer_count = 1;
    int extension_count = 3;
    const char* layers[] = { "VK_LAYER_LUNARG_standard_validation" };
    const char* extensions[] = { "VK_KHR_surface",
    #ifdef __linux__
      "VK_KHR_xcb_surface",
    #elif _WIN32
      "VK_KHR_win32_surface",
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

VkResult VkState::create_swapchain(void)
{
    /*
    * This section of code is dedicated to creating the illustrious swapchain.
    * Creating the swapchain is a very simple process, but filling the structure
    * used to create the swapchain is a different best.  Take a peek at all the
    * fields in that structure and you'll see that a lot of information is
    * required from the application to make create the swapchain.
    */
    VkResult result = VK_SUCCESS;
    uint32_t count = 0;

    /*
    * Here we figure out what surface formats the physical device supports.
    */
    count = 0;
    std::vector<VkSurfaceFormatKHR> surface_formats;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(this->gpu.device,
      this->swapchain.surface, &count, nullptr);
    this->_assert(result, "vkGetPhysicalDeviceSurfaceFormatsKHR");

    surface_formats.resize(count);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(this->gpu.device,
      this->swapchain.surface, &count, surface_formats.data());
    this->_assert(result, "vkGetPhysicalDeviceSurfaceFormatsKHR");

    /* There appears to be only one color space, but I'll grab it from
     * the surface formats just to be safe. */
    this->swapchain.colorspace = surface_formats[0].colorSpace;
    if (count == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED) {
        this->swapchain.format = VK_FORMAT_B8G8R8_UNORM;
    } else {
        this->swapchain.format = surface_formats[0].format;
    }

    /*
    * Next, we grab our surface capabilities.  This will allow us to discern
    * how many buffers that the machine is capable of presenting.  I'm needing
    * a double-buffered swapchain, but I'm going through the checks to ensure
    * the device actually supports that.  Triple-buffering is also pretty
    * pretty common, but I'd rather not add the complexity.
    */
    VkSurfaceCapabilitiesKHR sc = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this->gpu.device,
      this->swapchain.surface, &sc);

    /* Translation: if the surface's minimum capability is greater than 2, use
    * that.  But if the surface cannot support two, use the max supported. */
    uint32_t image_count = 2;
    if (image_count < sc.minImageCount) {
        image_count = sc.minImageCount;
    } else if (sc.maxImageCount != 0 && image_count > sc.maxImageCount) {
        image_count = sc.maxImageCount;
    }

    /*
    * We get the latest and greatest window resolution from SDL. After that,
    * we hang compare that the what the device is telling us about the
    * actual surface.
    */
    SDL_GetWindowSize(this->window, &this->width, &this->height);

    /* Compare that to the surface capabilities structure. */
    if (sc.currentExtent.width == UINT32_MAX) {
        this->swapchain.resolution.width = static_cast<uint32_t>(this->width);
        this->swapchain.resolution.height = static_cast<uint32_t>(this->height);
    } else {
        this->swapchain.resolution.width = sc.currentExtent.width;
        this->swapchain.resolution.height = sc.currentExtent.height;
    }

    VkSurfaceTransformFlagBitsKHR pre_transform = sc.currentTransform;
    if (sc.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }

    count = 0;
    std::vector<VkPresentModeKHR> present_modes;
    vkGetPhysicalDeviceSurfacePresentModesKHR(this->gpu.device,
      this->swapchain.surface, &count, nullptr);

    present_modes.resize(count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(this->gpu.device,
      this->swapchain.surface, &count, present_modes.data());

    this->swapchain.mode = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32_t i = 0; i < present_modes.size(); i++) {
        if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            this->swapchain.mode = present_modes[i];
            break;
        }
    }

    VkSwapchainCreateInfoKHR sci = {};
    sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sci.pNext = VK_NULL_HANDLE;
    sci.flags = 0;
    sci.surface = swapchain.surface;
    sci.minImageCount = image_count;        // should I _require_ two?
    sci.imageFormat = this->swapchain.format;
    sci.imageColorSpace = this->swapchain.colorspace;
    sci.imageExtent = this->swapchain.resolution;
    sci.imageArrayLayers = 1;   // 1, since we're not doing stereoscopic 3D :|
    sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sci.queueFamilyIndexCount = 0;              // because .imageSharingMode
    sci.pQueueFamilyIndices = VK_NULL_HANDLE;   // because .imageSharingMode
    sci.preTransform = pre_transform;
    sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sci.presentMode = this->swapchain.mode;
    sci.clipped = VK_TRUE;
    sci.oldSwapchain = VK_NULL_HANDLE;

    fprintf(stderr, "Here?\n");

    result = vkCreateSwapchainKHR(this->device, &sci, NULL,
      &this->swapchain.chain);
    this->_assert(result, "vkCreateSwapchainKHR");

    return result;
}


