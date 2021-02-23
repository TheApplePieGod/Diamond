#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#include <GLFW/glfw3.h>
#include <vector>
#include <array>
#include <optional>
#include <vec2.hpp>
#include <vec3.hpp>
#include <mat4x4.hpp>

enum diamond_camera_mode: uint16_t
{
    Perspective = 0, // Perspective: rendered in 3d as if it was being viewed in real life
    OrthographicViewportDependent = 1, // Orthographic: rendered as a flat image with no perspective or visual depth between objects, scale of objects do not change with viewport size
    OrthographicViewportIndependent = 2 // same as 1 except objects scale with viewport size
};

struct diamond_transform
{
    glm::vec2 location = { 0.f, 0.f }; // World absolute position
    float rotation = 0.f; // Rotation in degrees
    glm::vec2 scale = { 1.f, 1.f }; // World scale factor

    inline bool operator==(const diamond_transform& other)
    {
        return (
            location == other.location &&
            rotation == other.rotation &&
            scale == other.scale
        );
    }
};

// Internal use
struct diamond_texture
{
    VkImage image;
    VkDeviceMemory memory;
    VkImageView imageView;
    uint32_t id;
};

// Internal use
struct diamond_object_data
{
    glm::mat4 model;
    int textureIndex;
};

struct diamond_vertex
{
    glm::vec2 pos; // Object space position of the vertex
    glm::vec4 color; // Color to be either rendered by itself or applied as a hue to the texture of the vertex
    glm::vec2 texCoord; // Texture coordinates [0-1]
    int textureIndex; // Texture to be applied to this specific vertex. Useful when drawing sets of quads in one BindVertices() call. Set to -1 to only render vertex color

    // Internal use
    static VkVertexInputBindingDescription GetBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(diamond_vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    // Internal use
    static std::array<VkVertexInputAttributeDescription, 4> GetAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(diamond_vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(diamond_vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(diamond_vertex, texCoord);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32_SINT;
        attributeDescriptions[3].offset = offsetof(diamond_vertex, textureIndex);
        
        return attributeDescriptions;
    }
};

// Internal use
struct diamond_frame_buffer_object
{
    glm::mat4 viewProj;
};

// Internal use. Can be acquired through the VulkanSwapChain() command
struct diamond_swap_chain_info
{
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImage> swapChainImages = {};
    std::vector<VkImageView> swapChainImageViews = {};
    std::vector<VkFramebuffer> swapChainFrameBuffers = {};
};

// Internal use
struct diamond_queue_family_indices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    
    bool IsComplete()
    {
        return (
            graphicsFamily.has_value() &&
            presentFamily.has_value()
        );
    }
};

// Internal use
struct diamond_swap_chain_support_details
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};
