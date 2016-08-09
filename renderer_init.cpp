#include "renderer.h"

VkResult Renderer::create_cmdbuffers(void)
{
    VkResult result = VK_SUCCESS;

    /* Command buffer creation */
    VkCommandBufferAllocateInfo cbai = {};
    cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.pNext = nullptr;
    cbai.commandPool = m_cmdpool;
    cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = m_fbuffers.size();
    m_cmdbuffers.resize(m_fbuffers.size());
    result = vkAllocateCommandBuffers(m_device, &cbai,
      m_cmdbuffers.data());
    Assert(result, "vkAllocateCommandBuffers", m_window);

    /* I'm not sure what this is...will figure out. */
    for (uint32_t i = 0; i < m_cmdbuffers.size(); i++) {
        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        vkBeginCommandBuffer(m_cmdbuffers[i], &begin_info);

        VkRenderPassBeginInfo rpi = {};
        rpi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpi.renderPass = m_pipeline.renderpass;
        rpi.framebuffer = m_fbuffers[i];
        rpi.renderArea.offset = { 0, 0 };
        rpi.renderArea.extent = m_swapchain.extent;

        VkClearValue clear_color = { 0.2f, 0.2f, 0.2f, 1.0f };
        rpi.clearValueCount = 1;
        rpi.pClearValues = &clear_color;

        vkCmdBeginRenderPass(m_cmdbuffers[i], &rpi,
          VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(m_cmdbuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
          m_pipeline.gpipeline);

        VkBuffer buffs[] = {m_vbuffer};
        VkDeviceSize offsets[] = { 0 };

        vkCmdBindVertexBuffers(m_cmdbuffers[i], 0, 1, buffs, offsets);
        vkCmdBindIndexBuffer(m_cmdbuffers[i], m_ibuffer, 0,
          VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(m_cmdbuffers[i], m_indices.size(), 1, 0, 0, 0);

        vkCmdEndRenderPass(m_cmdbuffers[i]);

        result = vkEndCommandBuffer(m_cmdbuffers[i]);
        Assert(result, "vkEndCommandBuffer", m_window);
    }

    return VK_SUCCESS;

}

VkResult Renderer::create_cmdpool(void)
{
    /* Command pool creation */
    VkCommandPoolCreateInfo cpci = {};
    cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cpci.pNext = nullptr;
    cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cpci.queueFamilyIndex = m_gpu.qidx;

    return vkCreateCommandPool(m_device, &cpci, nullptr, &m_cmdpool);
}

VkResult Renderer::create_device(void)
{
    VkResult result = VK_SUCCESS;
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
    result = vkEnumeratePhysicalDevices(m_instance, &device_count, nullptr);

    physical_devices.resize(device_count);
    result = vkEnumeratePhysicalDevices(m_instance, &device_count,
      physical_devices.data());

    /*
     * Then, it's necessary to cycle through all your physical devices to
     * determine which has a queue capable of both rendering and presenting.
     * I imagine that it would be possible to come across a device that can
     * do one or the other, so I'm not considering that case.
     */
    m_gpu.qidx = UINT32_MAX;
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
                  target_device, j, m_swapchain.surface, &support);
                Assert(tr, "vkGetPhysicalDeviceSurfaceSupportKHR",
                  m_window);

                /* Notice that we're saving both the physical device and the
                 * queue index that supports presenting and rendering. */
                if (support == VK_TRUE) {
                    m_gpu.device = physical_devices[i];
                    m_gpu.qidx = j;
                    m_gpu.queue_properties = target_queue;
                    break;
                }
            }
        }

        /* Used to determine if a suitable device and queue index has already
         * been found to break out of the outer VkPhysicalDevice loop. */
        if (m_gpu.qidx != UINT32_MAX) {
            break;
        }
    }

    /*
    * Now that we know which GPU we're going to be using to render with,
    * we capture some additional information that will be useful for us
    * in the future.
    */
    vkGetPhysicalDeviceFeatures(m_gpu.device, &m_gpu.features);
    vkGetPhysicalDeviceProperties(m_gpu.device, &m_gpu.properties);
    vkGetPhysicalDeviceMemoryProperties(m_gpu.device,
      &m_gpu.memory_properties);

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
    queue_priorities.resize(m_gpu.queue_properties.queueCount);
    for (uint32_t i = 1; i <= queue_priorities.size(); i++) {
        queue_priorities[i-1] = 1.0f / i;
    }


    VkDeviceQueueCreateInfo q_create_info = {};
    q_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    q_create_info.pNext = nullptr;
    q_create_info.flags = 0;
    q_create_info.queueFamilyIndex = m_gpu.qidx;
    q_create_info.queueCount = m_gpu.queue_properties.queueCount;
    q_create_info.pQueuePriorities = queue_priorities.data();

    const char* swap_extension[] = { "VK_KHR_swapchain" };

#ifdef VKTEST_DEBUG
    uint32_t layer_count = 1;
    const char* dev_layers[] = { "VK_LAYER_LUNARG_standard_validation" };
#else
    uint32_t layer_count = 0;
    const char* dev_layers[] = { nullptr };
#endif

    m_gpu.features.shaderClipDistance = VK_TRUE;

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
    d_create_info.pEnabledFeatures = &m_gpu.features;

    result = vkCreateDevice(m_gpu.device, &d_create_info, NULL, 
      &m_device);

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
    * the Renderer::Release() method.
    */
    m_queues.resize(m_gpu.queue_properties.queueCount);
    for (uint32_t i = 0; i < m_queues.size(); i++) {
        vkGetDeviceQueue(m_device, m_gpu.qidx, i, &m_queues[i]);
    }

    return result;
}

VkResult Renderer::create_framebuffers(void)
{
    VkResult result = VK_SUCCESS;
    m_fbuffers.resize(m_swapchain.views.size());

    for (uint32_t i = 0; i < m_fbuffers.size(); i++) {
        VkImageView attachments[] = {
            m_swapchain.views[i]
        };

        VkFramebufferCreateInfo fci = {};
        fci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fci.renderPass = m_pipeline.renderpass;
        fci.attachmentCount = 1;
        fci.pAttachments = attachments;
        fci.width = m_swapchain.extent.width;
        fci.height = m_swapchain.extent.height;
        fci.layers = 1;

        result = vkCreateFramebuffer(m_device, &fci, nullptr,
          &m_fbuffers[i]);
        Assert(result, "vkCreateFramebuffer", m_window);
    }

    return result;
}

VkResult Renderer::create_instance(void)
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
#if defined(__linux__)
    extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined(_WIN32)
    extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif

    std::vector<const char*> layers;
#if defined(VKTEST_DEBUG)
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

    return vkCreateInstance(&create_info, NULL, &m_instance);
}

VkResult Renderer::create_pipeline(void)
{
    VkResult result = VK_SUCCESS;

    std::vector<char> vshader = ReadFile("./shaders/test.vert.spv");
    std::vector<char> fshader = ReadFile("./shaders/test.frag.spv");

    VkShaderModuleCreateInfo smci = {};
    smci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    smci.codeSize = vshader.size();
    smci.pCode = (const uint32_t*)vshader.data();
    result = vkCreateShaderModule(m_device, &smci, nullptr,
      &m_pipeline.vshadermodule);
    Assert(result, "vkCreateShaderModule: vertex shader.", m_window);

    smci.codeSize = fshader.size();
    smci.pCode = (const uint32_t*)fshader.data();
    result = vkCreateShaderModule(m_device, &smci, nullptr,
      &m_pipeline.fshadermodule);
    Assert(result, "vkCreateShaderModule: fragment shader.", m_window);

    VkPipelineShaderStageCreateInfo vssi = {};
    vssi.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vssi.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vssi.module = m_pipeline.vshadermodule;
    vssi.pName = "main";

    VkPipelineShaderStageCreateInfo fssi = {};
    fssi.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fssi.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fssi.module = m_pipeline.fshadermodule;
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
    viewport.width = (float)m_swapchain.extent.width;
    viewport.height = (float)m_swapchain.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = m_swapchain.extent;

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

    result = vkCreatePipelineLayout(m_device, &plci, nullptr,
      &m_pipeline.layout);
    Assert(result, "vkCreatePipelineLayout", m_window);

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
    pci.layout = m_pipeline.layout;
    pci.renderPass = m_pipeline.renderpass;
    pci.subpass = 0;
    pci.basePipelineHandle = VK_NULL_HANDLE;
    pci.basePipelineIndex = -1;

    result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1,
      &pci, nullptr, &m_pipeline.gpipeline);
    Assert(result, "vkCreateGraphicsPipelines", m_window);

    return result;
}

VkResult Renderer::create_renderpass(void)
{
    VkResult result = VK_SUCCESS;

    VkAttachmentDescription color_attachment = {};
    color_attachment.format = m_swapchain.format;
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

    result = vkCreateRenderPass(m_device, &rpci, nullptr,
      &m_pipeline.renderpass);
    Assert(result, "vkCreateRenderPass", m_window);

    return result;
}

VkResult Renderer::create_surface(void)
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
    if (SDL_GetWindowWMInfo(m_window, &info) != SDL_TRUE) {
        Assert(VK_INCOMPLETE, SDL_GetError(), m_window);
    }

#if defined(__linux__)
    VkXcbSurfaceCreateInfoKHR si = {};
    si.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    si.pNext = nullptr;
    si.flags = 0;
    si.connection = XGetXCBConnection(info.info.x11.display);
    si.window = info.info.x11.window;
    result = vkCreateXcbSurfaceKHR(m_instance, &si, nullptr,
      &m_swapchain.surface);
#elif defined(_WIN32)
    VkWin32SurfaceCreateInfoKHR si = {};
    si.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    si.pNext = nullptr;
    si.flags = 0;
    si.hinstance = GetModuleHandle(NULL);
    si.hwnd = info.info.win.window;
    result = vkCreateWin32SurfaceKHR(m_instance, &si, nullptr,
      &m_swapchain.surface);
#else
    Assert(VK_ERROR_FEATURE_NOT_PRESENT, "Unable to detect platform for "
      "VkSurfaceKHR creation.", m_window);
#endif
    return result;
}

VkResult Renderer::create_swapchain(void)
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
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_gpu.device,
      m_swapchain.surface, &count, nullptr);
    Assert(result, "vkGetPhysicalDeviceSurfaceFormatsKHR", m_window);

    surface_formats.resize(count);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_gpu.device,
      m_swapchain.surface, &count, surface_formats.data());
    Assert(result, "vkGetPhysicalDeviceSurfaceFormatsKHR", m_window);

    /* There appears to be only one color space, but I'll grab it from
     * the surface formats just to be safe. */
    m_swapchain.colorspace = surface_formats[0].colorSpace;
    if (count == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED) {
        m_swapchain.format = VK_FORMAT_B8G8R8_UNORM;
    } else {
        m_swapchain.format = surface_formats[0].format;
    }

    /*
    * Next, we grab our surface capabilities.  This will allow us to discern
    * how many buffers that the machine is capable of presenting.  I'm needing
    * a double-buffered swapchain, but I'm going through the checks to ensure
    * the device actually supports that.  Triple-buffering is also pretty
    * pretty common, but I'd rather not add the complexity.
    */
    VkSurfaceCapabilitiesKHR sc = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_gpu.device,
      m_swapchain.surface, &sc);

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
    int width, height;
    SDL_GetWindowSize(m_window, &width, &height);

    /* Compare that to the surface capabilities structure. */
    if (sc.currentExtent.width == UINT32_MAX) {
        m_swapchain.extent.width = static_cast<uint32_t>(width);
        m_swapchain.extent.height = static_cast<uint32_t>(height);
    } else {
        m_swapchain.extent.width = sc.currentExtent.width;
        m_swapchain.extent.height = sc.currentExtent.height;
    }

    VkSurfaceTransformFlagBitsKHR pre_transform = sc.currentTransform;
    if (sc.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }

    count = 0;
    std::vector<VkPresentModeKHR> present_modes;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_gpu.device,
      m_swapchain.surface, &count, nullptr);

    present_modes.resize(count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_gpu.device,
      m_swapchain.surface, &count, present_modes.data());

    VkPresentModeKHR want;
    if (m_cinfo.flags & Renderer::VSYNC_ON) {
        want = VK_PRESENT_MODE_MAILBOX_KHR;
    } else {
        want = VK_PRESENT_MODE_IMMEDIATE_KHR;
    }
    m_swapchain.mode = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32_t i = 0; i < present_modes.size(); i++) {
        if (present_modes[i] == want) {
            m_swapchain.mode = want;
            break;
        }
    }

    VkSwapchainCreateInfoKHR sci = {};
    sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sci.pNext = VK_NULL_HANDLE;
    sci.flags = 0;
    sci.surface = m_swapchain.surface;
    sci.minImageCount = image_count;        // should I _require_ two?
    sci.imageFormat = m_swapchain.format;
    sci.imageColorSpace = m_swapchain.colorspace;
    sci.imageExtent = m_swapchain.extent;
    sci.imageArrayLayers = 1;   // 1, since we're not doing stereoscopic 3D :|
    sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sci.queueFamilyIndexCount = 0;              // because .imageSharingMode
    sci.pQueueFamilyIndices = VK_NULL_HANDLE;   // because .imageSharingMode
    sci.preTransform = pre_transform;
    sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sci.presentMode = m_swapchain.mode;
    sci.clipped = VK_TRUE;
    sci.oldSwapchain = VK_NULL_HANDLE;

    result = vkCreateSwapchainKHR(m_device, &sci, NULL,
      &m_swapchain.chain);
    Assert(result, "vkCreateSwapchainKHR", m_window);

    count = 0;
    result = vkGetSwapchainImagesKHR(m_device, m_swapchain.chain,
      &count, nullptr);
    Assert(result, "vkGetSwapchainImagesKHR", m_window);

    m_swapchain.images.resize(count);
    result = vkGetSwapchainImagesKHR(m_device, m_swapchain.chain,
      &count, m_swapchain.images.data());
    Assert(result, "vkGetSwapchainImagesKHR", m_window);

    m_swapchain.views.resize(count);
    for (uint32_t i = 0; i < count; i++) {
        VkImageViewCreateInfo ivci = {};
        ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ivci.image = m_swapchain.images[i];
        ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ivci.format = m_swapchain.format;
        ivci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        ivci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        ivci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        ivci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        ivci.subresourceRange.baseMipLevel = 0;
        ivci.subresourceRange.levelCount = 1;
        ivci.subresourceRange.baseArrayLayer = 0;
        ivci.subresourceRange.layerCount = 1;

        result = vkCreateImageView(m_device, &ivci, nullptr,
          &m_swapchain.views[i]);
        Assert(result, "vkCreateImageView", m_window);
    }

    return result;
}

VkResult Renderer::create_synchronizers(void)
{
    VkResult result = VK_SUCCESS;

    VkSemaphoreCreateInfo semci = {};
    semci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    result = vkCreateSemaphore(m_device, &semci, nullptr,
      &m_swapchain.semready);
    if (result) {
        return result;
    }

    result = vkCreateSemaphore(m_device, &semci, nullptr,
      &m_swapchain.semfinished);

    return result;
}

SDL_Window* Renderer::create_window(void)
{
    SDL_DisplayMode displaymode;
    int r = SDL_GetCurrentDisplayMode(0, &displaymode);
    if (r) {
        Assert(VK_ERROR_FEATURE_NOT_PRESENT, "Unable to determine current "
          "display mode.");
    }

    uint32_t window_flags = 0;
    if (m_cinfo.flags & Renderer::FULLSCREEN) {
        window_flags |= SDL_WINDOW_FULLSCREEN;
        m_cinfo.width = static_cast<uint16_t>(displaymode.w);
        m_cinfo.height = static_cast<uint16_t>(displaymode.h);
    } else {
        /* Check for sane defaults first. */
        if (m_cinfo.width == UINT16_MAX || m_cinfo.width == 0) {
            m_cinfo.width = 800;
        }

        if (m_cinfo.height == UINT16_MAX || m_cinfo.height == 0) {
            m_cinfo.height = 600;
        }

        if (m_cinfo.flags & Renderer::RESIZABLE) {
            window_flags |= SDL_WINDOW_RESIZABLE;
        }
    }

    SDL_Window* ret = SDL_CreateWindow(RENDERER_WINDOW_NAME,
      SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
      static_cast<int>(m_cinfo.width), static_cast<int>(m_cinfo.height),
      window_flags);
    if (ret == nullptr) {
        Assert(VK_ERROR_FEATURE_NOT_PRESENT, "Failed to create SDL_Window.");
    }

    return ret;
}

