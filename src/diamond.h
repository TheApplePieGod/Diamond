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
    Perspective = 0,
    Orthographic = 1
};

struct diamond_transform
{
    glm::vec2 location = { 0.f, 0.f };
    float rotation = 0.f;
    glm::vec2 scale = { 1.f, 1.f };

    inline bool operator==(const diamond_transform& other)
    {
        return (
            location == other.location &&
            rotation == other.rotation &&
            scale == other.scale
        );
    }
};

struct diamond_texture
{
    VkImage image;
    VkDeviceMemory memory;
    VkImageView imageView;
    uint32_t id;
};

struct diamond_object_data
{
    glm::mat4 model;
    int textureIndex;
};

struct diamond_vertex
{
    glm::vec2 pos;
    glm::vec4 color;
    glm::vec2 texCoord;
    int textureIndex;

    static VkVertexInputBindingDescription GetBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(diamond_vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

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

struct diamond_frame_buffer_object
{
    glm::mat4 viewProj;
};

struct diamond_swap_chain_info
{
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImage> swapChainImages = {};
    std::vector<VkImageView> swapChainImageViews = {};
    std::vector<VkFramebuffer> swapChainFrameBuffers = {};
};

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

struct diamond_swap_chain_support_details
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class diamond
{
public:
    void Initialize(int width, int height, const char* name);
    void BeginFrame(diamond_camera_mode CameraMode, glm::vec2 CameraPos);
    void EndFrame();
    void Cleanup();

    uint32_t RegisterTexture(const char* filePath);
    uint32_t RegisterTexture(void* data, int width, int height);
    //uint32_t DeleteTexture(uint32_t textureId);
    void SyncTextureUpdates();

    void BindVertices(const diamond_vertex* vertices, uint32_t vertexCount);
    void BindVertices(diamond_vertex* vertices, uint32_t vertexCount);
    void BindIndices(const uint16_t* indices, uint32_t indexCount);
    void BindIndices(uint16_t* indices, uint32_t indexCount);

    void Draw(uint32_t vertexCount, int textureIndex, diamond_transform objectTransform);
    void DrawIndexed(uint32_t indexCount, uint32_t vertexCount, int textureIndex, diamond_transform objectTransform);
    void DrawQuad(int textureIndex, diamond_transform quadTransform, glm::vec4 color = glm::vec4(1.f));
    void DrawQuadsTransform(int* textureIndexes, diamond_transform* quadTransforms, int quadCount, glm::vec4* colors = nullptr);
    void DrawQuadsOffsetScale(int* textureIndexes, glm::vec4* offsetScales, int quadCount, glm::vec4* colors = nullptr);

    bool IsRunning();

    inline GLFWwindow* Window() { return window; };
    inline std::tuple<VkInstance, VkPhysicalDevice, VkDevice> VulkanComponents() { return std::make_tuple(instance, physicalDevice, logicalDevice); };
    inline diamond_swap_chain_info VulkanSwapChain() { return swapChain; };

private:
    void ConfigureValidationLayers();
    void CreateGraphicsPipeline();
    void CreateRenderPass();
    void CreateSwapChain();
    void CreateFrameBuffers();
    void CreateCommandBuffers();
    void RecreateSwapChain();
    void CleanupSwapChain();
    void CreateVertexBuffer();
    void CreateIndexBuffer();
    void CreateDescriptorSetLayout();
    void CreateUniformBuffers();
    void UpdatePerFrameBuffer(uint32_t imageIndex);
    void CreateDescriptorPool();
    void CreateDescriptorSets();
    void CreateTextureSampler();
    void CreateColorResources();
    void Present();
    VkPipelineShaderStageCreateInfo CreateShaderStage(VkShaderModule shaderModule, VkShaderStageFlagBits stage, const char* entrypoint = "main");
    VkShaderModule CreateShaderModule(const char* ShaderPath);
    diamond_queue_family_indices GetQueueFamilies(VkPhysicalDevice device);
    diamond_swap_chain_support_details GetSwapChainSupport(VkPhysicalDevice device);
    bool IsDeviceSuitable(VkPhysicalDevice device);
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void MapMemory(void* data, uint32_t dataSize, uint32_t elementCount, VkDeviceMemory bufferMemory, uint32_t elementMemoryOffset);
    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, uint32_t width, uint32_t height);
    VkImageView CreateTextureImage(const char* imagePath, VkImage& image, VkDeviceMemory& imageMemory);
    VkImageView CreateTextureImage(void* data, VkImage& image, VkDeviceMemory& imageMemory, int width, int height);
    void CreateImage(uint32_t width, uint32_t height, VkFormat format, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
    void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    VkImageView CreateImageView(VkImage image, VkFormat format, uint32_t mipLevels);
    glm::mat4 GenerateModelMatrix(diamond_transform objectTransform);
    VkSampleCountFlagBits GetMaxSampleCount();

    GLFWwindow* window;

    std::vector<const char*> validationLayers = {};
    std::vector<const char*> deviceExtensions = {};
    const int MAX_FRAMES_IN_FLIGHT = 2;
    int currentFrameIndex = 0;
    uint32_t boundIndexCount = 0;
    uint32_t boundVertexCount = 0;
    uint32_t nextImageIndex = 0;
    bool shouldPresent = true;
    diamond_camera_mode CameraMode = diamond_camera_mode::Orthographic;
    glm::vec2 CameraPosition = { 0.f, 0.f };

    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice logicalDevice = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers = {};
    VkCommandBuffer renderPassBuffer = {};
    std::vector<VkSemaphore> imageAvailableSemaphores = {};
    std::vector<VkSemaphore> renderFinishedSemaphores = {};
    std::vector<VkFence> inFlightFences = {};
    std::vector<VkFence> imagesInFlight = {};
    std::vector<VkDescriptorSet> descriptorSets;
    VkSampler textureSampler;
    std::vector<diamond_texture> textureArray = {};
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkImage colorImage;
    VkDeviceMemory colorImageMemory;
    VkImageView colorImageView;

    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;

    diamond_swap_chain_info swapChain;
};