#include <Diamond/diamond.h>
#include <iostream>
#include "../util/defs.h"
#include <glm/gtc/matrix_transform.hpp>
#include <imgui/imgui.h>

// the main function of the program is at the bottom of this file

// basic example
int basic_example(int argc, char** argv)
{
    diamond* Engine = new diamond();
    
    Engine->Initialize(800, 600, "Diamond Basic Example", "../../images/default-texture.png");

    diamond_graphics_pipeline_create_info gpCreateInfo = {};
    gpCreateInfo.vertexShaderPath = "../shaders/basic.vert.spv";
    gpCreateInfo.fragmentShaderPath = "../shaders/basic.frag.spv";
    gpCreateInfo.maxVertexCount = 100000;
    gpCreateInfo.maxIndexCount = 100000;
    Engine->CreateGraphicsPipeline(gpCreateInfo);

    Engine->RegisterTexture("../../images/test.png");
    Engine->RegisterTexture("../../images/chev.jpg"); 
    Engine->SyncTextureUpdates();

    int quadCount = 10; 
    std::vector<glm::vec4> quadOffsetScales(quadCount);
    std::vector<int> quadTextureIndexes(quadCount);
    f32 currentLocation = -500.f;
    for (int i = 0; i < quadCount; i++)
    {
        quadOffsetScales[i] = { currentLocation, 0.f, 50.f, 500.f };
        if (i % 2 == 0)
            quadTextureIndexes[i] = 0;
        else
            quadTextureIndexes[i] = 2;
        currentLocation += 100;
    }

    double animationTime = 4000.0; // 4 seconds
    double timer = 0.0;
    int framesPerRow = 12;
    int totalFrames = 72;
    while (Engine->IsRunning())
    {
        Engine->BeginFrame(diamond_camera_mode::OrthographicViewportIndependent, glm::vec2(500.f, 500.f), Engine->GenerateViewMatrix(glm::vec2(0.f, 0.f)));
        Engine->SetGraphicsPipeline(0);
        timer += Engine->FrameDelta();

        // imgui can go anywhere in between BeginFrame() and EndFrame()
        ImGui::Begin("Window");
        ImGui::End();

        int currentFrame = static_cast<int>((timer / animationTime) * totalFrames);
        diamond_transform quadTransform;
        quadTransform.location = { 0.f, 800.f };
        quadTransform.rotation = 45.f;
        quadTransform.scale = { 500.f , 500.f };
        Engine->DrawAnimatedQuad(1, framesPerRow, totalFrames, currentFrame, quadTransform);

        quadTransform.location = { 300.f, 0.f };
        quadTransform.rotation = -45.f;
        quadTransform.scale = { 300.f, 300.f };
        Engine->DrawQuad(2, quadTransform);

        Engine->DrawQuadsOffsetScale(quadTextureIndexes.data(), quadOffsetScales.data(), quadCount);

        if (timer == animationTime)
            timer = 0.0;

        //std::cout << "FPS: " << Engine->FPS() << std::endl;
        Engine->EndFrame({ 0.f, 0.f, 0.f, 1.f });
    }

    Engine->Cleanup();

    return 0;
}

// particle simulation example
int particle_example(int argc, char** argv)
{
    diamond* Engine = new diamond();

    int particleCount = 100000;
    Engine->Initialize(800, 600, "Diamond Particle Simulation Example", "../../images/default-texture.png");

    std::array<diamond_compute_buffer_info, 2> cpBuffers {
        diamond_compute_buffer_info(sizeof(diamond_test_compute_buffer2), true, true),
        diamond_compute_buffer_info(sizeof(diamond_test_compute_buffer), false, false) // used as the buffer for velocities
    };
    diamond_compute_pipeline_create_info cpCreateInfo = {};
    cpCreateInfo.bufferCount = static_cast<int>(cpBuffers.size());
    cpCreateInfo.bufferInfoList = cpBuffers.data();
    cpCreateInfo.computeShaderPath = "../shaders/sim.comp.spv";
    cpCreateInfo.groupCountX = static_cast<u32>(ceil(particleCount / 64.0));

    diamond_graphics_pipeline_create_info gpCreateInfo = {};
    gpCreateInfo.vertexShaderPath = "../shaders/sim.vert.spv";
    gpCreateInfo.fragmentShaderPath = "../shaders/sim.frag.spv";
    gpCreateInfo.maxVertexCount = particleCount;
    gpCreateInfo.maxIndexCount = 1;
    gpCreateInfo.vertexSize = sizeof(diamond_particle_vertex);
    gpCreateInfo.vertexTopology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    gpCreateInfo.getVertexAttributeDescriptions = diamond_particle_vertex::GetAttributeDescriptions;
    gpCreateInfo.getVertexBindingDescription = diamond_particle_vertex::GetBindingDescription;
    Engine->CreateGraphicsPipeline(gpCreateInfo);

    // initialze particle data
    std::vector<diamond_particle_vertex> computeData(particleCount);
    std::vector<glm::vec2> velocities(particleCount);
    for (int i = 0; i < computeData.size(); i++)
    {
        computeData[i].pos = glm::vec2(rand() % 2000 - 1000, rand() % 2000 - 1000);
        computeData[i].color = glm::vec4((double) rand() / (RAND_MAX), (double) rand() / (RAND_MAX), (double) rand() / (RAND_MAX), 1.0);
        velocities[i] = glm::vec2(rand() % 5 - 2.5, rand() % 5 - 2.5);
    }

    Engine->CreateComputePipeline(cpCreateInfo);
    Engine->MapComputeData(0, 0, 0, sizeof(diamond_test_compute_buffer2), computeData.data()); // map inital data
    Engine->MapComputeData(0, 1, 0, sizeof(diamond_test_compute_buffer), velocities.data()); // map inital data

    int frameCount = 0;
    bool useCompute = true; // disable to run the same code on the cpu to compare performance
    while (Engine->IsRunning())
    {
        Engine->BeginFrame(diamond_camera_mode::OrthographicViewportIndependent, glm::vec2(500.f, 500.f), Engine->GenerateViewMatrix(glm::vec2(0.f, 0.f)));
        
        Engine->SetGraphicsPipeline(0);

        if (useCompute)
        {
            if (frameCount == 0)
                Engine->UploadComputeData(0, 0);
            Engine->RunComputeShader(0);
            Engine->DrawFromCompute(0, 0, static_cast<u32>(computeData.size())); // we still need to tell vulkan to draw the particles
        }
        else
        {
            for (int i = 0; i < computeData.size(); i++)
            {
                computeData[i].pos += velocities[i];
                if (abs(computeData[i].pos.x) > 2000 || abs(computeData[i].pos.y) > 2000)
                    velocities[i] *= -1;
            }
            diamond_transform trans;
            Engine->BindVertices(computeData.data(), static_cast<u32>(computeData.size()));
            Engine->Draw(static_cast<u32>(computeData.size()), -1, trans);
        }

        Engine->EndFrame({ 0.f, 0.f, 0.f, 1.f });
        frameCount++;
        //std::cout << "FPS: " << Engine->FPS() << std::endl;
    }

    Engine->Cleanup();

    return 0;
}

// mandelbrot set example
int mandelbrot_example(int argc, char** argv)
{
    diamond* Engine = new diamond();
    
    Engine->Initialize(800, 600, "Diamond Mandelbrot Set Example", "../../images/default-texture.png");

    int imageSize = 2048;
    std::array<diamond_compute_image_info, 1> cpImages {
        diamond_compute_image_info(imageSize, imageSize, 8)
    };
    diamond_compute_pipeline_create_info cpCreateInfo = {};
    cpCreateInfo.imageCount = static_cast<int>(cpImages.size());
    cpCreateInfo.imageInfoList = cpImages.data();
    cpCreateInfo.bufferCount = 0;
    cpCreateInfo.bufferInfoList = nullptr;
    cpCreateInfo.computeShaderPath = "../shaders/mandel.comp.spv";
    cpCreateInfo.groupCountX = static_cast<u32>(ceil(imageSize / 8));
    cpCreateInfo.groupCountY = static_cast<u32>(ceil(imageSize / 8));
    cpCreateInfo.usePushConstants = true;
    cpCreateInfo.pushConstantsDataSize = sizeof(diamond_test_compute_constants);

    diamond_graphics_pipeline_create_info gpCreateInfo = {};
    gpCreateInfo.vertexShaderPath = "../shaders/mandel.vert.spv";
    gpCreateInfo.fragmentShaderPath = "../shaders/mandel.frag.spv";
    gpCreateInfo.maxVertexCount = 1000;
    gpCreateInfo.maxIndexCount = 1000;
    Engine->CreateGraphicsPipeline(gpCreateInfo);

    Engine->CreateComputePipeline(cpCreateInfo);

    int textureIndex = Engine->GetComputeTextureIndex(0, 0);

    diamond_test_compute_constants constants = {};
    constants.offsetX = 1.5f;
    constants.offsetY = 0.0008f;
    while (Engine->IsRunning())
    {
        Engine->BeginFrame(diamond_camera_mode::OrthographicViewportIndependent, glm::vec2(500.f, 500.f), Engine->GenerateViewMatrix(glm::vec2(0.f, 0.f)));

        Engine->SetGraphicsPipeline(0);

        Engine->RunComputeShader(0, &constants);
        constants.zoom *= 0.995f; 
        constants.offsetX = -constants.zoom * 0.2f + 1.48f;

        diamond_transform quadTransform;
        quadTransform.location = { 0.f, 0.f };
        quadTransform.rotation = 0.f;
        quadTransform.scale = { 3000.f ,3000.f };
        Engine->DrawQuad(1, quadTransform);

        Engine->EndFrame({ 0.f, 0.f, 0.f, 1.f });
        //std::cout << "FPS: " << Engine->FPS() << std::endl;
    }

    Engine->Cleanup();

    return 0;
}

// uncomment the example to use before compiling
int main(int argc, char** argv)
{
    return basic_example(argc, argv);
    //return particle_example(argc, argv);
    //return mandelbrot_example(argc, argv);
}