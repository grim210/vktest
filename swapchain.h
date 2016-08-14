#ifndef VKTEST_SWAPCHAIN_H
#define VKTEST_SWAPCHAIN_H

#include <vector>

#include <vulkan/vulkan.h>

class Swapchain {
public:
    static Swapchain* Init(VkSurfaceKHR surface, VkDevice device,
      VkPhysicalDevice gpu);
    static void Release(Swapchain* swapchain, VkDevice device);
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
