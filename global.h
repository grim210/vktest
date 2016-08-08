#ifndef VKTEST_GLOBAL_H
#define VKTEST_GLOBAL_H

#include <array>
#include <fstream>
#include <vector>
#include <sstream>
#include <string>

#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <SDL2/SDL.h>

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindDesc(void)
    {
        VkVertexInputBindingDescription desc = {};
        desc.binding = 0;
        desc.stride = sizeof(Vertex);
        desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return desc;
    }

    static std::array<VkVertexInputAttributeDescription, 2> getAttrDesc(void)
    {
        std::array<VkVertexInputAttributeDescription, 2> descs = {};

        /* must be the position attribute description. */
        descs[0].binding = 0;
        descs[0].location = 0;
        descs[0].format = VK_FORMAT_R32G32_SFLOAT;
        descs[0].offset = offsetof(Vertex, pos);

        /* and the color attribute descritpion */
        descs[1].binding = 0;
        descs[1].location = 1;
        descs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        descs[1].offset = offsetof(Vertex, color);

        return descs;
    }
};

const std::vector<Vertex> vertices = {
    {{ 0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{ 0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}}
};

void Assert(VkResult test, std::string message, SDL_Window* win = nullptr);
void Info(std::string message, SDL_Window* win = nullptr);
std::vector<char> ReadFile(std::string path);

#endif // VKTEST_GLOBAL_H
