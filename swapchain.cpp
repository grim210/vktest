#include "swapchain.h"

Swapchain* Swapchain::Init(VkSurfaceKHR surface, VkDevice device,
  VkPhysicalDevice gpu)
{
    VkResult result = VK_SUCCESS;
    uint32_t count = 0;

    /*
    * Ask Vulkan to take a look at the surface we've created with our GPU
    * and determine what sort of formats are supported for creation of our
    * swapchain.  The VkSurfaceFormatKHR structure contains a lot of info
    * that will be used to fill our swapchain creation structures.
    */
    std::vector<VkSurfaceFormatKHR> surface_formats;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface,
      &count, nullptr);
    if (result) {
        return nullptr;
    }

    surface_formats.resize(count);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface,
      &count, surface_formats.data());
    if (result) {
        return nullptr;
    }

    Swapchain* swapchain = new Swapchain();
    if (swapchain == nullptr) {
        return nullptr;
    }

    /*
    * This conditional is checking to see if the surface format is undefined.
    * If so, we set it to something easy (B8B8R8).  If the graphics card has
    * a preference, obviosly use that one.
    */
    swapchain->m_colorspace = surface_formats[0].colorSpace;
    if (count == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED) {
        swapchain->m_format = VK_FORMAT_B8G8R8_UNORM;
    } else {
        swapchain->m_format = surface_formats[0].format;
    }

    VkSurfaceCapabilitiesKHR sc = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &sc);

    /*
    * Here, our goal is 2 images.  One to be rendering to while the other
    * one is being seen by the user.  If that's not possible, we can go
    * down to 1 image, or higher if the minimum number of images is greater
    * than two.  The first conditional will decrease us, while the second
    * one will increase our images used for presentation.
    */
    uint32_t image_count = 2;
    if (image_count < sc.minImageCount) {
        image_count = sc.minImageCount;
    } else if (sc.maxImageCount != 0 && image_count > sc.maxImageCount) {
        image_count = sc.maxImageCount;
    }

    /*
    * The currentExtent field of the SurfaceCapabilities structure holds
    * the current size of the VkSurfaceKHR passed into the function.  The
    * spec seems unclear about the condition that that extent would be
    * filled with UINT32_MAX, but my graphics card hasn't hit that condition
    * that I'm aware of.  So throw an error.  Maybe someone can answer that
    * question for me down the line.
    */
    if (sc.currentExtent.width == UINT32_MAX) {
        delete(swapchain);
        return nullptr;
    } else {
        swapchain->m_extent = sc.currentExtent;
    }

    /*
    * The transformation of the swapchain is determined by the value
    * retrieved from the surface capabilities.  The transformation, from
    * what I'm able to discern, comes from whether the physical display is
    * rotated in a way that the swapchain would have to use the device to
    * rotate/flip/etc the image that is being presented.
    */
    VkSurfaceTransformFlagBitsKHR pre_transform = sc.currentTransform;
    if (sc.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }

    /*
    * Here, we're determining exactly what method we're going to be using
    * to display images.  The only present mode guaranteed to exist by the
    * specification is VK_PRESENT_MODE_FIFO_KHR.  We're wanting to avoid
    * tearing, so we're going shoot for VK_PRSENT_MODE_MAILBOX_KHR.  The
    * mailbox mode of rendering waits for vertical blanking, and is also
    * done with a single entry queue.  Translates to double-buffering.
    */
    count = 0;
    std::vector<VkPresentModeKHR> present_modes;
    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &count,
      nullptr);

    present_modes.resize(count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &count,
      present_modes.data());

    swapchain->m_presentmode = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32_t i = 0; i < present_modes.size(); i++) {
        if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            swapchain->m_presentmode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }

    VkSwapchainCreateInfoKHR sci = {};
    sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sci.pNext = VK_NULL_HANDLE;
    sci.flags = 0;
    sci.surface = surface;
    sci.minImageCount = image_count;
    sci.imageFormat = swapchain->m_format;
    sci.imageColorSpace = swapchain->m_colorspace;
    sci.imageExtent = swapchain->m_extent;
    sci.imageArrayLayers = 1;
    sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sci.queueFamilyIndexCount = 0;
    sci.pQueueFamilyIndices = VK_NULL_HANDLE;
    sci.preTransform = pre_transform;
    sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sci.presentMode = swapchain->m_presentmode;
    sci.clipped = VK_TRUE;
    sci.oldSwapchain = VK_NULL_HANDLE;

    result = vkCreateSwapchainKHR(device, &sci, nullptr,
      &swapchain->m_swapchain);
    if (result) {
        delete(swapchain);
        return nullptr;
    }

    count = 0;
    result = vkGetSwapchainImagesKHR(device, swapchain->m_swapchain,
      &count, nullptr);
    if (result) {
        vkDestroySwapchainKHR(device, swapchain->m_swapchain, nullptr);
        delete(swapchain);
        return nullptr;
    }

    swapchain->m_images.resize(count);
    result = vkGetSwapchainImagesKHR(device, swapchain->m_swapchain, &count,
      swapchain->m_images.data());

    swapchain->m_views.resize(count);
    for (uint32_t i = 0; i < count; i++) {
        VkImageViewCreateInfo view_create_info = {};
        view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_create_info.image = swapchain->m_images[i];
        view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_create_info.format = swapchain->m_format;
        view_create_info.subresourceRange.aspectMask =
          VK_IMAGE_ASPECT_COLOR_BIT;
        view_create_info.subresourceRange.baseMipLevel = 0;
        view_create_info.subresourceRange.levelCount = 1;
        view_create_info.subresourceRange.baseArrayLayer = 0;
        view_create_info.subresourceRange.layerCount = 1;
        result = vkCreateImageView(device, &view_create_info, nullptr,
          &swapchain->m_views[i]);
        if (result) {
            vkDestroySwapchainKHR(device, swapchain->m_swapchain, nullptr);
        }
    }

    return swapchain;
}

void Swapchain::Release(Swapchain* swapchain, VkDevice device)
{
    swapchain->release(device);
    delete(swapchain);
}

void Swapchain::release(VkDevice device)
{
    vkDeviceWaitIdle(device);

    for (uint32_t i = 0; i < m_views.size(); i++) {
        vkDestroyImageView(device, m_views[i], nullptr);
    }

    vkDestroySwapchainKHR(device, m_swapchain, nullptr);
}
