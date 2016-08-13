#ifndef VKTEST_GLOBAL_H
#define VKTEST_GLOBAL_H

#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#if defined(__linux__)
  #define VK_USE_PLATFORM_XCB_KHR
#elif defined(_WIN32)
  #define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <SDL2/SDL.h>

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texcoord;

    static VkVertexInputBindingDescription getBindDesc(void)
    {
        VkVertexInputBindingDescription desc = {};
        desc.binding = 0;
        desc.stride = sizeof(Vertex);
        desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return desc;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttrDesc(void)
    {
        std::array<VkVertexInputAttributeDescription, 3> descs = {};

        /* must be the position attribute description. */
        descs[0].binding = 0;
        descs[0].location = 0;
        descs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        descs[0].offset = offsetof(Vertex, pos);

        /* and the color attribute descritpion */
        descs[1].binding = 0;
        descs[1].location = 1;
        descs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        descs[1].offset = offsetof(Vertex, color);

        descs[2].binding = 0;
        descs[2].location = 2;
        descs[2].format = VK_FORMAT_R32G32_SFLOAT;
        descs[2].offset = offsetof(Vertex, texcoord);

        return descs;
    }
};

struct STBImage {
    int width, height, comp;
    unsigned char* data;
};

void Assert(VkResult test, std::string message, SDL_Window* win = nullptr);
void Info(std::string message, SDL_Window* win = nullptr);
std::vector<char> ReadFile(std::string path);

/* stb_image wrappers */
bool ReadImage(struct STBImage* out, std::string path);
void ReleaseImage(struct STBImage* img);

#endif // VKTEST_GLOBAL_H
