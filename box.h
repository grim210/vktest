#ifndef VKTEST_BOX_H
#define VKTEST_BOX_H

#include <vulkan/vulkan.h>

#include "gameobject.h"
#include "global.h"

class Box : public GameObject {
public:
    virtual ~Box(void) { };
    static Box* Init(void);
    static void Release(Box* box);

    void Draw(void);
    void Rebuild(void);
    void Update(double elapsed);
private:
    std::vector<Vertex> m_vertices;
    std::vector<uint16_t> m_indices;

    VkDescriptorPool m_dpool;
    VkDescriptorSet m_dset;
    VkDescriptorSetLayout m_dslayout;

    /* Vertex buffer */
    VkBuffer m_vbuffer;
    VkDeviceMemory m_vbuffermem;

    /* Index Buffer */
    VkBuffer m_ibuffer;
    VkDeviceMemory m_ibuffermem;

    /* Uniform Buffer */
    VkBuffer m_ubuffer;
    VkDeviceMemory m_ubuffermem;

    /* Uniform Staging Buffer */
    VkBuffer m_usbuffer;
    VkDeviceMemory m_usbuffermem;

    /* Texture data */
    VkImage m_image;
    VkImageView m_view;
    VkDeviceMemory m_texmem;
    VkSampler m_sampler;
};

#endif /* VKTEST_BOX_H */
