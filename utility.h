#ifndef VKTEST_UTILITY_H
#define VKTEST_UTILITY_H

#include <cstring>  // memcpy
#include <vector>

#include <vulkan/vulkan.h>

#include "global.h"

class Utility {
public:
    /*
    * There are quite a few circumstances in which you wish to do a single
    * piece of work with a command buffer.  These two helper functions provide
    * all the necessary boilerplate to make that process a little easier.
    * vkQueueWaitIdle is called on the provided queue in the end command.
    */
    static VkResult BufferSingleUseBegin(VkDevice device, VkCommandPool pool,
      VkCommandBuffer* buffer);
    static VkResult BufferSingleUseEnd(VkDevice device, VkQueue queue,
      VkCommandPool pool, VkCommandBuffer buffer);

    /*
    * Pretty easy to understand, I think.  Copies from src, to dest.  It needs
    * to create a command buffer, and submit that command, so the command
    * pool and VkQueue objects are required.  Cleans up after itself too..
    */
    static VkResult CopyBuffer(VkDevice device, VkBuffer src, VkBuffer dst,
      VkDeviceSize size, VkCommandPool pool, VkQueue queue);
    static VkResult CopyImage(VkDevice device, VkImage src, VkImage dst,
      VkExtent2D extent, VkCommandPool pool, VkQueue queue);
};

#endif /* VKTEST_UTILITY_H */
