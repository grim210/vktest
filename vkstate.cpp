#include "vkstate.h"

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindDesc(void)
    {
        VkVertexInputBindingDescription desc = {};
        desc.binding = 0;
        desc.stride = sizeof(Vertex);
        desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return desc;
    }

    static std::array<VkVertexInputAttributeDescription, 2> getAttrDesc(void)
    {
        std::array<VkVertexInputAttributeDescription, 2> descs = {};

        /* must be the position attribute description. */
        descs[0].binding = 0;
        descs[0].location = 0;
        descs[0].format = VK_FORMAT_R32G32_SFLOAT;
        descs[0].offset = offsetof(Vertex, pos);

        /* and the color attribute descritpion */
        descs[1].binding = 0;
        descs[1].location = 1;
        descs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        descs[1].offset = offsetof(Vertex, color);

        return descs;
    }
};

const std::vector<Vertex> vertices = {
    {{ 0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{ 0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}}
};

static std::vector<char> _read_file(VkState* state, std::string path);

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

    result = ret->create_swapchain();
    ret->_assert(result, "Unable to create Swapchain.");

    result = ret->create_renderpass();
    ret->_assert(result, "Unable to create Renderpass object.");

    result = ret->create_pipeline();
    ret->_assert(result, "Unable to create graphics pipeline.");

    result = ret->create_framebuffers();
    ret->_assert(result, "Unable to create framebuffer objects.");

    result = ret->create_buffers();
    ret->_assert(result, "Unable to create command buffers.");

    VkFenceCreateInfo fci = {};
    fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fci.pNext = nullptr;
    fci.flags = 0;
    result = vkCreateFence(ret->device, &fci, nullptr, &ret->fence);
    ret->_assert(result, "Unable to create fence.");

    ret->firstpass = true;

    return ret;
}

void VkState::Release(VkState* state)
{
    vkDeviceWaitIdle(state->device);
    vkWaitForFences(state->device, 1, &state->fence, VK_TRUE, 1000000000);

    state->release_render_objects();
    state->release_sync_objects();
    state->release_device_objects();
    state->release_instance_objects();

    delete(state);
}

void VkState::RecreateSwapchain(void)
{
    vkDeviceWaitIdle(this->device);

    this->release_render_objects();

    this->create_swapchain();
    this->create_renderpass();
    this->create_pipeline();
    this->create_framebuffers();
    this->create_buffers();
}

void VkState::Render(void)
{
    VkResult result = VK_SUCCESS;

    result = vkWaitForFences(this->device, 1, &this->fence,
      VK_TRUE, 100000000);
    if (result) {
        if (this->firstpass) {
            this->firstpass = false;
        } else {
            this->_info("Framerate is _really_ low..");
        }
    }

    vkResetFences(this->device, 1, &this->fence);

    uint32_t idx = 0;
    vkAcquireNextImageKHR(this->device, this->swapchain.chain, UINT64_MAX,
      this->swapchain.semready, VK_NULL_HANDLE, &idx);

    VkSemaphore waitsems[] = { this->swapchain.semready };
    VkSemaphore sigsems[] = { this->swapchain.semfinished };
    VkPipelineStageFlags waitstages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo si = {};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.waitSemaphoreCount = 1;
    si.pWaitSemaphores = waitsems;
    si.pWaitDstStageMask = waitstages;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &this->cbuffers[idx];
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores = sigsems;

    result = vkQueueSubmit(this->queues[0], 1, &si, this->fence);
    this->_assert(result, "vkQueueSubmit");

    VkSwapchainKHR swapchains[] = { this->swapchain.chain };

    VkPresentInfoKHR pi = {};
    pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores = sigsems;
    pi.swapchainCount = 1;
    pi.pSwapchains = swapchains;
    pi.pImageIndices = &idx;

    vkQueuePresentKHR(this->queues[0], &pi);
}

VkResult VkState::create_buffers(void)
{
    VkResult result = VK_SUCCESS;

    /* Command pool creation */
    VkCommandPoolCreateInfo cpci = {};
    cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cpci.pNext = nullptr;
    cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cpci.queueFamilyIndex = this->gpu.qidx;
    result = vkCreateCommandPool(this->device, &cpci, nullptr,
      &this->cmdpool);
    this->_assert(result, "vkCreateCommandPool");

    /* Vertex buffer creation */
    VkBufferCreateInfo bci = {};
    bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bci.flags = 0;
    bci.size = (sizeof(vertices[0]) * vertices.size());
    bci.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    result = vkCreateBuffer(this->device, &bci, nullptr, &this->vbuffer);
    this->_assert(result, "vkCreateBuffer: vertex buffer");

    /*
    * After the buffer is created, we must allocate memory for it.  Determine
    * the size, the required type of memory and then allocate.  After
    * allocation it must be bound.  Then it must be mapped so that we can
    * actually fill our buffer with vertex data.
    */
    VkMemoryRequirements memreq;
    vkGetBufferMemoryRequirements(this->device, vbuffer, &memreq);

    VkMemoryAllocateInfo mai = {};
    mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mai.allocationSize = memreq.size;
    mai.memoryTypeIndex = this->find_memory_type(memreq.memoryTypeBits,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    result = vkAllocateMemory(this->device, &mai, nullptr, &this->vbuffermem);
    this->_assert(result, "vkAllocateMemory: vertex buffer memory.");

    vkBindBufferMemory(this->device, this->vbuffer, this->vbuffermem, 0);

    /* bci.size is from when we created the original buffer...i had to look. */
    void* data;
    vkMapMemory(this->device, this->vbuffermem, 0, bci.size, 0, &data);
    std::memcpy(data, vertices.data(), (size_t)bci.size);
    vkUnmapMemory(this->device, this->vbuffermem);

    /* Command buffer creation */
    VkCommandBufferAllocateInfo cbai = {};
    cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.pNext = nullptr;
    cbai.commandPool = this->cmdpool;
    cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = this->fbuffers.size();
    this->cbuffers.resize(this->fbuffers.size());
    result = vkAllocateCommandBuffers(this->device, &cbai,
      this->cbuffers.data());
    this->_assert(result, "vkAllocateCommandBuffers");

    /* I'm not sure what this is...will figure out. */
    for (uint32_t i = 0; i < this->cbuffers.size(); i++) {
        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        vkBeginCommandBuffer(this->cbuffers[i], &begin_info);

        VkRenderPassBeginInfo rpi = {};
        rpi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpi.renderPass = this->pipeline.renderpass;
        rpi.framebuffer = this->fbuffers[i];
        rpi.renderArea.offset = { 0, 0 };
        rpi.renderArea.extent = this->swapchain.extent;

        VkClearValue clear_color = { 0.2f, 0.2f, 0.2f, 1.0f };
        rpi.clearValueCount = 1;
        rpi.pClearValues = &clear_color;

        vkCmdBeginRenderPass(this->cbuffers[i], &rpi,
          VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(this->cbuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
          this->pipeline.gpipeline);

        VkBuffer vbuffs[] = {this->vbuffer};
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(this->cbuffers[i], 0, 1, vbuffs, offsets);

        vkCmdDraw(this->cbuffers[i], vertices.size(), 1, 0, 0);

        vkCmdEndRenderPass(this->cbuffers[i]);

        result = vkEndCommandBuffer(this->cbuffers[i]);
        this->_assert(result, "vkEndCommandBuffer");
    }

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
                    this->gpu.queue_properties = target_queue;
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
    * device.  When we create our device, we also create a queue.
    *
    * The first block of code we set our queue priorities.  I'm taking the
    * maximum number of queues that the device supports and assigning them
    * a priority in this for loop.  The queues are sorted by highest to
    * lowest priority on the device.
    *
    * The next block, I'm simply putting that information in the QueueCreate
    * structure.  Followed by some messy #ifdef guards for debug mode layers.
    * Then the actual device creation code.  And upon successful completion
    * you have a device capable of sending draw commands to...eventually.
    */
    std::vector<float> queue_priorities;
    queue_priorities.resize(gpu.queue_properties.queueCount);
    for (uint32_t i = 1; i <= queue_priorities.size(); i++) {
        queue_priorities[i-1] = 1.0f / i;
    }


    VkDeviceQueueCreateInfo q_create_info = {};
    q_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    q_create_info.pNext = nullptr;
    q_create_info.flags = 0;
    q_create_info.queueFamilyIndex = this->gpu.qidx;
    q_create_info.queueCount = gpu.queue_properties.queueCount;
    q_create_info.pQueuePriorities = queue_priorities.data();

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

    /*
    * Once we've created the logical VkDevice that will be doing our work,
    * we need to grab the queues.  A VkQueue is how work is actually submitted
    * to our device.  I can think of no better place to grab them then as
    * the device (and thus the actual queues) are created.
    *
    * It's important to note that you don't _create_ queues, you merely take
    * a "pointer" to them from the device.  So when the device is destroyed
    * (or lost), your queues that you retrieve are destroyed along with it.
    * As a result, you will see no vkDestroyQueue or vkReleaseQueue call in
    * the VkState::Release() method.
    */
    this->queues.resize(this->gpu.queue_properties.queueCount);
    for (uint32_t i = 0; i < this->queues.size(); i++) {
        vkGetDeviceQueue(this->device, this->gpu.qidx, i, &this->queues[i]);
    }

    return result;
}

VkResult VkState::create_framebuffers(void)
{
    VkResult result = VK_SUCCESS;
    this->fbuffers.resize(this->swapchain.views.size());

    for (uint32_t i = 0; i < this->fbuffers.size(); i++) {
        VkImageView attachments[] = {
            this->swapchain.views[i]
        };

        VkFramebufferCreateInfo fci = {};
        fci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fci.renderPass = this->pipeline.renderpass;
        fci.attachmentCount = 1;
        fci.pAttachments = attachments;
        fci.width = this->swapchain.extent.width;
        fci.height = this->swapchain.extent.height;
        fci.layers = 1;

        result = vkCreateFramebuffer(this->device, &fci, nullptr,
          &this->fbuffers[i]);
        this->_assert(result, "vkCreateFramebuffer");
    }

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
    std::vector<const char*> extensions;
    extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef __linux
    extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif _WIN32
    extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif

    std::vector<const char*> layers;
#ifdef VKTEST_DEBUG
    layers.push_back("VK_LAYER_LUNARG_standard_validation");
    extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif

    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &application_info;
    create_info.enabledLayerCount = layers.size();
    create_info.ppEnabledLayerNames = layers.data();
    create_info.enabledExtensionCount = extensions.size();
    create_info.ppEnabledExtensionNames = extensions.data();

    return vkCreateInstance(&create_info, NULL, &this->instance);
}

VkResult VkState::create_pipeline(void)
{
    VkResult result = VK_SUCCESS;

    std::vector<char> vshader = _read_file(this, "./shaders/test.vert.spv");
    std::vector<char> fshader = _read_file(this, "./shaders/test.frag.spv");

    VkShaderModuleCreateInfo smci = {};
    smci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    smci.codeSize = vshader.size();
    smci.pCode = (const uint32_t*)vshader.data();
    result = vkCreateShaderModule(this->device, &smci, nullptr,
      &this->pipeline.vshadermodule);
    this->_assert(result, "vkCreateShaderModule: vertex shader.");

    smci.codeSize = fshader.size();
    smci.pCode = (const uint32_t*)fshader.data();
    result = vkCreateShaderModule(this->device, &smci, nullptr,
      &this->pipeline.fshadermodule);
    this->_assert(result, "vkCreateShaderModule: fragment shader.");

    VkPipelineShaderStageCreateInfo vssi = {};
    vssi.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vssi.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vssi.module = this->pipeline.vshadermodule;
    vssi.pName = "main";

    VkPipelineShaderStageCreateInfo fssi = {};
    fssi.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fssi.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fssi.module = this->pipeline.fshadermodule;
    fssi.pName = "main";

    std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
    shader_stages.push_back(vssi);
    shader_stages.push_back(fssi);

    /* grab the vertex data descriptions. */
    VkVertexInputBindingDescription bdesc = Vertex::getBindDesc();
    std::array<VkVertexInputAttributeDescription, 2> adescs =
      Vertex::getAttrDesc();

    VkPipelineVertexInputStateCreateInfo vertinputinfo = {};
    vertinputinfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertinputinfo.vertexBindingDescriptionCount = 1;
    vertinputinfo.pVertexBindingDescriptions = &bdesc;
    vertinputinfo.vertexAttributeDescriptionCount = adescs.size();
    vertinputinfo.pVertexAttributeDescriptions = adescs.data();

    VkPipelineInputAssemblyStateCreateInfo piasci = {};
    piasci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    piasci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    piasci.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)this->swapchain.extent.width;
    viewport.height = (float)this->swapchain.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = this->swapchain.extent;

    VkPipelineViewportStateCreateInfo vstate = {};
    vstate.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vstate.viewportCount = 1;
    vstate.pViewports = &viewport;
    vstate.scissorCount = 1;
    vstate.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rast = {};
    rast.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rast.depthClampEnable = VK_FALSE;
    rast.rasterizerDiscardEnable = VK_FALSE;
    rast.polygonMode = VK_POLYGON_MODE_FILL;
    rast.lineWidth = 1.0f;
    rast.cullMode = VK_CULL_MODE_BACK_BIT;
    rast.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rast.depthBiasEnable = VK_FALSE;
    rast.depthBiasConstantFactor = 0.0f;
    rast.depthBiasClamp = 0.0f;
    rast.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo ms = {};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.sampleShadingEnable = VK_FALSE;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    ms.minSampleShading = 1.0f;
    ms.pSampleMask = VK_NULL_HANDLE;
    ms.alphaToCoverageEnable = VK_FALSE;
    ms.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState cba = {};
    cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                         VK_COLOR_COMPONENT_G_BIT |
                         VK_COLOR_COMPONENT_B_BIT |
                         VK_COLOR_COMPONENT_A_BIT;
    cba.blendEnable = VK_FALSE;
    cba.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    cba.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    cba.colorBlendOp = VK_BLEND_OP_ADD;
    cba.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    cba.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    cba.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo cbsci = {};
    cbsci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cbsci.logicOpEnable = VK_FALSE;
    cbsci.logicOp = VK_LOGIC_OP_COPY;
    cbsci.attachmentCount = 1;
    cbsci.pAttachments = &cba;
    cbsci.blendConstants[0] = 0.0f;
    cbsci.blendConstants[1] = 0.0f;
    cbsci.blendConstants[2] = 0.0f;
    cbsci.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo plci = {};
    plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plci.setLayoutCount = 0;
    plci.pSetLayouts = nullptr;
    plci.pushConstantRangeCount = 0;
    plci.pPushConstantRanges = nullptr;

    result = vkCreatePipelineLayout(this->device, &plci, nullptr,
      &this->pipeline.layout);
    this->_assert(result, "vkCreatePipelineLayout");

    VkGraphicsPipelineCreateInfo pci = {};
    pci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pci.stageCount = 2;
    pci.pStages = shader_stages.data();
    pci.pVertexInputState = &vertinputinfo;
    pci.pInputAssemblyState = &piasci;
    pci.pViewportState = &vstate;
    pci.pRasterizationState = &rast;
    pci.pMultisampleState = &ms;
    pci.pDepthStencilState = nullptr;
    pci.pColorBlendState = &cbsci;
    pci.pDynamicState = nullptr;
    pci.layout = this->pipeline.layout;
    pci.renderPass = this->pipeline.renderpass;
    pci.subpass = 0;
    pci.basePipelineHandle = VK_NULL_HANDLE;
    pci.basePipelineIndex = -1;

    result = vkCreateGraphicsPipelines(this->device, VK_NULL_HANDLE, 1,
      &pci, nullptr, &this->pipeline.gpipeline);
    this->_assert(result, "vkCreateGraphicsPipelines");

    return result;
}

VkResult VkState::create_renderpass(void)
{
    VkResult result = VK_SUCCESS;

    VkAttachmentDescription color_attachment = {};
    color_attachment.format = this->swapchain.format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference car = {};
    car.attachment = 0;
    car.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &car;

    VkRenderPassCreateInfo rpci = {};
    rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpci.attachmentCount = 1;
    rpci.pAttachments = &color_attachment;
    rpci.subpassCount = 1;
    rpci.pSubpasses = &subpass;

    result = vkCreateRenderPass(this->device, &rpci, nullptr,
      &this->pipeline.renderpass);
    this->_assert(result, "vkCreateRenderPass");

    return result;
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
        this->swapchain.extent.width = static_cast<uint32_t>(this->width);
        this->swapchain.extent.height = static_cast<uint32_t>(this->height);
    } else {
        this->swapchain.extent.width = sc.currentExtent.width;
        this->swapchain.extent.height = sc.currentExtent.height;
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
#ifdef VKTEST_VSYNC
        if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            this->swapchain.mode = present_modes[i];
            break;
        }
#else
        if (present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            this->swapchain.mode = present_modes[i];
        }
#endif
    }

#ifdef VKTEST_DEBUG
    switch (this->swapchain.mode) {
    case VK_PRESENT_MODE_IMMEDIATE_KHR:
        std::cerr << "Immediate Mode" << std::endl;
        break;
    case VK_PRESENT_MODE_FIFO_KHR:
        std::cerr << "FIFO" << std::endl;
        break;
    case VK_PRESENT_MODE_MAILBOX_KHR:
        std::cerr << "Mailbox" << std::endl;
        break;
    default:
        std::cerr << "Some other mode.." << std::endl;
    }
#endif

    VkSwapchainCreateInfoKHR sci = {};
    sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sci.pNext = VK_NULL_HANDLE;
    sci.flags = 0;
    sci.surface = swapchain.surface;
    sci.minImageCount = image_count;        // should I _require_ two?
    sci.imageFormat = this->swapchain.format;
    sci.imageColorSpace = this->swapchain.colorspace;
    sci.imageExtent = this->swapchain.extent;
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

    result = vkCreateSwapchainKHR(this->device, &sci, NULL,
      &this->swapchain.chain);
    this->_assert(result, "vkCreateSwapchainKHR");

    count = 0;
    result = vkGetSwapchainImagesKHR(this->device, this->swapchain.chain,
      &count, nullptr);
    this->_assert(result, "vkGetSwapchainImagesKHR");

    this->swapchain.images.resize(count);
    result = vkGetSwapchainImagesKHR(this->device, this->swapchain.chain,
      &count, this->swapchain.images.data());
    this->_assert(result, "vkGetSwapchainImagesKHR");

    this->swapchain.views.resize(count);
    for (uint32_t i = 0; i < count; i++) {
        VkImageViewCreateInfo ivci = {};
        ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ivci.image = this->swapchain.images[i];
        ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ivci.format = this->swapchain.format;
        ivci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        ivci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        ivci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        ivci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        ivci.subresourceRange.baseMipLevel = 0;
        ivci.subresourceRange.levelCount = 1;
        ivci.subresourceRange.baseArrayLayer = 0;
        ivci.subresourceRange.layerCount = 1;

        result = vkCreateImageView(this->device, &ivci, nullptr,
          &this->swapchain.views[i]);
        this->_assert(result, "vkCreateImageView");
    }

    VkSemaphoreCreateInfo semci = {};
    semci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    result = vkCreateSemaphore(this->device, &semci, nullptr,
      &this->swapchain.semready);
    this->_assert(result, "vkCreateSemaphore: semready");

    result = vkCreateSemaphore(this->device, &semci, nullptr,
      &this->swapchain.semfinished);
    this->_assert(result, "vkCreateSemaphore: semfinished");

    return result;
}

uint32_t VkState::find_memory_type(uint32_t filter, VkMemoryPropertyFlags flags)
{
    VkPhysicalDeviceMemoryProperties props = this->gpu.memory_properties;
    uint32_t count = props.memoryTypeCount;

    for (uint32_t i = 0; i < count; i++) {
        if ((filter & (1 << i)) && (props.memoryTypes[i].propertyFlags &
          flags) == flags) {
            return i;
        }
    }

    this->_assert(VK_ERROR_INCOMPATIBLE_DRIVER,
      "Failed to find suitable GPU memory for vertex data");

    return UINT32_MAX;
}

VkResult VkState::release_device_objects(void)
{
    vkDestroyDevice(this->device, nullptr);
    vkDestroySurfaceKHR(this->instance, this->swapchain.surface, nullptr);

    return VK_SUCCESS;
}

VkResult VkState::release_instance_objects(void)
{
    this->release_debug();
    vkDestroyInstance(this->instance, nullptr);

    return VK_SUCCESS;
}

VkResult VkState::release_render_objects(void)
{
    vkResetCommandPool(this->device, this->cmdpool,
      VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

    for (uint32_t i = 0; i < this->fbuffers.size(); i++) {
        vkDestroyFramebuffer(this->device, this->fbuffers[i], nullptr);
    }

    vkFreeMemory(this->device, this->vbuffermem, nullptr);
    vkDestroyBuffer(this->device, this->vbuffer, nullptr);

    vkDestroyPipeline(this->device, this->pipeline.gpipeline, nullptr);
    vkDestroyPipelineLayout(this->device, this->pipeline.layout, nullptr);
    vkDestroyRenderPass(this->device, this->pipeline.renderpass, nullptr);
    vkDestroyShaderModule(this->device,
      this->pipeline.vshadermodule, nullptr);
    vkDestroyShaderModule(this->device,
      this->pipeline.fshadermodule, nullptr);
    for (uint32_t i = 0; i < this->swapchain.views.size(); i++) {
        vkDestroyImageView(this->device, this->swapchain.views[i], nullptr);
    }

    vkDestroySwapchainKHR(this->device, this->swapchain.chain, nullptr);
    vkFreeCommandBuffers(this->device, this->cmdpool,
      this->cbuffers.size(), this->cbuffers.data());
    vkDestroyCommandPool(this->device, this->cmdpool, nullptr);

    return VK_SUCCESS;
}

VkResult VkState::release_sync_objects(void)
{
    vkDestroyFence(this->device, this->fence, nullptr);
    vkDestroySemaphore(this->device, this->swapchain.semready, nullptr);
    vkDestroySemaphore(this->device, this->swapchain.semfinished, nullptr);

    return VK_SUCCESS;
}

std::vector<char> _read_file(VkState* state, std::string path)
{
    std::fstream in;
    std::vector<char> ret;

    in.open(path, std::fstream::binary | std::fstream::in);
    if (!in.is_open()) {
        std::stringstream out;
        out << "Unable to open file \'" << path << "\'" << std::endl;
        state->_assert(VK_INCOMPLETE, out.str());
        return ret;
    }

    uint32_t len = 0;
    in.seekg(0L, in.end);
    len = in.tellg();
    in.seekg(0L, in.beg);

    ret.resize(len);
    in.read(ret.data(), len);

    return ret;
}

