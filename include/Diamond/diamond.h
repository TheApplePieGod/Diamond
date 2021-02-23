#pragma once
#include "structures.h"

// Main engine class
class diamond
{
public:

    /*
    * Initialize the diamond engine
    * 
    * Creates a window using glfw and sets up everything needed by vulkan
    * 
    * @param width Desired starting width of the window
    * @param height Desired starting height of the window
    * @param maxVertexCount Max amount of vertices rendered per frame
    * @param maxIndexCount Max amount of indices rendered per frame
    * @param windowName Desired name of the window and also name of the vulkan application
    * @param vertShaderPath Path to the initial compiled .spv vertex shader
    * @param fragShaderPath Path to the initial compiled .spv fragment shader
    * @note See https://github.com/google/shaderc/tree/main/glslc for .spv shader compilation
    */
    void Initialize(int width, int height, int maxVertexCount, int maxIndexCount, const char* windowName, const char* vertShaderPath, const char* fragShaderPath);

    /*
    * Called at the start of every frame in the game loop
    * 
    * Internal updates, binds initial pipeline values, and resets command buffers
    * 
    * @param cameraMode Specify whether to render the scene using a predefined perspective or orthographic view
    * @param cameraDimensions Dimensions to use for the projection if camera mode is orthographic frame independent
    * @param cameraViewMatrix Specify the view matrix of the camera
    * @see GenerateViewMatrix()
    */
    void BeginFrame(diamond_camera_mode cameraMode, glm::vec2 camDimensions, glm::mat4 cameraViewMatrix);

    /*
    * Called at the end of every frame in the game loop
    * 
    * Does some cleanup and also handles presenting to the swap chain image buffer
    * 
    * @param clearColor The color to clear the screen with when drawing a new frame
    */
    void EndFrame(glm::vec4 clearColor);

    /*
    * Cleanup engine resources
    * 
    * Called at the very end of the program when closing. Cleans up engine & vulkan resources.
    */
    void Cleanup();

    /*
    * Register a texture to the internal texture array
    * 
    * Any texture that needs to be used must be registered to the array. After all desired textures are registered,
    * call SyncTextureUpdates() to commit changes. All register commands return the id of the registered texture,
    * which is used later when calling functions like Draw()
    * 
    * @param filePath The filepath of the texture image to load
    * @param data The raw pixel data of the image. Data will not be freed automatically
    * @param width The width of the image
    * @param height The height of the image
    * @returns The id of the registered texture to be used later
    * @note Currently, there is no way to unregister a texture once it is registered
    * @see SyncTextureUpdates()
    */
    uint32_t RegisterTexture(const char* filePath);
    uint32_t RegisterTexture(void* data, int width, int height);

    /*
    * Apply changes made to the registered texture array
    * 
    * Call this after all registrations have been made. It can be called multiple times, but the less
    * the better
    * @see RegisterTexture()
    */
    void SyncTextureUpdates();

    /*
    * Change the currently bound shader
    * 
    * The current implementation is not optimal since it has to recreate the graphics pipeline every time, but
    * there will be a separate and more optimal implementation in the future.
    * 
    * @param vertShaderPath Path to the initial compiled .spv vertex shader
    * @param fragShaderPath Path to the initial compiled .spv fragment shader
    * @warning This is a temporary implementation and cannot be called mid frame. It must be called before BeginFrame() is called
    * @see BeginFrame()
    */
    void UpdateShaders(const char* vertShaderPath, const char* fragShaderPath);

    /*
    * Set the vertices that will be drawn in the next draw call
    * 
    * This can be called any number of times during a frame. Call it right before a draw call is made
    * and it can be called again immediately after for a different set of vertices if necessary.
    * 
    * @param vertices An array of diamond_vertex vertices
    * @param vertexCount The amount of vertices passed in the array
    * @note Implementation is limited to only use the diamond_vertex structure, but that will change in the future
    * @see Draw() DrawIndexed() diamond_vertex
    */
    void BindVertices(const diamond_vertex* vertices, uint32_t vertexCount);
    void BindVertices(diamond_vertex* vertices, uint32_t vertexCount);

    /*
    * Set the indices that will be used in the next DrawIndexed() call
    * 
    * This can be called any number of times during a frame. Call it right before a DrawIndexed() call is made
    * and it can be called again immediately after for a different set of indices if necessary.
    * 
    * @param indices An array of vertex indices
    * @param index The amount of indices passed in the array
    * @note See https://www.proofof.blog/2018/09/24/vertex-and-index-buffer.html for information on indexed drawing
    * @see Draw() DrawIndexed() diamond_vertex
    */
    void BindIndices(const uint16_t* indices, uint32_t indexCount);
    void BindIndices(uint16_t* indices, uint32_t indexCount);

    /*
    * Draw the currently bound vertices to the screen
    * 
    * This can be called any number of times during a frame. BindVertices() must have been called prior to this
    * call. The api expects vertices to be given in triangle list format, meaning every 3 vertices corresponds
    * to one separate triangle. Diamond expects a clockwise winding order
    * 
    * @param vertexCount The amount of vertices that should be drawn (usually equivalent to the amount passed to BindVertices())
    * @param textureIndex The index of the registered texture that will override vertices and be drawn on all triangles. Pass -1 to ignore the override
    * @param objectTransform The world space transform of the object that is being drawn
    * @note See https://www.khronos.org/opengl/wiki/Face_Culling for information on winding order
    * @see BindVertices() RegisterTexture() diamond_vertex diamond_transform
    */
    void Draw(uint32_t vertexCount, int textureIndex, diamond_transform objectTransform);

    /*
    * Draw the currently bound vertices and indices to the screen
    * 
    * This can be called any number of times during a frame. BindVertices() and BindIndices() must have been called prior to this
    * call. With DrawIndexed(), any amount of vertices can be provided in any order, but indices must still make up sets of 3 because 
    * Diamond expects a list of triangles with clockwise winding order
    * 
    * @param indexCount The amount of indices that should be drawn (usually equivalent to the amount passed to BindIndices())
    * @param vertexCount The amount of vertices that should be drawn (usually equivalent to the amount passed to BindVertices())
    * @param textureIndex The index of the registered texture that will override vertices and be drawn on all triangles. Pass -1 to ignore the override
    * @param objectTransform The world space transform of the object that is being drawn
    * @note See https://www.khronos.org/opengl/wiki/Face_Culling for information on winding order
    * @see BindVertices() BindIndices() RegisterTexture() diamond_vertex diamond_transform
    */
    void DrawIndexed(uint32_t indexCount, uint32_t vertexCount, int textureIndex, diamond_transform objectTransform);

    /*
    * Draw a quad to the screen with a given transform
    * 
    * This can be called any number of times during a frame. There is no need to call BindVertices() or BindIndices(), as
    * this function handles the geometry for you.
    * 
    * @param textureIndex The index of the registered texture that will be drawn on the quad. Pass -1 to render only color
    * @param quadTransform The world space transform of the quad
    * @param color The color applied to the quad
    * @see RegisterTexture() diamond_transform
    */
    void DrawQuad(int textureIndex, diamond_transform quadTransform, glm::vec4 color = glm::vec4(1.f));

    /*
    * Draw many quads to the screen each with a given transform
    * 
    * This can be called any number of times during a frame. There is no need to call BindVertices() or BindIndices(), as
    * this function handles the geometry for you. This function produces the same results as DrawQuadsOffsetScale(), but
    * has a significant performance hit when rendering large amounts of quads. The advantage of this implementation is that
    * the data is provided in transforms which are organized and have support for rotation.
    * 
    * @param textureIndexes Array of indexes of registered textures that will be drawn on each quad. Pass a -1 to any element to render only color
    * @param quadTransforms Array of world space transforms of each quad
    * @param quadCount The amount of quads to render
    * @param originTransform Optional transform to transform all drawn quads by
    * @param colors Array of colors that will be applied to each quad. This parameter is optional and will default to no color applied (white)
    * @param texCoords Array of texture coordinates that will be applied to each quad, top left and bottom right. This parameter is optional and will default to [0, 0] and [1, 1]
    * @see DrawQuadsOffsetScale() RegisterTexture() diamond_transform
    */
    void DrawQuadsTransform(int* textureIndexes, diamond_transform* quadTransforms, int quadCount, diamond_transform originTransform = diamond_transform(), glm::vec4* colors = nullptr, glm::vec4* texCoords = nullptr);

    /*
    * Draw many quads to the screen each with a given offset and scale
    * 
    * This can be called any number of times during a frame. There is no need to call BindVertices() or BindIndices(), as
    * this function handles the geometry for you. This function produces the same results as DrawQuadsTransform(), but
    * has significantly better performance when rendering large amounts of quads. The disadvantage of this implementation is that
    * there is no support for rotation of the quads individually
    * 
    * @param textureIndexes Array of indexes of registered textures that will be drawn on each quad. Pass a -1 to any element to render only color
    * @param offsetScales Array of vec4 that represents the offset (x, y) and scale (z, w) of each quad
    * @param quadCount The amount of quads to render
    * @param originTransform Optional transform to transform all drawn quads by
    * @param colors Array of colors that will be applied to each quad. This parameter is optional and will default to no color applied (white)
    * @param texCoords Array of texture coordinates that will be applied to each quad, top left and bottom right. This parameter is optional and will default to [0, 0] and [1, 1]
    * @see DrawQuadsTransform() RegisterTexture() diamond_transform
    */
    void DrawQuadsOffsetScale(int* textureIndexes, glm::vec4* offsetScales, int quadCount, diamond_transform originTransform = diamond_transform(), glm::vec4* colors = nullptr, glm::vec4* texCoords = nullptr);

    /*
    * Generate a basic 2D view matrix given the position of the camera
    * 
    * @param cameraPosition World position of the camera
    */
    glm::mat4 GenerateViewMatrix(glm::vec2 cameraPosition);

    /*
    * Get the camera's projection matrix (CameraMode)
    * @see diamond_camera_mode
    */
    inline glm::mat4 GetProjectionMatrix() { return cameraProjMatrix; };

    /*
    * Is the engine marked as still running
    * 
    * Used as the condition in the while() game loop
    * 
    * @returns true if the engine is running and the window is still open
    */
    bool IsRunning();

    /*
    * Get the current size of the engine window
    * 
    * @returns The size of the window in pixels
    */
    glm::vec2 GetWindowSize();

    /*
    * Get the current aspect ratio of the engine window
    * 
    * @returns The aspect ratio
    */
    float GetAspectRatio();

    /*
    * Sets the glfw window to windowed or fullscreen exclusive mode depending on the input parameter
    * 
    * @param fullscreen true if the window should be fullscreen
    */
    void SetFullscreen(bool fullscreen);

    /*
    * Get the glfw window handle
    * 
    * This exposes the internal window handle for outside use. Useful for handling window/engine related functions
    * such as keyboard and mouse input
    * 
    * @returns glfw window handle
    * @warning Utilizing this handle may result in undefined behavior
    */
    inline GLFWwindow* Window() { return window; };

    /*
    * Get the set of core vulkan components
    * 
    * This exposes the internal three core vulkan components for outside use: instance, physical device, logical device
    * 
    * @returns A tuple of the three core components (VkInstance, VkPhysicalDevice, VkLogicalDevice)
    * @warning Utilizing these handles may result in undefined behavior
    */
    inline std::tuple<VkInstance, VkPhysicalDevice, VkDevice> VulkanComponents() { return std::make_tuple(instance, physicalDevice, logicalDevice); };

    /*
    * Get the vulkan swap chain tied to the engine window
    * 
    * This exposes the internal swap chain and its associated data for outside use
    * 
    * @returns A copy of the internal diamond_swap_chain_info structure
    * @warning Utilizing these handles may result in undefined behavior
    * @see diamond_swap_chain_info
    */
    inline diamond_swap_chain_info VulkanSwapChain() { return swapChain; };

private:

    // private documentation coming soon
    void ConfigureValidationLayers();
    void CreateGraphicsPipeline(const char* vertShaderPath, const char* fragShaderPath);
    void CreateRenderPass();
    void CreateSwapChain();
    void CreateFrameBuffers();
    void CreateCommandBuffers();
    void RecreateSwapChain();
    void CleanupSwapChain();
    void CreateVertexBuffer(int maxVertexCount);
    void CreateIndexBuffer(int maxIndexCount);
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
    diamond_camera_mode cameraMode = diamond_camera_mode::OrthographicViewportIndependent;
    glm::mat4 cameraViewMatrix;
    glm::mat4 cameraProjMatrix;
    glm::vec2 cameraDimensions;
    const char* defaultVertexShader = "";
    const char* defaultFragmentShader = "";
    int savedWindowSizeAndPos[4]; // size xy, pos xy

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