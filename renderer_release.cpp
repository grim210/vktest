#include "renderer.h"

VkResult Renderer::release_device_objects(void)
{
    vkDestroyDevice(m_device, nullptr);
    vkDestroySurfaceKHR(m_instance, m_swapchain.surface, nullptr);

    return VK_SUCCESS;
}

VkResult Renderer::release_instance_objects(void)
{
    this->release_debug();
    vkDestroyInstance(m_instance, nullptr);

    return VK_SUCCESS;
}

VkResult Renderer::release_render_objects(void)
{
    vkDestroyDescriptorSetLayout(m_device, m_box.dslayout, nullptr);
    vkDestroyDescriptorPool(m_device, m_box.dpool, nullptr);
    vkResetCommandPool(m_device, m_cmdpool,
      VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

    for (uint32_t i = 0; i < m_fbuffers.size(); i++) {
        vkDestroyFramebuffer(m_device, m_fbuffers[i], nullptr);
    }
    m_fbuffers.clear();

    vkDestroySampler(m_device, m_texture.sampler, nullptr);
    vkDestroyImageView(m_device, m_texture.view, nullptr);
    vkDestroyImage(m_device, m_texture.image, nullptr);
    vkFreeMemory(m_device, m_texture.memory, nullptr);

    vkFreeMemory(m_device, m_box.vbuffermem, nullptr);
    vkDestroyBuffer(m_device, m_box.vbuffer, nullptr);

    vkFreeMemory(m_device, m_box.ibuffermem, nullptr);
    vkDestroyBuffer(m_device, m_box.ibuffer, nullptr);

    vkFreeMemory(m_device, m_box.ubuffermem, nullptr);
    vkDestroyBuffer(m_device, m_box.ubuffer, nullptr);

    vkFreeMemory(m_device, m_box.usbuffermem, nullptr);
    vkDestroyBuffer(m_device, m_box.usbuffer, nullptr);

    vkDestroyPipeline(m_device, m_pipeline.gpipeline, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipeline.layout, nullptr);
    vkDestroyRenderPass(m_device, m_pipeline.renderpass, nullptr);
    vkDestroyShaderModule(m_device,
      m_pipeline.vshadermodule, nullptr);
    vkDestroyShaderModule(m_device,
      m_pipeline.fshadermodule, nullptr);
    for (uint32_t i = 0; i < m_swapchain.views.size(); i++) {
        vkDestroyImageView(m_device, m_swapchain.views[i], nullptr);
    }
    m_swapchain.views.clear();

    vkDestroySwapchainKHR(m_device, m_swapchain.chain, nullptr);
    vkFreeCommandBuffers(m_device, m_cmdpool,
      m_cmdbuffers.size(), m_cmdbuffers.data());
    m_cmdbuffers.clear();
    vkDestroyCommandPool(m_device, m_cmdpool, nullptr);

    return VK_SUCCESS;
}

VkResult Renderer::release_sync_objects(void)
{
    vkDestroySemaphore(m_device, m_swapchain.semready, nullptr);
    vkDestroySemaphore(m_device, m_swapchain.semfinished, nullptr);

    return VK_SUCCESS;
}
