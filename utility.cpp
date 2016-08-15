#include "utility.h"

VkResult Utility::BufferSingleUseBegin(VkDevice device, VkCommandPool pool,
  VkCommandBuffer* buffer)
{
    VkResult result = VK_SUCCESS;

    VkCommandBufferAllocateInfo ai = {};
    ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandPool = pool;
    ai.commandBufferCount = 1;

    result = vkAllocateCommandBuffers(device, &ai, buffer);
    if (result) {
        Log::Write(Log::SEVERE, "Call to vkAllocateCommandBuffers in "
          "Utility::BufferSingleUseBegin failed.");
        return result;
    }

    VkCommandBufferBeginInfo bi = {};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result = vkBeginCommandBuffer(*buffer, &bi);
    if (result) {
        Log::Write(Log::SEVERE, "Call to vkBeginCommandBuffer in "
          "Utility::BufferSingleUseBegin failed.");
        return result;
    }

    return VK_SUCCESS;
}

VkResult Utility::BufferSingleUseEnd(VkDevice device, VkQueue queue,
  VkCommandPool pool, VkCommandBuffer buffer)
{
    VkResult result = VK_SUCCESS;

    result = vkEndCommandBuffer(buffer);
    if (result) {
        Log::Write(Log::SEVERE, "Call to vkEndCommandBuffer in "
          "Utility::BufferSingleUseEnd failed.");
        return result;
    }

    VkSubmitInfo si = {};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &buffer;

    result = vkQueueSubmit(queue, 1, &si, VK_NULL_HANDLE);
    if (result) {
        Log::Write(Log::SEVERE, "Call to vkQueueSubmit in "
          "Utility::BufferSingleUseEnd failed.");
        return result;
    }

    result = vkQueueWaitIdle(queue);
    if (result) {
        Log::Write(Log::SEVERE, "Call to vkQueueWaitIdle in "
          "Utility::BufferSingleUseEnd failed.");
        return result;
    }

    vkFreeCommandBuffers(device, pool, 1, &buffer);

    return VK_SUCCESS;
}

VkResult Utility::CopyBuffer(VkDevice device, VkBuffer src, VkBuffer dst,
  VkDeviceSize size, VkCommandPool pool, VkQueue queue)
{
    VkResult result = VK_SUCCESS;

    VkCommandBuffer buff;
    result = Utility::BufferSingleUseBegin(device, pool, &buff);
    if (result) {
        Log::Write(Log::SEVERE, "Utility::CopyBuffer -> Call to "
          "Utility::BufferSingleUseBegin failed.");
        return result;
    }

    VkBufferCopy region = {};
    region.srcOffset = 0;
    region.dstOffset = 0;
    region.size = size;

    vkCmdCopyBuffer(buff, src, dst, 1, &region);

    result = Utility::BufferSingleUseEnd(device, queue, pool, buff);
    if (result) {
        Log::Write(Log::SEVERE, "Utility::CopyBuffer -> Call to "
          "Utility::BufferSingleUseEnd failed.");
        return result;
    }

    return VK_SUCCESS;
}

VkResult Utility::CopyImage(VkDevice device, VkImage src, VkImage dst,
  VkExtent2D extent, VkCommandPool pool, VkQueue queue)
{
    VkCommandBuffer cbuff;
    VkResult result = Utility::BufferSingleUseBegin(device, pool, &cbuff);
    if (result) {
        Log::Write(Log::SEVERE, "Utility::CopyImage -> call to "
          "Utility::BufferSingleUseBegin failed.");
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
    region.extent.width = extent.width;
    region.extent.height = extent.height;
    region.extent.depth = 1;

    vkCmdCopyImage(cbuff, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    result = Utility::BufferSingleUseEnd(device, queue, pool, cbuff);
    if (result) {
        Log::Write(Log::SEVERE, "Utility::CopyImage -> Call to "
          "Utility::BufferSingleUseEnd failed.");
        return result;
    }

    return VK_SUCCESS;
}

