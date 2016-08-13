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

    result = ret->create_cmdpool();
    Assert(result, "Unable to create command pool", ret->m_window);

    result = ret->create_depthresources();
    Assert(result, "Unable to create depth resources.", ret->m_window);

    result = ret->create_framebuffers();
    Assert(result, "Unable to create framebuffer objects.", ret->m_window);

    result = ret->create_texture();
    Assert(result, "Unable to create texture!", ret->m_window);

    result = ret->create_textureimageview();
    Assert(result, "Unable to create texture view.", ret->m_window);

    result = ret->create_sampler();
    Assert(result, "Unable to create sampler for texture.", ret->m_window);

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
    this->create_texture();
    this->create_textureimageview();
    this->create_sampler();
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

VkResult Renderer::begin_single_time_commands(VkCommandBuffer* cbuff)
{
    VkCommandBufferAllocateInfo ai = {};
    ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandPool = m_cmdpool;
    ai.commandBufferCount = 1;

    vkAllocateCommandBuffers(m_device, &ai, cbuff);

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(*cbuff, &begin_info);

    return VK_SUCCESS;
}

VkResult Renderer::end_single_time_commands(VkCommandBuffer cbuff)
{
    vkEndCommandBuffer(cbuff);

    VkSubmitInfo si = {};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cbuff;

    vkQueueSubmit(m_renderqueue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_renderqueue);

    vkFreeCommandBuffers(m_device, m_cmdpool, 1, &cbuff);

    return VK_SUCCESS;
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

VkResult Renderer::copy_image(VkImage src, VkImage dst, uint32_t w, uint32_t h)
{
    VkCommandBuffer cbuff;
    VkResult result = begin_single_time_commands(&cbuff);
    if (result) {
        return result;
    }

    VkImageSubresourceLayers sub = {};
    sub.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    sub.baseArrayLayer = 0;
    sub.mipLevel = 0;
    sub.layerCount = 1;

    VkImageCopy region = {};
    region.srcSubresource = sub;
    region.dstSubresource = sub;
    region.srcOffset = {0, 0, 0};
    region.dstOffset = {0, 0, 0};
    region.extent.width = w;
    region.extent.height = h;
    region.extent.depth = 1;

    vkCmdCopyImage(cbuff, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    result = end_single_time_commands(cbuff);
    if (result) {
        return result;
    }

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

VkResult Renderer::create_depthresources(void)
{
    VkResult result = VK_SUCCESS;

    VkFormat format;
    result = find_depth_format(&format);
    if (result) {
        return result;
    }

    create_image(m_swapchain.extent.width, m_swapchain.extent.height,
      format, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      &m_texture.depth_image, &m_texture.depth_memory);
    create_imageview(m_texture.depth_image, format,
      VK_IMAGE_ASPECT_DEPTH_BIT, &m_texture.depth_view);
    transition_image_layout(m_texture.depth_image, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    return result;
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

    VkDescriptorImageInfo ii = {};
    ii.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    ii.imageView = m_texture.view;
    ii.sampler = m_texture.sampler;

    std::array<VkWriteDescriptorSet, 2> dw = {};
    dw[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    dw[0].dstSet = m_box.dset;
    dw[0].dstBinding = 0;
    dw[0].dstArrayElement = 0;
    dw[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dw[0].descriptorCount = 1;
    dw[0].pBufferInfo = &bi;
    dw[0].pImageInfo = nullptr;
    dw[0].pTexelBufferView = nullptr;

    dw[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    dw[1].dstSet = m_box.dset;
    dw[1].dstBinding = 1;
    dw[1].dstArrayElement = 0;
    dw[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    dw[1].descriptorCount = 1;
    dw[1].pImageInfo = &ii;

    vkUpdateDescriptorSets(m_device, dw.size(), dw.data(), 0, nullptr);

    return result;
}

VkResult Renderer::create_descriptorpool(void)
{
    VkResult result = VK_SUCCESS;

    std::array<VkDescriptorPoolSize, 2> pool_sizes = {};
    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[0].descriptorCount = 1;
    pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_sizes[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolinfo = {};
    poolinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolinfo.poolSizeCount = pool_sizes.size();
    poolinfo.pPoolSizes = pool_sizes.data();
    poolinfo.maxSets = 1;

    result = vkCreateDescriptorPool(m_device, &poolinfo, nullptr, &m_box.dpool);

    return result;
}

VkResult Renderer::create_descriptorset_layout(void)
{
    VkResult result = VK_SUCCESS;

    VkDescriptorSetLayoutBinding sampler_layout_binding = {};
    sampler_layout_binding.binding = 1;
    sampler_layout_binding.descriptorCount = 1;
    sampler_layout_binding.descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler_layout_binding.pImmutableSamplers = nullptr;
    sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding ubo_layout_binding = {};
    ubo_layout_binding.binding = 0;
    ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_layout_binding.descriptorCount = 1;
    ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    ubo_layout_binding.pImmutableSamplers = nullptr;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings =
      { ubo_layout_binding, sampler_layout_binding };

    VkDescriptorSetLayoutCreateInfo li = {};
    li.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    li.bindingCount = bindings.size();
    li.pBindings = bindings.data();

    result = vkCreateDescriptorSetLayout(m_device, &li, nullptr,
      &m_box.dslayout);
    if (result) {
        return result;
    }

    return result;
}

VkResult Renderer::create_sampler(void)
{
    VkSamplerCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    ci.magFilter = VK_FILTER_LINEAR;
    ci.minFilter = VK_FILTER_LINEAR;
    ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    ci.anisotropyEnable = VK_TRUE;
    ci.maxAnisotropy = 16;
    ci.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    ci.unnormalizedCoordinates = VK_FALSE;
    ci.compareEnable = VK_FALSE;
    ci.compareOp = VK_COMPARE_OP_ALWAYS;
    ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    ci.mipLodBias = 0.0f;
    ci.minLod = 0.0f;
    ci.maxLod = 0.0f;

    return vkCreateSampler(m_device, &ci, nullptr, &m_texture.sampler);
}

VkResult Renderer::create_texture(void)
{
    VkResult result = VK_SUCCESS;
    STBImage img;

    if (!ReadImage(&img, "./textures/bitcoin.png")) {
        return VK_ERROR_FEATURE_NOT_PRESENT;
    }

    VkDeviceSize img_size = img.width * img.height * 4;
    VkImage staging_image;
    VkDeviceMemory staging_memory;

    result = create_image(img.width, img.height, VK_FORMAT_R8G8B8A8_UNORM,
      VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_image, &staging_memory);
    if (result) {
        return result;
    }

    void* data = nullptr;
    vkMapMemory(m_device, staging_memory, 0, img_size, 0, &data);
    std::memcpy(data, img.data, (size_t)img_size);
    vkUnmapMemory(m_device, staging_memory);

    result = create_image(img.width, img.height, VK_FORMAT_R8G8B8A8_UNORM,
      VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT |
      VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      &m_texture.image, &m_texture.memory);
    if (result) {
        return result;
    }

    result = transition_image_layout(staging_image,
      VK_IMAGE_LAYOUT_PREINITIALIZED,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    if (result) {
        return result;
    }

    result = transition_image_layout(m_texture.image,
      VK_IMAGE_LAYOUT_PREINITIALIZED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    if (result) {
        return result;
    }

    result = copy_image(staging_image, m_texture.image, img.width, img.height);
    if (result) {
        return result;
    }

    result = transition_image_layout(m_texture.image,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    if (result) {
        return result;
    }

    ReleaseImage(&img);
    vkDestroyImage(m_device, staging_image, nullptr);
    vkFreeMemory(m_device, staging_memory, nullptr);

    return result;
}

VkResult Renderer::create_imageview(VkImage image, VkFormat format,
  VkImageAspectFlags aflags, VkImageView* view)
{
    VkImageViewCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ci.image = image;
    ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ci.format = format;
    ci.subresourceRange.aspectMask = aflags;
    ci.subresourceRange.baseMipLevel = 0;
    ci.subresourceRange.levelCount = 1;
    ci.subresourceRange.baseArrayLayer = 0;
    ci.subresourceRange.layerCount = 1;

    return vkCreateImageView(m_device, &ci, nullptr, view);
}

VkResult Renderer::create_textureimageview(void)
{
    VkResult result = VK_SUCCESS;

    result = create_imageview(m_texture.image, VK_FORMAT_R8G8B8A8_UNORM,
      VK_IMAGE_ASPECT_COLOR_BIT, &m_texture.view);
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
       {{-0.5f, -0.5f,  0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
       {{ 0.5f, -0.5f,  0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
       {{ 0.5f,  0.5f,  0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
       {{-0.5f,  0.5f,  0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

       {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
       {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
       {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
       {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}

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

VkResult Renderer::create_image(uint32_t w, uint32_t h, VkFormat fmt,
  VkImageTiling tiling, VkImageUsageFlags usage,
  VkMemoryPropertyFlags properties, VkImage* image, VkDeviceMemory* mem)
{
    VkResult result = VK_SUCCESS;

    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = w;
    image_info.extent.height = h;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = fmt;
    image_info.tiling = tiling;
    image_info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    image_info.usage = usage;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    result = vkCreateImage(m_device, &image_info, nullptr, image);
    if (result) {
        return result;
    }

    VkMemoryRequirements mem_req;
    vkGetImageMemoryRequirements(m_device, *image, &mem_req);

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_req.size;
    alloc_info.memoryTypeIndex = find_memory_type(mem_req.memoryTypeBits,
      properties);

    result = vkAllocateMemory(m_device, &alloc_info, nullptr, mem);
    if (result) {
        return result;
    }

    vkBindImageMemory(m_device, *image, *mem, 0);
    return VK_SUCCESS;
}

VkResult Renderer::create_indexbuffer(void)
{
    VkResult result = VK_SUCCESS;

    m_box.indices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4
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

VkResult Renderer::find_depth_format(VkFormat* format)
{
    std::vector<VkFormat> candidates = {
        VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };

    return find_supported_format(m_gpu.device, candidates,
      VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
      format);
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

VkResult Renderer::find_supported_format(VkPhysicalDevice gpu,
  std::vector<VkFormat> candidates, VkImageTiling tiling,
  VkFormatFeatureFlags features, VkFormat* out)
{
    bool found = false;
    for (uint32_t i = 0; i < candidates.size(); i++) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(gpu, candidates[i], &props);

        if (tiling == VK_IMAGE_TILING_LINEAR &&
          (props.linearTilingFeatures & features) == features) {
            *out = candidates[i];
            found = true;
            break;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
          (props.optimalTilingFeatures & features) == features) {
            *out = candidates[i];
            found = true;
            break;
        }
    }

    if (!found) {
        return VK_ERROR_FEATURE_NOT_PRESENT;
    }

    return VK_SUCCESS;
}

VkResult Renderer::transition_image_layout(VkImage img, VkImageLayout old,
  VkImageLayout _new)
{
    VkCommandBuffer cbuff;
    begin_single_time_commands(&cbuff);

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old;
    barrier.newLayout = _new;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = img;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    if (_new == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    } else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    if (old == VK_IMAGE_LAYOUT_PREINITIALIZED &&
      _new == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    } else if (old == VK_IMAGE_LAYOUT_PREINITIALIZED &&
      _new == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    } else if (old == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
      _new == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    } else if (old == VK_IMAGE_LAYOUT_UNDEFINED &&
      _new == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    } else {
        return VK_ERROR_FEATURE_NOT_PRESENT;
    }

    vkCmdPipelineBarrier(cbuff, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
      0, nullptr, 0, nullptr, 1, &barrier);
    end_single_time_commands(cbuff);

    return VK_SUCCESS;
}

