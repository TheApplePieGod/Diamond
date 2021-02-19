#include "diamond.h"
#include "util/defs.h"
#include <iostream>
#include <fstream>
#include <set>
#include <chrono>
#include <gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

bool framebufferResized = false;

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    framebufferResized = true;
}

void diamond::Initialize(int width, int height, const char* windowName, const char* vertShaderPath, const char* fragShaderPath)
{
    #if DIAMOND_DEBUG
        std::cerr << "Initializing diamond in debug mode" << std::endl;
    #else
        std::cerr << "Initializing diamond in release mode" << std::endl;
    #endif

    Assert(vertShaderPath != "");
    Assert(fragShaderPath != "");
    defaultVertexShader = vertShaderPath;
    defaultFragmentShader = fragShaderPath;

    // init glfw & create window
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        window = glfwCreateWindow(width, height, windowName, nullptr, nullptr);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }
    // ------------------------

    // create vulkan instance
    {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = windowName;
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Diamond";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_1;

        // get api extension support
        u32 supportedExtensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionCount, nullptr);
        std::vector<VkExtensionProperties> supportedExtensions(supportedExtensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionCount, supportedExtensions.data());

        // get required extensions
        u32 glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        // check for compatability
        for (int i = 0; i < glfwExtensionCount; i++)
        {
            Assert(std::find_if(supportedExtensions.begin(), supportedExtensions.end(), [&glfwExtensions, &i](const VkExtensionProperties& arg) { return strcmp(arg.extensionName, glfwExtensions[i]); }) != supportedExtensions.end());
        }

        // create a new extensions array to add debug item
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        // finally create instance
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        #if DIAMOND_DEBUG
            ConfigureValidationLayers();
            createInfo.enabledLayerCount = static_cast<u32>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        #else
            createInfo.enabledLayerCount = 0;
        #endif
        createInfo.enabledExtensionCount = static_cast<u32>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
        Assert(result == VK_SUCCESS);
    }
    // ------------------------

#if DIAMOND_DEBUG
    // setup debug messenger
    {
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr; // Optional

        // locate extension creation function and run
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        Assert(func != nullptr)
        VkResult result = func(instance, &createInfo, nullptr, &debugMessenger);
        Assert(result == VK_SUCCESS);
    }   
    // ------------------------
#endif

    // create window surface
    {
        VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
        Assert(result == VK_SUCCESS);
    }
    // ------------------------

    // setup physical device
    {
        // add required device extensions
        deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        deviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);

        // get devices
        u32 deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        Assert(deviceCount != 0);
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        // find first compatible device
        for (const auto& device: devices)
        {
            if (IsDeviceSuitable(device))
            {
                physicalDevice = device;
                msaaSamples = GetMaxSampleCount();
                break;
            }
        }
        Assert(physicalDevice != VK_NULL_HANDLE);
    }
    // ------------------------

    // setup logical device & queues
    {
        diamond_queue_family_indices indices = GetQueueFamilies(physicalDevice);
        float queuePriority = 1.0f;

        // info for creating queues
        std::set<u32> uniqueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        for (u32 family : uniqueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = family;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        // enabled device features
        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.samplerAnisotropy = VK_TRUE;
        deviceFeatures.shaderSampledImageArrayDynamicIndexing = VK_TRUE;
        
        VkPhysicalDeviceDescriptorIndexingFeaturesEXT indexingFeatures{};
        indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
        indexingFeatures.pNext = nullptr;
        indexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
        indexingFeatures.runtimeDescriptorArray = VK_TRUE;

        // finally create device
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pNext = &indexingFeatures;
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = static_cast<u32>(queueCreateInfos.size());;
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = static_cast<u32>(deviceExtensions.size());;
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();
        #if DIAMOND_DEBUG
            createInfo.enabledLayerCount = static_cast<u32>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        #else
            createInfo.enabledLayerCount = 0;
        #endif

        VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice);
        Assert(result == VK_SUCCESS);

        vkGetDeviceQueue(logicalDevice, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(logicalDevice, indices.presentFamily.value(), 0, &presentQueue);
    }
    // ------------------------

    // create command pool
    {
        diamond_queue_family_indices indices = GetQueueFamilies(physicalDevice);
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = indices.graphicsFamily.value();
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // allow resetting

        VkResult result = vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &commandPool);
        Assert(result == VK_SUCCESS);
    }
    // ------------------------

    RegisterTexture("../images/default-texture.png");

    CreateSwapChain();

    // setup rest of pipeline
    {
        CreateRenderPass();
        CreateDescriptorSetLayout();
        CreateGraphicsPipeline(defaultVertexShader, defaultFragmentShader);
        CreateColorResources();
    }
    // ------------------------

    CreateFrameBuffers();

    CreateVertexBuffer();
    //textureImageView = CreateTextureImage("../images/test.png", textureImage, textureImageMemory);
    //textureImageView2 = CreateTextureImage("../images/test2.png", textureImage2, textureImageMemory2);
    CreateTextureSampler();
    CreateIndexBuffer();
    CreateUniformBuffers();
    CreateDescriptorPool();
    CreateDescriptorSets();

    CreateCommandBuffers();

    // create rendering secondary command buffer
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    allocInfo.commandBufferCount = 1;

    VkResult result = vkAllocateCommandBuffers(logicalDevice, &allocInfo, &renderPassBuffer);
    Assert(result == VK_SUCCESS);

    // create presenting semaphores & fences
    {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        imagesInFlight.resize(swapChain.swapChainImages.size(), VK_NULL_HANDLE);
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            VkResult result = vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]);
            Assert(result == VK_SUCCESS);
            result = vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]);
            Assert(result == VK_SUCCESS);
            result = vkCreateFence(logicalDevice, &fenceInfo, nullptr, &inFlightFences[i]);
            Assert(result == VK_SUCCESS);
        }
    }
    // ------------------------
}

void diamond::CreateSwapChain()
{
    diamond_swap_chain_support_details swapChainSupport = GetSwapChainSupport(physicalDevice);
    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

    u32 imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        imageCount = swapChainSupport.capabilities.maxImageCount;

    diamond_queue_family_indices indices = GetQueueFamilies(physicalDevice);
    u32 queueIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    swapChain.swapChainImageFormat = surfaceFormat.format;
    swapChain.swapChainExtent = extent;

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (indices.graphicsFamily != indices.presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    VkResult result = vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapChain.swapChain);
    Assert(result == VK_SUCCESS);

    vkGetSwapchainImagesKHR(logicalDevice, swapChain.swapChain, &imageCount, nullptr);
    swapChain.swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(logicalDevice, swapChain.swapChain, &imageCount, swapChain.swapChainImages.data());

    swapChain.swapChainImageViews.resize(swapChain.swapChainImages.size());
    for (int i = 0; i < swapChain.swapChainImages.size(); i++)
    {
        swapChain.swapChainImageViews[i] = CreateImageView(swapChain.swapChainImages[i], swapChain.swapChainImageFormat, 1);
    }
}

u32 diamond::RegisterTexture(const char* filePath)
{
    diamond_texture newTex{};
    newTex.imageView = CreateTextureImage(filePath, newTex.image, newTex.memory);
    newTex.id = static_cast<u32>(textureArray.size());
    textureArray.push_back(newTex);
    return newTex.id;
}

u32 diamond::RegisterTexture(void* data, int width, int height)
{
    diamond_texture newTex{};
    newTex.imageView = CreateTextureImage(data, newTex.image, newTex.memory, width, height);
    newTex.id = static_cast<u32>(textureArray.size());
    textureArray.push_back(newTex);
    return newTex.id;
}

void diamond::SyncTextureUpdates()
{
    // cleanup old resources
    vkDestroyDescriptorPool(logicalDevice, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(logicalDevice, descriptorSetLayout, nullptr);
    vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);

    // recreate bindings
    CreateDescriptorSetLayout();
    CreateGraphicsPipeline(defaultVertexShader, defaultFragmentShader);
    CreateDescriptorPool();
    CreateDescriptorSets();
}

void diamond::UpdateShaders(const char* vertShaderPath, const char* fragShaderPath)
{
    vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr); // todo: move this out of pipeline
    CreateGraphicsPipeline(vertShaderPath, fragShaderPath);
}

void diamond::BindVertices(const diamond_vertex* vertices, u32 vertexCount)
{
    BindVertices(const_cast<diamond_vertex*>(vertices), vertexCount);
}

void diamond::BindVertices(diamond_vertex* vertices, u32 vertexCount)
{
    MapMemory((void*)vertices, sizeof(diamond_vertex), vertexCount, vertexBufferMemory, boundVertexCount);
    boundVertexCount += vertexCount;
}

void diamond::BindIndices(const u16* indices, u32 indexCount)
{
    BindIndices(const_cast<u16*>(indices), indexCount);
}

void diamond::BindIndices(u16* indices, u32 indexCount)
{
    MapMemory((u16*)indices, sizeof(u16), indexCount, indexBufferMemory, boundIndexCount);
    boundIndexCount += indexCount;
}

void diamond::Draw(u32 vertexCount, int textureIndex, diamond_transform objectTransform)
{
    diamond_object_data data;
    data.textureIndex = textureIndex;
    data.model = GenerateModelMatrix(objectTransform);

    vkCmdPushConstants(renderPassBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(diamond_object_data), &data);
    vkCmdDraw(renderPassBuffer, vertexCount, 1, boundVertexCount - vertexCount, 0);
}

void diamond::DrawIndexed(u32 indexCount, u32 vertexCount, int textureIndex, diamond_transform objectTransform)
{
    diamond_object_data data;
    data.textureIndex = textureIndex;
    data.model = GenerateModelMatrix(objectTransform);

    vkCmdPushConstants(renderPassBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(diamond_object_data), &data);
    vkCmdDrawIndexed(renderPassBuffer, indexCount, 1, boundIndexCount - indexCount, boundVertexCount - vertexCount, 0);
}

// todo: bake quad vertex & index data?
void diamond::DrawQuad(int textureIndex, diamond_transform quadTransform, glm::vec4 color)
{
    const diamond_vertex vertices[] =
    {
        {{-0.5f, -0.5f}, color, {0.0f, 1.0f}, -1},
        {{0.5f, -0.5f}, color, {1.0f, 1.0f}, -1},
        {{0.5f, 0.5f}, color, {1.0f, 0.0f}, -1},
        {{-0.5f, 0.5f}, color, {0.0f, 0.0f}, -1}
    };
    const u16 indices[] =
    {
        0, 3, 2, 2, 1, 0
    };

    BindVertices(vertices, 4);
    BindIndices(indices, 6);
    DrawIndexed(6, 4, textureIndex, quadTransform);
}

void diamond::DrawQuadsTransform(int* textureIndexes, diamond_transform* quadTransforms, int quadCount, glm::vec4* colors)
{
    std::vector<diamond_vertex> vertices(quadCount * 4);
    std::vector<u16> indices(quadCount * 6);

    const u16 baseIndices[] =
    {
        0, 3, 2, 2, 1, 0
    };

    for (int i = 0; i < quadCount; i++)
    {
        int vertexIndex = 4 * i;
        int indicesIndex = 6 * i;

        glm::mat4 modelMatrix = GenerateModelMatrix(quadTransforms[i]);
        glm::vec4 color = { 1.f, 1.f, 1.f, 1.f };
        if (colors != nullptr)
            color = colors[i];

        vertices[vertexIndex] =     { modelMatrix * glm::vec4(-0.5f, -0.5f, 0.f, 1.f), color, {0.0f, 1.0f}, textureIndexes[i]};
        vertices[vertexIndex + 1] = { modelMatrix * glm::vec4(0.5f, -0.5f, 0.f, 1.f), color, {1.0f, 1.0f}, textureIndexes[i]};
        vertices[vertexIndex + 2] = { modelMatrix * glm::vec4(0.5f, 0.5f, 0.f, 1.f), color, {1.0f, 0.0f}, textureIndexes[i]};
        vertices[vertexIndex + 3] = { modelMatrix * glm::vec4(-0.5f, 0.5f, 0.f, 1.f), color, {0.0f, 0.0f}, textureIndexes[i]};
        indices[indicesIndex] =     baseIndices[0] + vertexIndex;
        indices[indicesIndex + 1] = baseIndices[1] + vertexIndex;
        indices[indicesIndex + 2] = baseIndices[2] + vertexIndex;
        indices[indicesIndex + 3] = baseIndices[3] + vertexIndex;
        indices[indicesIndex + 4] = baseIndices[4] + vertexIndex;
        indices[indicesIndex + 5] = baseIndices[5] + vertexIndex;
    }

    diamond_transform baseTransform = diamond_transform();

    BindVertices(vertices.data(), static_cast<u32>(vertices.size()));
    BindIndices(indices.data(), static_cast<u32>(indices.size()));
    DrawIndexed(static_cast<u32>(indices.size()), static_cast<u32>(vertices.size()), -1, baseTransform);
}

void diamond::DrawQuadsOffsetScale(int* textureIndexes, glm::vec4* offsetScales, int quadCount, glm::vec4* colors)
{
    std::vector<diamond_vertex> vertices(quadCount * 4);
    std::vector<u16> indices(quadCount * 6);

    const u16 baseIndices[] =
    {
        0, 3, 2, 2, 1, 0
    };

    for (int i = 0; i < quadCount; i++)
    {
        int vertexIndex = 4 * i;
        int indicesIndex = 6 * i;

        glm::vec4 color = { 1.f, 1.f, 1.f, 1.f };
        if (colors != nullptr)
            color = colors[i];

        vertices[vertexIndex] =     { {(-0.5f * offsetScales[i].z) + offsetScales[i].x, (-0.5f * offsetScales[i].w) + offsetScales[i].y}, color, {0.0f, 1.0f}, textureIndexes[i] };
        vertices[vertexIndex + 1] = { {(0.5f * offsetScales[i].z) + offsetScales[i].x, (-0.5f * offsetScales[i].w) + offsetScales[i].y}, color, {1.0f, 1.0f}, textureIndexes[i] };
        vertices[vertexIndex + 2] = { {(0.5f * offsetScales[i].z) + offsetScales[i].x, (0.5f * offsetScales[i].w) + offsetScales[i].y}, color, {1.0f, 0.0f}, textureIndexes[i] };
        vertices[vertexIndex + 3] = { {(-0.5f * offsetScales[i].z) + offsetScales[i].x, (0.5f * offsetScales[i].w) + offsetScales[i].y}, color, {0.0f, 0.0f}, textureIndexes[i] };
        indices[indicesIndex] =     baseIndices[0] + vertexIndex;
        indices[indicesIndex + 1] = baseIndices[1] + vertexIndex;
        indices[indicesIndex + 2] = baseIndices[2] + vertexIndex;
        indices[indicesIndex + 3] = baseIndices[3] + vertexIndex;
        indices[indicesIndex + 4] = baseIndices[4] + vertexIndex;
        indices[indicesIndex + 5] = baseIndices[5] + vertexIndex;
    }

    diamond_transform baseTransform = diamond_transform();

    BindVertices(vertices.data(), static_cast<u32>(vertices.size()));
    BindIndices(indices.data(), static_cast<u32>(indices.size()));
    DrawIndexed(static_cast<u32>(indices.size()), static_cast<u32>(vertices.size()), -1, baseTransform);
}

glm::mat4 diamond::GenerateModelMatrix(diamond_transform objectTransform)
{
    glm::mat4 model = glm::translate(glm::mat4(1.f), glm::vec3(objectTransform.location, 0.f));
    model = model * glm::scale(glm::mat4(1.f), glm::vec3(objectTransform.scale, 1.f));
    model = model * glm::rotate(glm::mat4(1.f), glm::radians(objectTransform.rotation), glm::vec3(0.0f, 0.0f, -1.0f));
    return model;
}

void diamond::CreateFrameBuffers()
{
    swapChain.swapChainFrameBuffers.resize(swapChain.swapChainImageViews.size());
    for (int i = 0; i < swapChain.swapChainImageViews.size(); i++)
    {
        std::array<VkImageView, 2> attachments = { colorImageView, swapChain.swapChainImageViews[i] };
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<u32>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChain.swapChainExtent.width;
        framebufferInfo.height = swapChain.swapChainExtent.height;
        framebufferInfo.layers = 1;
        
        VkResult result = vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &swapChain.swapChainFrameBuffers[i]);
        Assert(result == VK_SUCCESS);
    }
}

void diamond::CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width, height, 1 };

    vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    EndSingleTimeCommands(commandBuffer);
}

void diamond::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    /*
    Undefined → transfer destination: transfer writes that don't need to wait on anything
    Transfer destination → shader reading: shader reads should wait on transfer writes, specifically the shader reads in the fragment shader, because that's where we're going to use the texture
    */
    
    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
        throw std::invalid_argument("Unsupported layout transition");

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    EndSingleTimeCommands(commandBuffer);
}

void diamond::CreateImage(uint32_t width, uint32_t height, VkFormat format, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<u32>(width);
    imageInfo.extent.height = static_cast<u32>(height);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = numSamples;
    imageInfo.flags = 0; // Optional

    VkResult result = vkCreateImage(logicalDevice, &imageInfo, nullptr, &image);
    Assert(result == VK_SUCCESS);

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(logicalDevice, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    result = vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &imageMemory);
    Assert(result == VK_SUCCESS);

    vkBindImageMemory(logicalDevice, image, imageMemory, 0);
}

VkImageView diamond::CreateTextureImage(const char* imagePath, VkImage& image, VkDeviceMemory& imageMemory)
{
    int width, height, channels;
    stbi_uc* pixels = stbi_load(imagePath, &width, &height, &channels, STBI_rgb_alpha);
    Assert(pixels != nullptr)

    VkImageView view = CreateTextureImage((void*)pixels, image, imageMemory, width, height);

    stbi_image_free(pixels);
    return view;
}

VkImageView diamond::CreateTextureImage(void* data, VkImage& image, VkDeviceMemory& imageMemory, int width, int height)
{
    VkDeviceSize imageSize = width * height * 4;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    MapMemory(data, sizeof(stbi_uc), imageSize, stagingBufferMemory, 0);

    CreateImage(width, height, VK_FORMAT_R8G8B8A8_SRGB, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory);

    TransitionImageLayout(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    CopyBufferToImage(stagingBuffer, image, static_cast<u32>(width), static_cast<u32>(height));

    TransitionImageLayout(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);

    return CreateImageView(image, VK_FORMAT_R8G8B8A8_SRGB, 1);
}

void diamond::CreateTextureSampler()
{
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    VkResult result = vkCreateSampler(logicalDevice, &samplerInfo, nullptr, &textureSampler);
    Assert(result == VK_SUCCESS)
}

void diamond::CreateColorResources()
{
    VkFormat colorFormat = swapChain.swapChainImageFormat;

    CreateImage(swapChain.swapChainExtent.width, swapChain.swapChainExtent.height, colorFormat, 1, msaaSamples, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage, colorImageMemory);
    colorImageView = CreateImageView(colorImage, colorFormat, 1);
}

VkImageView diamond::CreateImageView(VkImage image, VkFormat format, u32 mipLevels)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView view;
    VkResult result = vkCreateImageView(logicalDevice, &viewInfo, nullptr, &view);
    Assert(result == VK_SUCCESS);

    return view;
}

void diamond::CreateCommandBuffers()
{
    commandBuffers.resize(swapChain.swapChainFrameBuffers.size());
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (u32)commandBuffers.size();

    VkResult result = vkAllocateCommandBuffers(logicalDevice, &allocInfo, commandBuffers.data());
    Assert(result == VK_SUCCESS);
}

void diamond::UpdatePerFrameBuffer(u32 imageIndex)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    glm::mat4 view = glm::lookAt(glm::vec3(cameraPosition.x, cameraPosition.y, 5.f), glm::vec3(cameraPosition.x, cameraPosition.y, 0.f), glm::vec3(0.f, 1.f, 0.f));
    //view = view * glm::rotate(glm::mat4(1.f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    glm::mat4 proj;
    if (cameraMode == diamond_camera_mode::Perspective)
        proj = glm::perspective(glm::radians(75.f), swapChain.swapChainExtent.width / (f32) swapChain.swapChainExtent.height, 0.1f, 10.f);
    else
        proj = glm::transpose(glm::ortho(-0.5f * swapChain.swapChainExtent.width, 0.5f * swapChain.swapChainExtent.width, 0.5f * swapChain.swapChainExtent.height, -0.5f * swapChain.swapChainExtent.height, 0.1f, 50.f));

    diamond_frame_buffer_object fbo{};
    fbo.viewProj = proj * view;

    MapMemory(&fbo, sizeof(diamond_frame_buffer_object), 1, uniformBuffersMemory[imageIndex], 0);
}

void diamond::CreateDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<u32>(swapChain.swapChainImages.size());
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<u32>(swapChain.swapChainImages.size() * textureArray.size());

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<u32>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<u32>(swapChain.swapChainImages.size());
    poolInfo.flags = 0;

    VkResult result = vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &descriptorPool);
    Assert(result == VK_SUCCESS);
}

void diamond::CreateDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts(swapChain.swapChainImages.size(), descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<u32>(swapChain.swapChainImages.size());
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(swapChain.swapChainImages.size());
    VkResult result = vkAllocateDescriptorSets(logicalDevice, &allocInfo, descriptorSets.data());
    Assert(result == VK_SUCCESS);

    for (int i = 0; i < swapChain.swapChainImages.size(); i++)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(diamond_frame_buffer_object);

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        std::vector<VkDescriptorImageInfo> images(textureArray.size());
        for (int i = 0; i < textureArray.size(); i++)
        {
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = textureArray[i].imageView;
            imageInfo.sampler = textureSampler;

            images[i] = imageInfo;
        }

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = static_cast<u32>(images.size());
        descriptorWrites[1].pImageInfo = images.data();

        vkUpdateDescriptorSets(logicalDevice, static_cast<u32>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

void diamond::CleanupSwapChain()
{
    for (int i = 0; i < swapChain.swapChainFrameBuffers.size(); i++)
    {
        vkDestroyFramebuffer(logicalDevice, swapChain.swapChainFrameBuffers[i], nullptr);
    }

    vkFreeCommandBuffers(logicalDevice, commandPool, static_cast<u32>(commandBuffers.size()), commandBuffers.data());
    vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
    vkDestroyRenderPass(logicalDevice, renderPass, nullptr);

    vkDestroyImageView(logicalDevice, colorImageView, nullptr);
    vkDestroyImage(logicalDevice, colorImage, nullptr);
    vkFreeMemory(logicalDevice, colorImageMemory, nullptr);

    for (int i = 0; i < swapChain.swapChainImages.size(); i++)
    {
        vkDestroyBuffer(logicalDevice, uniformBuffers[i], nullptr);
        vkFreeMemory(logicalDevice, uniformBuffersMemory[i], nullptr);
    }

    vkDestroyDescriptorPool(logicalDevice, descriptorPool, nullptr);

    for (int i = 0; i < swapChain.swapChainImageViews.size(); i++)
    {
        vkDestroyImageView(logicalDevice, swapChain.swapChainImageViews[i], nullptr);
    }

    vkDestroySwapchainKHR(logicalDevice, swapChain.swapChain, nullptr);
}

void diamond::RecreateSwapChain()
{
    #if DIAMOND_DEBUG
    std::cerr << "Recreating swap chain" << std::endl;
    #endif

    // pause application on minimize (change later?)
    int width, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(logicalDevice);

    // cleanup
    CleanupSwapChain();

    // recreate
    CreateSwapChain();
    CreateRenderPass();
    CreateGraphicsPipeline(defaultVertexShader, defaultFragmentShader);
    CreateColorResources();
    CreateFrameBuffers();
    CreateUniformBuffers();
    CreateDescriptorPool();
    CreateDescriptorSets();
    CreateCommandBuffers();
}

diamond_swap_chain_support_details diamond::GetSwapChainSupport(VkPhysicalDevice device)
{
    diamond_swap_chain_support_details details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
    
    u32 formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    u32 presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

diamond_queue_family_indices diamond::GetQueueFamilies(VkPhysicalDevice device)
{
    diamond_queue_family_indices indices;

    u32 supportedQueueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &supportedQueueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> supportedQueueFamilies(supportedQueueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &supportedQueueFamilyCount, supportedQueueFamilies.data());

    int i = 0;
    for (const auto& family : supportedQueueFamilies)
    {
        if (indices.IsComplete())
            break;

        if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            indices.graphicsFamily = i;

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport)
            indices.presentFamily = i;
        
        i++;
    }

    return indices;
}

u32 diamond::FindMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (u32 i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }

    throw std::runtime_error("Failed to find suitable memory type");
}

VkCommandBuffer diamond::BeginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    VkResult result = vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer);
    Assert(result == VK_SUCCESS);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    Assert(result == VK_SUCCESS);

    return commandBuffer;
}

void diamond::EndSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    VkResult result = vkEndCommandBuffer(commandBuffer);
    Assert(result == VK_SUCCESS);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    Assert(result == VK_SUCCESS);

    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
}

void diamond::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    EndSingleTimeCommands(commandBuffer);
}

void diamond::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size; // really shouldnt have more vertices than this
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &buffer);
    Assert(result == VK_SUCCESS);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(logicalDevice, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);
    
    result = vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &bufferMemory);
    Assert(result == VK_SUCCESS);

    result = vkBindBufferMemory(logicalDevice, buffer, bufferMemory, 0);
    Assert(result == VK_SUCCESS);
}

void diamond::CreateVertexBuffer()
{
    VkDeviceSize bufferSize = sizeof(diamond_vertex) * 5000;
    CreateBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertexBuffer, vertexBufferMemory);
}

void diamond::CreateIndexBuffer()
{
    VkDeviceSize bufferSize = sizeof(u16) * 10000;
    CreateBuffer(bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, indexBuffer, indexBufferMemory);
}

void diamond::CreateGraphicsPipeline(const char* vertShaderPath, const char* fragShaderPath)
{
    VkShaderModule vertShader = CreateShaderModule(vertShaderPath);
    VkShaderModule fragShader = CreateShaderModule(fragShaderPath);

    VkPipelineShaderStageCreateInfo shaderStages[] = { CreateShaderStage(vertShader, VK_SHADER_STAGE_VERTEX_BIT), CreateShaderStage(fragShader, VK_SHADER_STAGE_FRAGMENT_BIT) };

    auto bindingDescription = diamond_vertex::GetBindingDescription();
    auto attributeDescriptions = diamond_vertex::GetAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<u32>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swapChain.swapChainExtent.width;
    viewport.height = (float) swapChain.swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChain.swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = msaaSamples;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_LINE_WIDTH
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkPushConstantRange pushConstants{};
    pushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstants.offset = 0;
    pushConstants.size = sizeof(diamond_object_data);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstants;
    VkResult result = vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout);
    Assert(result == VK_SUCCESS);

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr; // Optional
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional
    result = vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline);
    Assert(result == VK_SUCCESS);

    vkDestroyShaderModule(logicalDevice, vertShader, nullptr);
    vkDestroyShaderModule(logicalDevice, fragShader, nullptr);
}

void diamond::CreateDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = static_cast<u32>(textureArray.size());
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.pImmutableSamplers = nullptr;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<u32>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    VkResult result = vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &descriptorSetLayout);
    Assert(result == VK_SUCCESS);
}

void diamond::CreateRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChain.swapChainImageFormat;
    colorAttachment.samples = msaaSamples;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachmentResolve{};
    colorAttachmentResolve.format = swapChain.swapChainImageFormat;
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentResolveRef{};
    colorAttachmentResolveRef.attachment = 1;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pResolveAttachments = &colorAttachmentResolveRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, colorAttachmentResolve };
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<u32>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkResult result = vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass);
    Assert(result == VK_SUCCESS);
}

void diamond::CreateUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(diamond_frame_buffer_object);
    
    uniformBuffers.resize(swapChain.swapChainImages.size());
    uniformBuffersMemory.resize(swapChain.swapChainImages.size());

    for (int i = 0; i < swapChain.swapChainImages.size(); i++)
    {
        CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
    }
}

VkPipelineShaderStageCreateInfo diamond::CreateShaderStage(VkShaderModule shaderModule, VkShaderStageFlagBits stage, const char* entrypoint)
{
    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage = stage;
    shaderStageInfo.module = shaderModule;
    shaderStageInfo.pName = entrypoint;
    shaderStageInfo.pSpecializationInfo = nullptr;

    return shaderStageInfo;
}

VkShaderModule diamond::CreateShaderModule(const char* ShaderPath)
{
    std::ifstream file(ShaderPath, std::ios::ate | std::ios::binary);
    Assert(file.is_open())
    
    u64 fileSize = file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = buffer.size();
    createInfo.pCode = reinterpret_cast<const u32*>(buffer.data());

    VkShaderModule module;
    VkResult result = vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &module);
    Assert(result == VK_SUCCESS);

    return module;
}

bool diamond::CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
    // get hardware extension support
    u32 supportedExtensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &supportedExtensionCount, nullptr);
    std::vector<VkExtensionProperties> supportedExtensions(supportedExtensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &supportedExtensionCount, supportedExtensions.data());

    // check for compatability
    int count = 0;
    for (int i = 0; i < deviceExtensions.size(); i++)
    {
        if (std::find_if(supportedExtensions.begin(), supportedExtensions.end(), [this, &i](const VkExtensionProperties& arg) { return strcmp(arg.extensionName, deviceExtensions[i]); }) == supportedExtensions.end())
            return false;
    }

    return true;
}

VkSampleCountFlagBits diamond::GetMaxSampleCount()
{
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;

    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}

bool diamond::IsDeviceSuitable(VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
    diamond_queue_family_indices indices = GetQueueFamilies(device);

    VkPhysicalDeviceDescriptorIndexingFeaturesEXT indexingFeatures{};
    indexingFeatures.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
    indexingFeatures.pNext = nullptr;
    VkPhysicalDeviceFeatures2 deviceFeatures2{};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.pNext = &indexingFeatures;
    vkGetPhysicalDeviceFeatures2(device, &deviceFeatures2);


    bool extensionsSupported = CheckDeviceExtensionSupport(device);
    bool swapChainAdequate = false;
    if (extensionsSupported)
    {
        diamond_swap_chain_support_details swapDetails = GetSwapChainSupport(device);
        swapChainAdequate = !swapDetails.formats.empty() && !swapDetails.presentModes.empty();
    }

    return (
        indices.IsComplete() &&
        extensionsSupported &&
        swapChainAdequate &&
        deviceFeatures.samplerAnisotropy &&
        deviceFeatures.shaderSampledImageArrayDynamicIndexing &&
        indexingFeatures.descriptorBindingPartiallyBound &&
        indexingFeatures.runtimeDescriptorArray
    );
}

VkSurfaceFormatKHR diamond::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
    for (const auto& availableFormat : formats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return availableFormat;
    }

    return formats[0];
}

VkPresentModeKHR diamond::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes)
{
    for (const auto& availablePresentMode : presentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            return availablePresentMode;
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D diamond::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != UINT32_MAX)
        return capabilities.currentExtent;
    else
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        VkExtent2D actualExtent = {
            static_cast<u32>(width),
            static_cast<u32>(height)
        };

        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

void diamond::MapMemory(void* data, u32 dataSize, u32 elementCount, VkDeviceMemory bufferMemory, u32 elementMemoryOffset)
{
    void* buffer;
    vkMapMemory(logicalDevice, bufferMemory, elementMemoryOffset * dataSize, dataSize * elementCount, 0, &buffer);
    memcpy(buffer, data, dataSize * elementCount);
    vkUnmapMemory(logicalDevice, bufferMemory);
}

void diamond::BeginFrame(diamond_camera_mode camMode, glm::vec2 camPos)
{
    cameraMode = camMode;
    cameraPosition = camPos;

    glfwPollEvents();

    VkResult result = vkAcquireNextImageKHR(logicalDevice, swapChain.swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrameIndex], VK_NULL_HANDLE, &nextImageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
        shouldPresent = false;
    else
    {
        Assert(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR);
        shouldPresent = true;
    }

    // start recording the render command buffer
    VkCommandBufferInheritanceInfo inheritanceInfo{};
    inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    inheritanceInfo.pNext = nullptr;
    inheritanceInfo.renderPass = renderPass;
    inheritanceInfo.subpass = 0;
    inheritanceInfo.framebuffer = VK_NULL_HANDLE;

    VkCommandBufferBeginInfo secondaryBeginInfo{};
    secondaryBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    secondaryBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    secondaryBeginInfo.pInheritanceInfo = &inheritanceInfo;

    result = vkBeginCommandBuffer(renderPassBuffer, &secondaryBeginInfo);
    Assert(result == VK_SUCCESS);

    VkBuffer vertexBuffers[] = { vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(renderPassBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(renderPassBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdBindDescriptorSets(renderPassBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[nextImageIndex], 0, nullptr);
    vkCmdBindPipeline(renderPassBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    boundVertexCount = 0;
    boundIndexCount = 0;
}

void diamond::EndFrame()
{
    VkResult result = vkEndCommandBuffer(renderPassBuffer);
    Assert(result == VK_SUCCESS);

    // start command buffers and render recorded renderBuffer
    for (int i = 0; i < commandBuffers.size(); i++)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        result = vkBeginCommandBuffer(commandBuffers[i], &beginInfo);
        Assert(result == VK_SUCCESS);

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChain.swapChainFrameBuffers[i];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChain.swapChainExtent;

        VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
        
        //vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);

        vkCmdExecuteCommands(commandBuffers[i], 1, &renderPassBuffer);

        vkCmdEndRenderPass(commandBuffers[i]);
        
        result = vkEndCommandBuffer(commandBuffers[i]);
        Assert(result == VK_SUCCESS);
    }

    if (shouldPresent)
        Present();
    else
        RecreateSwapChain();
    currentFrameIndex = (currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

void diamond::Present()
{
    vkWaitForFences(logicalDevice, 1, &inFlightFences[currentFrameIndex], VK_TRUE, UINT64_MAX);

    // check if a previous frame is using this image (i.e. there is its fence to wait on)
    if (imagesInFlight[nextImageIndex] != VK_NULL_HANDLE)
        vkWaitForFences(logicalDevice, 1, &imagesInFlight[nextImageIndex], VK_TRUE, UINT64_MAX);

    UpdatePerFrameBuffer(nextImageIndex);

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrameIndex] };
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[nextImageIndex];

    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrameIndex] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(logicalDevice, 1, &inFlightFences[currentFrameIndex]);
    VkResult result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrameIndex]);
    Assert(result == VK_SUCCESS);

    VkSwapchainKHR swapChains[] = { swapChain.swapChain };
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &nextImageIndex;
    presentInfo.pResults = nullptr; // Optional

    result = vkQueuePresentKHR(presentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
    {
        framebufferResized = false;
        RecreateSwapChain();
    }
    else
        Assert(result == VK_SUCCESS);

    vkQueueWaitIdle(presentQueue);
}

void diamond::Cleanup()
{
    vkDeviceWaitIdle(logicalDevice);

    CleanupSwapChain();

    vkDestroySampler(logicalDevice, textureSampler, nullptr);

    for (int i = 0; i < textureArray.size(); i++)
    {
        vkDestroyImageView(logicalDevice, textureArray[i].imageView, nullptr);
        vkDestroyImage(logicalDevice, textureArray[i].image, nullptr);
        vkFreeMemory(logicalDevice, textureArray[i].memory, nullptr);
    }

    vkDestroyBuffer(logicalDevice, vertexBuffer, nullptr);
    vkFreeMemory(logicalDevice, vertexBufferMemory, nullptr);
    vkDestroyBuffer(logicalDevice, indexBuffer, nullptr);
    vkFreeMemory(logicalDevice, indexBufferMemory, nullptr);

    vkDestroyDescriptorSetLayout(logicalDevice, descriptorSetLayout, nullptr);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(logicalDevice, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(logicalDevice, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(logicalDevice, inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyDevice(logicalDevice, nullptr);

#if DIAMOND_DEBUG
    // find func and destroy debug instance
    auto destroyDebugUtilsFunc = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    Assert(destroyDebugUtilsFunc != nullptr);
    destroyDebugUtilsFunc(instance, debugMessenger, nullptr);
#endif

    vkDestroyInstance(instance, nullptr);

    // glfw cleanup
    glfwDestroyWindow(window);
    glfwTerminate();
}

bool diamond::IsRunning()
{
    return !glfwWindowShouldClose(window);
}

void diamond::ConfigureValidationLayers()
{
    validationLayers.push_back("VK_LAYER_KHRONOS_validation");

    u32 supportedLayerCount;
    vkEnumerateInstanceLayerProperties(&supportedLayerCount, nullptr);
    std::vector<VkLayerProperties> supportedLayers(supportedLayerCount);
    vkEnumerateInstanceLayerProperties(&supportedLayerCount, supportedLayers.data());

    // check for compatability
    for (int i = 0; i < validationLayers.size(); i++)
    {
        Assert(std::find_if(supportedLayers.begin(), supportedLayers.end(), [this, &i](const VkLayerProperties& arg) { return strcmp(arg.layerName, validationLayers[i]); }) != supportedLayers.end());
    }
}