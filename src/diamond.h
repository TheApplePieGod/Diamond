#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <array>
#include <optional>
#include <vec2.hpp>
#include <vec3.hpp>

struct vertex
{
    glm::vec2 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription GetBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(vertex, pos);
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(vertex, color);
        
        return attributeDescriptions;
    }
};

struct queue_family_indices
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

struct swap_chain_support_details
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class diamond
{
public:
    void Initialize(int width, int height, const char* name);
    void BeginFrame();
    void EndFrame();
    void Cleanup();

    bool IsRunning();

    inline GLFWwindow* Window() { return window; };

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
    void Present();
    VkPipelineShaderStageCreateInfo CreateShaderStage(VkShaderModule shaderModule, VkShaderStageFlagBits stage, const char* entrypoint = "main");
    VkShaderModule CreateShaderModule(const char* ShaderPath);
    queue_family_indices GetQueueFamilies(VkPhysicalDevice device);
    swap_chain_support_details GetSwapChainSupport(VkPhysicalDevice device);
    bool IsDeviceSuitable(VkPhysicalDevice device);
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void MapMemory(void* data, uint32_t dataSize, uint32_t elementCount, VkDeviceMemory bufferMemory);
    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    GLFWwindow* window;

    std::vector<const char*> validationLayers = {};
    std::vector<const char*> deviceExtensions = {};
    const int MAX_FRAMES_IN_FLIGHT = 2;
    int currentFrameIndex = 0;

    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice logicalDevice = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers = {};
    VkCommandBuffer renderPassBuffer = {};
    std::vector<VkSemaphore> imageAvailableSemaphores = {};
    std::vector<VkSemaphore> renderFinishedSemaphores = {};
    std::vector<VkFence> inFlightFences = {};
    std::vector<VkFence> imagesInFlight = {};

    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

    // todo: move to struct
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImage> swapChainImages = {};
    std::vector<VkImageView> swapChainImageViews = {};
    std::vector<VkFramebuffer> swapChainFrameBuffers = {};
};