#include "box.h"

Box* Box::Init(VkDevice device, VkCommandPool pool, VkQueue queue)
{
    Box* box = new Box();

    box->m_vertices = {
       {{-0.5f, -0.5f,  0.25f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
       {{ 0.5f, -0.5f,  0.25f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
       {{ 0.5f,  0.5f,  0.25f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
       {{-0.5f,  0.5f,  0.25f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

       {{-0.5f, -0.5f, -0.25f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
       {{ 0.5f, -0.5f, -0.25f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
       {{ 0.5f,  0.5f, -0.25f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
       {{-0.5f,  0.5f, -0.25f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
    };

    /*
    * This is where I'll make the box.  If I can sit two boxes on top
    * of each other, I should be able to form the cube.
    */
    box->m_indices = {
        /* top */
        0, 1, 2,
        2, 3, 0,

        /* bottom */
        4, 5, 6,
        6, 7, 4
    };

    /*
    * Filling the vertex buffer.
    */
    VkDeviceSize buffersize = (sizeof(m_vertices[0]) * m_vertices.size());
    VkBuffer stagingbuffer;
    VkDeviceMemory stagingbuffermemory;

    /* First, the staging buffer is created and filled */
    result = create_buffer(buffersize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingbuffer,
      &stagingbuffermemory);
    if (result) {
        return nullptr;
    }

    void* data = nullptr;
    vkMapMemory(device, stagingbuffermemory, 0, buffersize, 0, &data);
    std::memcpy(data, m_vertices.data(), (size_t)buffersize);
    vkUnmapMemory(device, stagingbuffermemory);

    /* Then, the actual device memory buffer is created. */
    result = create_buffer(buffersize, VK_BUFFER_USAGE_TRANSFER_DST_BIT |
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      &m_vbuffer, &m_vbuffermem);
    if (result) {
        return nullptr;
    }

    result = Utility::CopyBuffer(device, stagingbuffer, m_vbuffer,
      buffersize, pool, queue);
    if (result) {
        return nullptr;
    }

    /* Finally, free those temporary staging resources. */
    vkFreeMemory(device, stagingbuffermemory, nullptr);
    vkDestroyBuffer(device, stagingbuffer, nullptr);

    return box;
}

void Box::Release(Box* box)
{
    delete(box);
}

/* Inherited from GameObject */
void Box::Draw(void)
{

}

/* Inherited from GameObject */
void Box::Rebuild(void)
{

}

/* Inherited from GameObject */
void Box::Update(double elapsed)
{

}

