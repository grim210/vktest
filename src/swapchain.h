#ifndef VKTEST_SWAPCHAIN_H
#define VKTEST_SWAPCHAIN_H

#include <vector>
#include <vulkan/vulkan.h>

#include "global.h"

class Swapchain {
public:
    static Swapchain* Init(VkSurfaceKHR surface, VkDevice device,
      VkPhysicalDevice gpu);
    static void Release(VkDevice device, Swapchain* swapchain);

    VkResult CreateImageViews(VkDevice device, uint32_t* count);

    VkResult GetExtent(VkExtent2D* out);
    VkResult GetFormat(VkFormat* out);
    VkResult GetHandle(VkSwapchainKHR* out);
    VkResult GetImageCount(uint32_t* out);
    VkResult GetImageView(uint32_t index, VkImageView* out);

private:
    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_views;

    VkColorSpaceKHR m_colorspace;
    VkExtent2D m_extent;
    VkFormat m_format;
    VkPresentModeKHR m_presentmode;
    VkSwapchainKHR m_swapchain;

    void release(VkDevice device);
};

#endif // VKTEST_SWAPCHAIN_H
