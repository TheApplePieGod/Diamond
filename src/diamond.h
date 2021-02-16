#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <optional>

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

private:
    void ConfigureValidationLayers();
    void CreateGraphicsPipeline();
    void CreateRenderPass();
    VkPipelineShaderStageCreateInfo CreateShaderStage(VkShaderModule shaderModule, VkShaderStageFlagBits stage, const char* entrypoint = "main");
    VkShaderModule CreateShaderModule(const char* ShaderPath);
    queue_family_indices GetQueueFamilies(VkPhysicalDevice device);
    swap_chain_support_details GetSwapChainSupport(VkPhysicalDevice device);
    bool IsDeviceSuitable(VkPhysicalDevice device);
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

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
    VkPipelineLayout pipelineLayout;
    VkRenderPass renderPass;
    VkPipeline graphicsPipeline;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers = {};
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;

    // todo: move to struct
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImage> swapChainImages = {};
    std::vector<VkImageView> swapChainImageViews = {};
    std::vector<VkFramebuffer> swapChainFrameBuffers = {};
};