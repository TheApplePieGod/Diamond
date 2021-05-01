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
#include <chrono>

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
    VkImageLayout imageLayout;
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
    static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions()
    {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(4);
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(diamond_vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
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

struct diamond_compute_buffer_info
{
    diamond_compute_buffer_info()
    {}

    diamond_compute_buffer_info(int _size, bool _bindVertexBuffer, bool _staging)
    {
        identifier = "";
        size = _size;
        bindVertexBuffer = _bindVertexBuffer;
        staging = _staging;
    }
    
    diamond_compute_buffer_info(const char* _identifier, int _size, bool _bindVertexBuffer, bool _staging)
    {
        identifier = _identifier;
        size = _size;
        bindVertexBuffer = _bindVertexBuffer;
        staging = _staging;
    }

    diamond_compute_buffer_info(const char* _identifier)
    {
        identifier = _identifier;
    }

    const char* identifier = ""; // used when creating new pipelines which should access existing buffers
    int size = 0; // size in bytes of the buffer
    bool bindVertexBuffer = false; // enable if this buffer should be compatible as a vertex buffer
    bool staging = false; // enable if this buffer should be split into two separate buffers, one for the CPU and one for the GPU. This is helpful because the GPU optimized buffer is extremely fast and leaving this enabled is the preferred method for interfacing with data in the compute shader
};

struct diamond_compute_image_info
{
    diamond_compute_image_info()
    {}
    
    diamond_compute_image_info(int _width, int _height, int _precision)
    {
        identifier = "";
        width = _width;
        height = _height;
        precision = _precision;
    }

    diamond_compute_image_info(const char* _identifier, int _width, int _height, int _precision)
    {
        identifier = _identifier;
        width = _width;
        height = _height;
        precision = _precision;
    }

    diamond_compute_image_info(const char* _identifier)
    {
        identifier = _identifier;
    }

    const char* identifier = ""; // used when creating new pipelines which should access existing images
    int width = 0; // width of the image
    int height = 0; // height of the image
    int precision = 8; // 8 16 32 64 precision of each color value
};

// References
// https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkAccessFlagBits.html
// https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkPipelineStageFlagBits.html
// https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkMemoryBarrier.html
// https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/vkCmdPipelineBarrier.html

struct diamond_compute_pipeline_create_info
{
    diamond_compute_buffer_info* bufferInfoList = nullptr; // can be temporary; these values get copied internally
    int bufferCount = 0; // amount of elements in bufferInfoList
    diamond_compute_image_info* imageInfoList = nullptr; // can be temporary; these values get copied internally
    int imageCount = 0; // amount of elements in imageInfoList
    const char* computeShaderPath = ""; // path to the compiled .spv shader (See https://github.com/google/shaderc/tree/main/glslc for .spv shader compilation)
    const char* entryFunctionName = "main";
    uint32_t groupCountX = 1; // see https://vkguide.dev/docs/gpudriven/compute_shaders/
    uint32_t groupCountY = 1;
    uint32_t groupCountZ = 1;
    bool usePushConstants = false; // see http://web.engr.oregonstate.edu/~mjb/vulkan/Handouts/PushConstants.1pp.pdf
    int pushConstantsDataSize = 0;
};

struct diamond_graphics_pipeline_create_info
{
    const char* vertexShaderPath = ""; // path to the compiled .spv shader (See https://github.com/google/shaderc/tree/main/glslc for .spv shader compilation)
    const char* fragmentShaderPath = "";

    // these fields only need to be changed when using custom vertices
    // this feature is still pretty rudimentary, so 
    int vertexSize = sizeof(diamond_vertex);
    std::vector<VkVertexInputAttributeDescription> (*getVertexAttributeDescriptions)() = diamond_vertex::GetAttributeDescriptions;
    VkVertexInputBindingDescription (*getVertexBindingDescription)() = diamond_vertex::GetBindingDescription;
    VkPrimitiveTopology vertexTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // see https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkPrimitiveTopology.html

    bool useCustomPushConstants = false;
    int pushConstantsDataSize = 0;

    // max amounts that can be bound to each pipeline
    uint32_t maxVertexCount = 1000;
    uint32_t maxIndexCount = 2000;
};

// Internal use
struct diamond_frame_buffer_object
{
    glm::mat4 viewProj;
};

// for the examples
struct diamond_particle_vertex
{
    glm::vec2 pos; // Object space position of the vertex
    glm::vec2 padding;
    glm::vec4 color; // Color to be either rendered by itself or applied as a hue to the texture of the vertex

    // Internal use
    static VkVertexInputBindingDescription GetBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(diamond_particle_vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    // Internal use
    static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions()
    {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(diamond_particle_vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(diamond_particle_vertex, color);

        return attributeDescriptions;
    }
};

struct diamond_test_compute_buffer
{
    glm::vec2 pos[100000];
};

struct diamond_test_compute_buffer2
{
    diamond_particle_vertex vertices[100000];
};

struct diamond_test_compute_constants
{
    float zoom = 2.0;
    float offsetX = 1.0;
    float offsetY = 0.0;
};
// ----------------------

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
    std::optional<uint32_t> computeFamily;
    
    bool IsComplete()
    {
        return (
            graphicsFamily.has_value() &&
            presentFamily.has_value() &&
            computeFamily.has_value()
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

struct diamond_compute_pipeline
{
    bool enabled = true;
    std::vector<VkBuffer> buffers;
    std::vector<VkDeviceMemory> buffersMemory;
    std::vector<VkBuffer> deviceBuffers;
    std::vector<VkDeviceMemory> deviceBuffersMemory;
    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<int> textureIndexes;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    diamond_compute_pipeline_create_info pipelineInfo = {};
};

struct diamond_graphics_pipeline
{
    bool enabled = true;
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
    uint32_t boundIndexCount = 0;
    uint32_t boundVertexCount = 0;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    diamond_graphics_pipeline_create_info pipelineInfo = {};
};