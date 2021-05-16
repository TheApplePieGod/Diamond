#pragma once
#include "structures.h"

#if DIAMOND_IMGUI
#include "imgui/imgui_impl_vulkan.h"
#include "imgui/imgui_impl_glfw.h"
#endif

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
    * @param windowName Desired name of the window and also name of the vulkan application
    * @param defaultTexturePath The path to the default texture the engine will fallback to in the case of a missing texture
    */
    void Initialize(int width, int height, const char* windowName, const char* defaultTexturePath);

    /*
    * Called at the start of every frame in the game loop
    * 
    * Internal updates, resets command buffers, and starts a new ImGui frame if applicable
    * 
    * @param cameraMode Specify whether to render the scene using a predefined perspective or orthographic view
    * @param cameraDimensions Dimensions to use for the projection if camera mode is orthographic frame independent
    * @param cameraViewMatrix Specify the view matrix of the camera
    * @see GenerateViewMatrix()
    */
    void BeginFrame();

    /*
    * Called at the end of every frame in the game loop
    * 
    * Does some cleanup and also handles presenting to the swap chain image buffer. Also draws ImGui data if applicable
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
    * Sets the view matrix of the camera to render the scene from
    *
    * The matrix must be regenerated and set every time the camera moves
    * 
    * @param matrix The view matrix
    * @see GenerateViewMatrix()
    */
    void SetCameraViewMatrix(glm::mat4 matrix);

    /*
    * Sets the mode of the camera which renders the scene
    *
    * This only needs to be called once unless the mode changes or typically when the screen gets resized
    * 
    * @param camMode The projection mode of the camera
    * @camDimensions The size that the camera should be able to see (only applies for OrthographicViewportIndependent mode)
    * @see diamond_camera_mode
    */
    void UpdateCameraViewMode(diamond_camera_mode camMode, glm::vec2 camDimensions);

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
    * Call this after all registrations have been made. It can be called multiple times, but every call recreates many parts of
    * the vulkan pipeline and will likely cause performance issues
    * 
    * @see RegisterTexture()
    */
    void SyncTextureUpdates();
    
    /*
    * Create a pipeline which specifies the shaders and information layouts to use during the following draw calls
    *
    * The pipeline to be used must be set explicitly every frame before any draw calls are made. The pipeline can
    * be switched any number of times during a frame
    * 
    * @param createInfo The struct containing creation information about the pipeline
    * @returns The index of the pipeline for future referencing
    * @see diamond_graphics_pipeline_create_info
    */
    int CreateGraphicsPipeline(diamond_graphics_pipeline_create_info createInfo);

    /*
    * Delete a graphics pipeline via its index
    *
    * The id of the old pipeline never gets reused
    * 
    * @param pipelineIndex The index of the graphics pipeline
    * @see CreateGraphicsPipeline()
    */
    void DeleteGraphicsPipeline(int pipelineIndex);

    /*
    * Create a pipeline which enables the use of powerful vulkan compute shaders
    *
    * Each pipeline must be run explicitly. See the creation info struct for more details on
    * what can be supplied to and accessed from the shader
    * 
    * @param createInfo The struct containing creation information about the pipeline
    * @returns The index of the pipeline for future referencing
    * @see diamond_compute_pipeline_create_info RunComputeShader()
    * @see https://vkguide.dev/docs/gpudriven/compute_shaders/
    */
    int CreateComputePipeline(diamond_compute_pipeline_create_info createInfo);

    /*
    * Delete a compute pipeline via its index
    *
    * The id of the old pipeline never gets reused nor does the associated image texture indexes
    * 
    * @param pipelineIndex The index of the compute pipeline
    * @see CreateComputePipeline()
    */
    void DeleteComputePipeline(int pipelineIndex);

    /*
    * Map data to a compute pipeline buffer to be accessed in the shader
    *
    * Can be called any time. When a buffer is flagged as staging, the data must also be uploaded after it is mapped in order
    * for the changes to propogate to the GPU memory
    * 
    * @param pipelineIndex The index of the compute pipeline
    * @param bufferIndex The index local to this specific pipeline of the destination buffer
    * @param dataOffset The offset in bytes in the source buffer to start copying from
    * @param dataSize The size in bytes of the copy
    * @param source A pointer to the source data to copy from 
    * @see UploadComputeData()
    */
    void MapComputeData(int pipelineIndex, int bufferIndex, int dataOffset, int dataSize, void* source);

    /*
    * Retrieve data from a compute pipeline buffer to be accessed locally on the CPU
    *
    * Can be called any time. When a buffer is flagged as staging, the data must downloaded first before it is retrieved in order
    * to obtain accurate data
    * 
    * @param pipelineIndex The index of the compute pipeline
    * @param bufferIndex The index local to this specific pipeline of the source buffer
    * @param dataOffset The offset in bytes in the source buffer to start copying from
    * @param dataSize The size in bytes of the copy
    * @param destination A pointer to the destination memory to copy to 
    * @see DownloadComputeData()
    */
    void RetrieveComputeData(int pipelineIndex, int bufferIndex, int dataOffset, int dataSize, void* destination);

    /*
    * Transfer data stored on a local buffer marked as staging onto the GPU
    *
    * Must be called between begin and end frame. The operation completes at the end of the frame, so mapping can occur at any point before then.
    * This also means that the new data will not be accessable on the shader until the next frame
    * 
    * @param pipelineIndex The index of the compute pipeline
    * @param bufferIndex The index local to this specific pipeline of the buffer to upload
    * @see MapComputeData()
    */
    void UploadComputeData(int pipelineIndex, int bufferIndex);

    /*
    * Transfer buffer data stored on the GPU to a local staging buffer 
    *
    * Must be called between begin and end frame. The operation completes at the end of the frame, which means that retrieval
    * must occur during the next frame in order to retrieve accurate data
    * 
    * @param pipelineIndex The index of the compute pipeline
    * @param bufferIndex The index local to this specific pipeline of the buffer to download
    * @see RetrieveComputeData()
    */
    void DownloadComputeData(int pipelineIndex, int bufferIndex);

    /*
    * Run the specified compute shader
    *
    * Will run using whatever data is currently bound and will not move on until it is completed
    * 
    * @param pipelineIndex The index of the compute pipeline
    * @param pushConstantsData A pointer to the data which should be bound to the push constants for this execution (if usePushConstants is set)
    */
    void RunComputeShader(int pipelineIndex, void* pushConsantsData = nullptr);

    /*
    * Set the graphics pipeline to be used during the following draw calls
    *
    * Can be called multiple times per frame. Switch to the desired pipeline before any draw calls are made or vertices/indices are bound, as
    * they rely on the currently bound graphics pipeline. The bound pipeline gets reset every frame; thus, this function must be recalled
    * 
    * @param pipelineIndex The index of the graphics pipeline to set
    */
    void SetGraphicsPipeline(int pipelineIndex);

    /*
    * Get the texture index of the specified image bound to the specified compute pipeline
    * 
    * @param pipelineIndex The index of the compute pipeline
    * @param imageIndex The index local to this specific pipeline of the image to get the associated texture index
    * @returns 
    */
    int GetComputeTextureIndex(int pipelineIndex, int imageIndex);

    /*
    * Get the device max supported number of workgroup dispatches for each dimension
    * 
    * These values are typically extremely large, and they represent the maximum number which can be passed into the
    * compute pipeline's groupCount fields
    * 
    * @returns A vec3 containing the max size for each dimension
    * @see https://www.khronos.org/opengl/wiki/Compute_Shader#Inputs
    */
    glm::vec3 GetDeviceMaxWorkgroupCount();

    /*
    * Get the device max supported number of workgroups for each dimension
    * 
    * These values reference the workgroup sizes which are defined in the compute shader file itself
    * 
    * @returns A vec3 containing the max size for each dimension
    * @see https://www.khronos.org/opengl/wiki/Compute_Shader#Inputs
    */
    glm::vec3 GetDeviceMaxWorkgroupSize();

    /*
    * Set the vertices that will be drawn in the next draw call in the currently bound pipeline
    * 
    * This can be called any number of times during a frame. Call it right before a draw call is made
    * and it can be called again immediately after for a different set of vertices if necessary.
    * 
    * @param vertices An array of vertex data which match the layout defined in the specified pipeline
    * @param vertexCount The amount of vertices passed in the array
    * @see Draw() DrawIndexed()
    */
    void BindVertices(const void* vertices, uint32_t vertexCount);
    void BindVertices(void* vertices, uint32_t vertexCount);

    /*
    * Set the indices that will be used in the next DrawIndexed() call in the currently bound pipeline
    * 
    * This can be called any number of times during a frame. Call it right before a DrawIndexed() call is made
    * and it can be called again immediately after for a different set of indices if necessary.
    * 
    * @param indices An array of vertex indices
    * @param index The amount of indices passed in the array
    * @note See https://www.proofof.blog/2018/09/24/vertex-and-index-buffer.html for information on indexed drawing
    * @see Draw() DrawIndexed()
    */
    void BindIndices(const uint16_t* indices, uint32_t indexCount);
    void BindIndices(uint16_t* indices, uint32_t indexCount);

    /*
    * Draw the currently bound vertices to the screen using the currently bound pipeline
    * 
    * This can be called any number of times during a frame. BindVertices() must have been called prior to this
    * call. The vertices must be provided in the same format defined in the pipeline (e.g. VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST must
    * be provided as sets of three vertices defining a single triangle). A clockwise winding order is also required
    * 
    * The second override is functionally identical to the first, except it assumes the pipeline has useCustomPushConstants set to false
    * because it uses the default push constants struct to provide the data passed in through the parameters
    * 
    * @param vertexCount The amount of vertices that should be drawn (usually equivalent to the amount passed to BindVertices())
    * @param pushConstantsData A pointer to the data which should be pushed for this object, which means useCustomPushConstants must be true and the layout must match the one specified in the pipeline
    * @param textureIndex The index of the desired texture for this object that will be passed through to the shader
    * @param objectTransform The world space transform of the object that is being drawn that will be passed through to the shader
    * @note See https://www.khronos.org/opengl/wiki/Face_Culling for information on winding order
    * @see BindVertices() RegisterTexture() diamond_vertex diamond_transform diamond_object_data
    */
    void Draw(uint32_t vertexCount, void* pushConstantsData);
    void Draw(uint32_t vertexCount, int textureIndex, diamond_transform objectTransform);

    /*
    * Draw the currently bound vertices and indices to the screen using the currently bound pipeline
    * 
    * This can be called any number of times during a frame. BindVertices() and BindIndices() must have been called prior to this
    * call. The vertices must be provided in the same format defined in the pipeline (e.g. VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST must
    * be provided as sets of three vertices defining a single triangle). A clockwise winding order is also required
    * 
    * The second override is functionally identical to the first, except it assumes the pipeline has useCustomPushConstants set to false
    * because it uses the default push constants struct to provide the data passed in through the parameters
    * 
    * @param indexCount The amount of indices that should be drawn (usually equivalent to the amount passed to BindIndices())
    * @param vertexCount The amount of vertices that should be drawn (usually equivalent to the amount passed to BindVertices())
    * @param pushConstantsData A pointer to the data which should be pushed for this object, which means useCustomPushConstants must be true and the layout must match the one specified in the pipeline
    * @param textureIndex The index of the desired texture for this object that will be passed through to the shader
    * @param objectTransform The world space transform of the object that is being drawn that will be passed through to the shader
    * @note See https://www.khronos.org/opengl/wiki/Face_Culling for information on winding order
    * @see BindIndices() BindVertices() RegisterTexture() diamond_vertex diamond_transform diamond_object_data
    */
    void DrawIndexed(uint32_t indexCount, uint32_t vertexCount, void* pushConstantsData);
    void DrawIndexed(uint32_t indexCount, uint32_t vertexCount, int textureIndex, diamond_transform objectTransform);

    /*
    * Use a compute shader buffer as a vertex buffer and draw it to the screen using the currently bound graphics pipeline
    *
    * This can be called any number of times during a frame. The specified buffer must be created with bindVertexBuffer set to true. The data
    * in the buffer must match the vertex layout defined in the graphics pipeline. Because this function pulls the vertex data right from the GPU, regardless
    * of if the buffer is marked as staging, no data copying or uploading/downloading has to happen.
    * 
    * @param pipelineIndex The index of the compute pipeline
    * @param bufferIndex The index local to this specific pipeline of the buffer to use
    * @param vertexCount The amount of vertices to be drawn from the buffer 
    */
    void DrawFromCompute(int pipelineIndex, int bufferIndex, uint32_t vertexCount); // This will use the currently bound graphics pipeline, but draw vertices from a compute shader buffer

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
    * @warning This is is not compatible when a custom vertex structure is being used
    */
    void DrawQuad(int textureIndex, diamond_transform quadTransform, glm::vec4 color = glm::vec4(1.f));

    /*
    * Draw a quad which has an animated texture to the screen with a given transform
    * 
    * This can be called any number of times during a frame. There is no need to call BindVertices() or BindIndices(), as
    * this function handles the geometry for you.
    * 
    * @param textureIndex The index of the registered texture that will be drawn on the quad. Pass -1 to render only color
    * @param framesPerRow The amount of frames in a single row of the animated texture
    * @param totalFrames The total amount of frames in the animated texture
    * @param currentFrame The current frame of the animation that should be rendered
    * @param quadTransform The world space transform of the quad
    * @param color The color applied to the quad
    * @see RegisterTexture() diamond_transform
    * @warning This is is not compatible when a custom vertex structure is being used
    * @note Each frame of the animation must be uniform in size
    */
    void DrawAnimatedQuad(int textureIndex, int framesPerRow, int totalFrames, int currentFrame, diamond_transform quadTransform, glm::vec4 color = glm::vec4(1.f));

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
    * @warning This is is not compatible when custom vertex structure or push constants are being used
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
    * @warning This is is not compatible when custom vertex structure or push constants are being used
    */
    void DrawQuadsOffsetScale(int* textureIndexes, glm::vec4* offsetScales, int quadCount, diamond_transform originTransform = diamond_transform(), glm::vec4* colors = nullptr, glm::vec4* texCoords = nullptr);

    /*
    * Generate a basic 2D view matrix given the position of the camera
    * 
    * The camera is always looking directly ahead towards z = 0
    * 
    * @param cameraPosition World position of the camera (effects of the z position depend on the camera view mode)
    */
    glm::mat4 GenerateViewMatrix(glm::vec3 cameraPosition);

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
    * @returns The aspect ratio x/y
    */
    float GetAspectRatio();

    /*
    * Sets the glfw window to windowed or fullscreen exclusive mode depending on the input parameter
    * 
    * @param fullscreen True if the window should be fullscreen
    */
    void SetFullscreen(bool fullscreen);

    /*
    * Sets the glfw window to the specified size
    * 
    * @param size The desired size in pixels
    */
    void SetWindowSize(glm::vec2 size);

    /*
    * Records the time elapsed between when BeginFrame() and EndFrame() are called. The time is queried
    * using std::high_resolution_clock in nanoseconds and then converted to milliseconds and averaged and
    * trimmed over a period of 11 frames in order to give the most accurate and consistent result.
    *
    * @returns The time in milliseconds
    */
    inline double FrameDelta() { return frameDelta; };

    /*
    * Same as FrameDelta() except returns the raw delta for the last frame with no smoothing or averaging
    *
    * @returns The time in milliseconds
    */
    inline double FrameDeltaRaw() { return currentFrameDelta; };

    /*
    * @returns The current FPS based off of the current averaged FrameDelta
    */
    inline double FPS() { return fps; };

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
    * Get the set of vulkan components used during rendering
    * 
    * This exposes two vulkan components for outside use: the main render pass, the command buffer used during the main render pass
    * 
    * @returns A tuple of the two components (VkRenderPass, VkCommandBuffer)
    * @warning Utilizing these handles may result in undefined behavior
    */
    inline std::tuple<VkRenderPass, VkCommandBuffer> VulkanRenderComponents() { return std::make_tuple(renderPass, renderPassBuffer); };

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

    /*
    * Start a single-use command buffer for completing general vulkan tasks
    *
    * @returns The handle to the created command buffer
    * @warning Utilizing these functions may result in undefined behavior
    */
    VkCommandBuffer BeginSingleTimeCommands();

    /*
    * Submit & cleanup a single-use command buffer
    *
    * @param commandBuffer The handle of the command buffer returned by BeginSingleTimeCommands()
    * @warning Utilizing these functions may result in undefined behavior
    */
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

private:

    // private documentation coming soon
    void MemoryBarrier(VkCommandBuffer cmd, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);
    void ConfigureValidationLayers();
    void CreateGraphicsPipeline(diamond_graphics_pipeline& pipeline);
    void CreateComputePipeline(diamond_compute_pipeline& pipeline);
    void CreateRenderPass();
    void CreateSwapChain();
    void CreateFrameBuffers();
    void CreateCommandBuffers();
    void RecreateSwapChain();
    void RecreateCompute(diamond_compute_pipeline& pipeline, diamond_compute_pipeline_create_info createInfo);
    void CleanupSwapChain();
    void CleanupCompute(diamond_compute_pipeline& pipeline);
    void CleanupGraphics(diamond_graphics_pipeline& pipeline);
    void CreateVertexBuffer(int vertexSize, int maxVertexCount, VkBuffer& vertexBuffer, VkDeviceMemory& vertexBufferMemory);
    void CreateIndexBuffer(int maxIndexCount, VkBuffer& indexBuffer, VkDeviceMemory& indexBufferMemory);
    void CreateDescriptorSetLayout();
    void CreateComputeDescriptorSetLayout(diamond_compute_pipeline& pipeline, int bufferCount, int imageCount);
    void CreateUniformBuffers();
    void UpdatePerFrameBuffer(uint32_t imageIndex);
    void CreateDescriptorPool();
    void CreateComputeDescriptorPool(diamond_compute_pipeline& pipeline, int bufferCount, int imageCount);
    void CreateDescriptorSets();
    void CreateComputeDescriptorSets(diamond_compute_pipeline& pipeline, int bufferCount, int imageCount, diamond_compute_buffer_info* bufferInfo);
    void CreateTextureSampler();
    void CreateColorResources();
    void CreateDepthResources();
    void Present();

    #if DIAMOND_IMGUI
    void CleanupImGui();
    void CreateImGui();
    ImGui_ImplVulkan_InitInfo ImGuiInitInfo();
    #endif

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
    VkImageView CreateTextureImage(const char* imagePath, VkImage& image, VkDeviceMemory& imageMemory, int& width, int& height);
    VkImageView CreateTextureImage(void* data, VkImage& image, VkDeviceMemory& imageMemory, int width, int height);
    void CreateImage(uint32_t width, uint32_t height, VkFormat format, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED);
    void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    VkImageView CreateImageView(VkImage image, VkFormat format, uint32_t mipLevels, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
    glm::mat4 GenerateModelMatrix(diamond_transform objectTransform);
    VkSampleCountFlagBits GetMaxSampleCount();

    GLFWwindow* window;

    std::array<double, 11> deltaTimes;
    int frameCount = 0;
    double frameDelta = 0.0;
    double currentFrameDelta = 0.0;
    double fps = 0.0;
    std::chrono::steady_clock::time_point frameStartTime;

    std::vector<const char*> validationLayers = {};
    std::vector<const char*> deviceExtensions = {};
    const int MAX_FRAMES_IN_FLIGHT = 2;
    int currentFrameIndex = 0;
    uint32_t nextImageIndex = 0;
    bool shouldPresent = true;
    diamond_camera_mode cameraMode = diamond_camera_mode::OrthographicViewportIndependent;
    glm::mat4 cameraViewMatrix;
    glm::mat4 cameraProjMatrix;
    glm::vec2 cameraDimensions = { 500.f, 500.f };
    int savedWindowSizeAndPos[4]; // size xy, pos xy
    std::vector<diamond_vertex> quadVertices;
    std::vector<uint16_t> quadIndices;
    VkPhysicalDeviceProperties physicalDeviceProperties;

    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice logicalDevice = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkQueue computeQueue = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
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
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    
    // compute
    std::vector<diamond_compute_pipeline> computePipelines;
    VkFence computeFence = VK_NULL_HANDLE;
    VkCommandBuffer computeBuffer = {};
    std::vector<const char*> freedBuffers;

    // graphics
    std::vector<diamond_graphics_pipeline> graphicsPipelines;
    int boundGraphicsPipelineIndex = -1;

    diamond_swap_chain_info swapChain;
};