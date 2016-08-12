#include "renderer.h"

Renderer* Renderer::Init(CreateInfo* info)
{
    VkResult result = VK_SUCCESS;

    /* Check to see if the video subsystem was initialized before proceeding. */
    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
        return nullptr;
    }

    Renderer* ret = new Renderer();

    /* If we don't receive anything in the parameter,
     * just fill with safe default values */
    if (info == nullptr) {
        ret->m_cinfo.width = RENDERER_DEFAULT_WIDTH;
        ret->m_cinfo.height = RENDERER_DEFAULT_HEIGHT;
        ret->m_cinfo.flags = Renderer::RESIZABLE;
    } else {
        memcpy(&ret->m_cinfo, info, sizeof(CreateInfo));
    }

    ret->m_window = ret->create_window();

    result = ret->create_instance();
    Assert(result, "Failed to create Vulkan instance.  Do you have a "
      "compatible system with up-to-date drivers?", ret->m_window);

    result = ret->init_debug();
    Assert(result, "Failed to initialize the debug extension.", ret->m_window);

    result = ret->create_surface();
    Assert(result, "Unable to create VkSurfaceKHR", ret->m_window);

    result = ret->create_device();
    Assert(result, "Failed to create a rendering device.  Do you have "
      "up-to-date drivers?", ret->m_window);

    result = ret->create_swapchain();
    Assert(result, "Unable to create Swapchain.", ret->m_window);

    result = ret->create_renderpass();
    Assert(result, "Unable to create Renderpass object.", ret->m_window);

    result = ret->create_descriptorset_layout();
    Assert(result, "Unable to create descriptorset layout.", ret->m_window);

    result = ret->create_pipeline();
    Assert(result, "Unable to create graphics pipeline.", ret->m_window);

    result = ret->create_framebuffers();
    Assert(result, "Unable to create framebuffer objects.", ret->m_window);

    result = ret->create_cmdpool();
    Assert(result, "Unable to create command pool", ret->m_window);

    result = ret->create_vertexbuffer();
    Assert(result, "Unable to create vertex buffers.", ret->m_window);

    result = ret->create_indexbuffer();
    Assert(result, "Unable to create index buffers.", ret->m_window);

    result = ret->create_uniformbuffer();
    Assert(result, "Unable to create uniform buffers.", ret->m_window);

    result = ret->create_descriptorpool();
    Assert(result, "Unable to create descriptor pool.", ret->m_window);

    result = ret->create_descriptorset();
    Assert(result, "Unable to create descriptor set.", ret->m_window);

    result = ret->create_cmdbuffers();
    Assert(result, "Unable to create command buffers.", ret->m_window);

    result = ret->create_synchronizers();
    Assert(result, "Unable to create synchronization primitives.",
      ret->m_window);

    return ret;
}

void Renderer::Release(Renderer* state)
{
    vkDeviceWaitIdle(state->m_device);

    state->release_render_objects();
    state->release_sync_objects();
    state->release_device_objects();
    state->release_instance_objects();

    SDL_DestroyWindow(state->m_window);
    delete(state);
}

void Renderer::PushEvent(SDL_WindowEvent event)
{
    m_events.push(event);
}

void Renderer::RecreateSwapchain(void)
{
    vkDeviceWaitIdle(m_device);

    this->release_render_objects();

    this->create_swapchain();
    this->create_renderpass();
    this->create_descriptorset_layout();
    this->create_pipeline();
    this->create_framebuffers();
    this->create_cmdpool();
    this->create_vertexbuffer();
    this->create_indexbuffer();
    this->create_uniformbuffer();
    this->create_descriptorpool();
    this->create_descriptorset();
    this->create_cmdbuffers();

    vkDeviceWaitIdle(m_device);
}

void Renderer::Render(void)
{
    VkResult result = VK_SUCCESS;

    uint32_t idx = 0;
    vkAcquireNextImageKHR(m_device, m_swapchain.chain, UINT64_MAX,
      m_swapchain.semready, VK_NULL_HANDLE, &idx);

    VkSemaphore waitsems[] = { m_swapchain.semready };
    VkSemaphore sigsems[] = { m_swapchain.semfinished };
    VkPipelineStageFlags waitstages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo si = {};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.waitSemaphoreCount = 1;
    si.pWaitSemaphores = waitsems;
    si.pWaitDstStageMask = waitstages;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &m_cmdbuffers[idx];
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores = sigsems;

    result = vkQueueSubmit(m_renderqueue, 1, &si, VK_NULL_HANDLE);
    Assert(result, "vkQueueSubmit", m_window);

    VkSwapchainKHR swapchains[] = { m_swapchain.chain };

    VkPresentInfoKHR pi = {};
    pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores = sigsems;
    pi.swapchainCount = 1;
    pi.pSwapchains = swapchains;
    pi.pImageIndices = &idx;

    vkQueuePresentKHR(m_renderqueue, &pi);
    vkQueueWaitIdle(m_renderqueue);

    m_fpsinfo.framecount++;
}

void Renderer::Update(double elapsed)
{

    while (!m_events.empty()) {
        SDL_WindowEvent ev = m_events.front();
        switch (ev.event) {
        case SDL_WINDOWEVENT_RESIZED:
            this->RecreateSwapchain();
            break;
        }

        m_events.pop();
    }

    UniformBufferObject ubo = {};

    ubo.model = glm::rotate(glm::mat4(), static_cast<float>(elapsed) *
      glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    ubo.view = glm::lookAt(
      glm::vec3(2.0f, 2.0f, 2.0f),
      glm::vec3(0.0f, 0.0f, 0.0f),
      glm::vec3(0.0f, 0.0f, 1.0f)
    );

    float ratio = m_swapchain.extent.width / m_swapchain.extent.height;
    ubo.proj = glm::perspective(glm::radians(45.0f), ratio, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;       // y-axis is opposite of OpenGL in Vulkan.

    void* data = nullptr;
    vkMapMemory(m_device, m_box.usbuffermem, 0, sizeof(ubo), 0, &data);
    std::memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(m_device, m_box.usbuffermem);

    copy_buffer(m_box.usbuffer, m_box.ubuffer, sizeof(ubo));

    if (elapsed - m_fpsinfo.last >= 1.0) {
        std::stringstream out;
        out << RENDERER_WINDOW_NAME << " | ";
        out << "FPS: " << m_fpsinfo.framecount;

        m_fpsinfo.framecount = 0;
        m_fpsinfo.last = elapsed;

        /* temporary solution.  I would eventually like to render test
        * in the window.  But that's a story for another day. */
        if (m_cinfo.flags & Renderer::FPS_ON) {
            SDL_SetWindowTitle(m_window, out.str().c_str());
        }

        /* log to stdout regardless. */
        std::cout << out.str() << std::endl;
    }
}


VkResult Renderer::copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
    VkCommandBufferAllocateInfo cbai = {};
    cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandPool = m_cmdpool;
    cbai.commandBufferCount = 1;

    VkCommandBuffer cbuff;
    vkAllocateCommandBuffers(m_device, &cbai, &cbuff);

    VkCommandBufferBeginInfo cbbi = {};
    cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cbuff, &cbbi);

    VkBufferCopy copyregion = {};
    copyregion.srcOffset = 0;
    copyregion.dstOffset = 0;
    copyregion.size = size;
    vkCmdCopyBuffer(cbuff, src, dst, 1, &copyregion);

    vkEndCommandBuffer(cbuff);

    VkSubmitInfo submitinfo = {};
    submitinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitinfo.commandBufferCount = 1;
    submitinfo.pCommandBuffers = &cbuff;

    vkQueueSubmit(m_renderqueue, 1, &submitinfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_renderqueue);

    vkFreeCommandBuffers(m_device, m_cmdpool, 1, &cbuff);

    return VK_SUCCESS;
}

VkResult Renderer::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
  VkMemoryPropertyFlags properties, VkBuffer* buffer,
  VkDeviceMemory* buffer_memory)
{
    VkResult result = VK_SUCCESS;

    VkBufferCreateInfo bci = {};
    bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bci.size = size;
    bci.usage = usage;
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    result = vkCreateBuffer(m_device, &bci, nullptr, buffer);
    if (result) {
        return result;
    }

    VkMemoryRequirements memreq;
    vkGetBufferMemoryRequirements(m_device, *buffer, &memreq);

    VkMemoryAllocateInfo mai = {};
    mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mai.allocationSize = memreq.size;
    mai.memoryTypeIndex = find_memory_type(memreq.memoryTypeBits, properties);

    result = vkAllocateMemory(m_device, &mai, nullptr, buffer_memory);
    if (result) {
        return result;
    }

    return vkBindBufferMemory(m_device, *buffer, *buffer_memory, 0);
}

VkResult Renderer::create_descriptorset(void)
{
    VkResult result = VK_SUCCESS;

    VkDescriptorSetLayout layouts[] = { m_box.dslayout };
    VkDescriptorSetAllocateInfo allocinfo = {};
    allocinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocinfo.descriptorPool = m_box.dpool;
    allocinfo.descriptorSetCount = 1;
    allocinfo.pSetLayouts = layouts;

    result =  vkAllocateDescriptorSets(m_device, &allocinfo, &m_box.dset);
    if (result) {
        return result;
    }

    VkDescriptorBufferInfo bi = {};
    bi.buffer = m_box.ubuffer;
    bi.offset = 0;
    bi.range = sizeof(UniformBufferObject);

    VkWriteDescriptorSet dw = {};
    dw.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    dw.dstSet = m_box.dset;
    dw.dstBinding = 0;
    dw.dstArrayElement = 0;
    dw.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dw.descriptorCount = 1;
    dw.pBufferInfo = &bi;
    dw.pImageInfo = nullptr;
    dw.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(m_device, 1, &dw, 0, nullptr);

    return result;
}

VkResult Renderer::create_descriptorpool(void)
{
    VkResult result = VK_SUCCESS;

    VkDescriptorPoolSize poolsize = {};
    poolsize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolsize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolinfo = {};
    poolinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolinfo.poolSizeCount = 1;
    poolinfo.pPoolSizes = &poolsize;
    poolinfo.maxSets = 1;

    result = vkCreateDescriptorPool(m_device, &poolinfo, nullptr, &m_box.dpool);

    return result;
}

VkResult Renderer::create_descriptorset_layout(void)
{
    VkResult result = VK_SUCCESS;

    VkDescriptorSetLayoutBinding ubo_layout_binding = {};
    ubo_layout_binding.binding = 0;
    ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_layout_binding.descriptorCount = 1;
    ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    ubo_layout_binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo li = {};
    li.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    li.bindingCount = 1;
    li.pBindings = &ubo_layout_binding;

    result = vkCreateDescriptorSetLayout(m_device, &li, nullptr,
      &m_box.dslayout);
    if (result) {
        return result;
    }

    return result;
}

VkResult Renderer::create_uniformbuffer(void)
{
    VkResult result = VK_SUCCESS;
    VkDeviceSize buffersize = sizeof(UniformBufferObject);

    result = create_buffer(buffersize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &m_box.usbuffer,
      &m_box.usbuffermem);
    if (result) {
        return result;
    }

    result = create_buffer(buffersize, VK_BUFFER_USAGE_TRANSFER_DST_BIT |
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      &m_box.ubuffer, &m_box.ubuffermem);

    return result;
}

VkResult Renderer::create_vertexbuffer(void)
{
    VkResult result = VK_SUCCESS;

    m_box.vertices = {
       {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
       {{ 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
       {{ 0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
       {{-0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}}
    };

    VkDeviceSize buffersize = (sizeof(m_box.vertices[0]) *
      m_box.vertices.size());

    VkBuffer stagingbuffer;
    VkDeviceMemory stagingbuffermemory;

    result = create_buffer(buffersize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingbuffer,
      &stagingbuffermemory);
    if (result) {
        return result;
    }

    void* data = nullptr;
    vkMapMemory(m_device, stagingbuffermemory, 0, buffersize, 0, &data);
    std::memcpy(data, m_box.vertices.data(), (size_t)buffersize);
    vkUnmapMemory(m_device, stagingbuffermemory);

    result = create_buffer(buffersize, VK_BUFFER_USAGE_TRANSFER_DST_BIT |
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      &m_box.vbuffer, &m_box.vbuffermem);
    if (result) {
        return result;
    }

    result = copy_buffer(stagingbuffer, m_box.vbuffer, buffersize);
    if (result) {
        return result;
    }

    vkFreeMemory(m_device, stagingbuffermemory, nullptr);
    vkDestroyBuffer(m_device, stagingbuffer, nullptr);

    return result;
}

VkResult Renderer::create_indexbuffer(void)
{
    VkResult result = VK_SUCCESS;

    m_box.indices = {
        0, 1, 2,
        2, 3, 0
    };

    VkDeviceSize bsize = (sizeof(m_box.indices[0]) * m_box.indices.size());

    VkBuffer stagingbuffer;
    VkDeviceMemory stagingbuffermemory;

    result = create_buffer(bsize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      &stagingbuffer, &stagingbuffermemory);
    Assert(result, "create_buffer: m_indices staging buffer", m_window);

    void* data = nullptr;
    vkMapMemory(m_device, stagingbuffermemory, 0, bsize, 0, &data);
    std::memcpy(data, m_box.indices.data(), (size_t)bsize);
    vkUnmapMemory(m_device, stagingbuffermemory);

    result = create_buffer(bsize, VK_BUFFER_USAGE_TRANSFER_DST_BIT |
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      &m_box.ibuffer, &m_box.ibuffermem);
    Assert(result, "create_buffer: m_ibuffer and m_ibuffermem", m_window);

    result = copy_buffer(stagingbuffer, m_box.ibuffer, bsize);
    Assert(result, "copy_buffer: stagingbuffer to m_ibuffer");

    vkFreeMemory(m_device, stagingbuffermemory, nullptr);
    vkDestroyBuffer(m_device, stagingbuffer, nullptr);

    return result;
}

uint32_t Renderer::find_memory_type(uint32_t filter,
  VkMemoryPropertyFlags flags)
{
    VkPhysicalDeviceMemoryProperties props = m_gpu.memory_properties;
    uint32_t count = props.memoryTypeCount;

    for (uint32_t i = 0; i < count; i++) {
        if ((filter & (1 << i)) && (props.memoryTypes[i].propertyFlags &
          flags) == flags) {
            return i;
        }
    }

    Assert(VK_ERROR_INCOMPATIBLE_DRIVER,
      "Failed to find suitable GPU memory for vertex data", m_window);

    return UINT32_MAX;
}

