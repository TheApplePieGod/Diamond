#include <Diamond/diamond.h>
#include <iostream>
#include "../util/defs.h"
#include <gtc/matrix_transform.hpp>
#include <chrono>

// basic example
int main2(int argc, char** argv)
{
    diamond* Engine = new diamond();
    
    Engine->Initialize(800, 600, "Diamond Basic Example");

    int particleCount = 100000;
    std::array<diamond_compute_buffer_info, 1> cpBuffers { diamond_compute_buffer_info(sizeof(diamond_test_compute_buffer), false, true) };
    diamond_compute_pipeline_create_info cpCreateInfo = {};
    std::vector<glm::vec2> computeData(particleCount);
    cpCreateInfo.bufferCount = cpBuffers.size();
    cpCreateInfo.bufferInfoList = cpBuffers.data();
    cpCreateInfo.computeShaderPath = "../shaders/basic.comp.spv";
    cpCreateInfo.groupCountX = ceil(particleCount / 64.0);
    Engine->CreateComputePipeline(cpCreateInfo);
    Engine->MapComputeData(0, 0, 0, sizeof(diamond_test_compute_buffer), computeData.data()); // map inital data

    diamond_graphics_pipeline_create_info gpCreateInfo = {};
    gpCreateInfo.vertexShaderPath = "../shaders/basic.vert.spv";
    gpCreateInfo.fragmentShaderPath = "../shaders/basic.frag.spv";
    gpCreateInfo.maxVertexCount = 100000;
    gpCreateInfo.maxIndexCount = 100000;
    Engine->CreateGraphicsPipeline(gpCreateInfo);

    Engine->RegisterTexture("../images/test.png");
    Engine->RegisterTexture("../images/chev.jpg"); 
    Engine->SyncTextureUpdates();

    f32 deltaTime = 0.f;
    f32 fps = 0.f;

    int quadCount = 10; 
    std::vector<glm::vec4> quadOffsetScales(quadCount);
    std::vector<int> quadTextureIndexes(quadCount);
    f32 currentLocation = -500.f;
    for (int i = 0; i < quadCount; i++)
    {
        quadOffsetScales[i] = { currentLocation, 0.f, 50.f, 500.f };
        quadTextureIndexes[i] = i % 3;
        currentLocation += 100;
    }

    while (Engine->IsRunning())
    {
        auto start = std::chrono::high_resolution_clock::now();
        Engine->BeginFrame(diamond_camera_mode::OrthographicViewportIndependent, glm::vec2(500.f, 500.f), Engine->GenerateViewMatrix(glm::vec2(0.f, 0.f)));
        
        Engine->SetGraphicsPipeline(0);

        // Compute shader pipeline example
        Engine->RunComputeShader(0);
        Engine->DownloadComputeData(0, 0);
        Engine->RetrieveComputeData(0, 0, 0, sizeof(diamond_test_compute_buffer), computeData.data()); // retrieve updated data
        //std::cout << computeData[0].x; // debug print

        // Graphics pipeline example
        diamond_transform quadTransform;
        quadTransform.location = { 0.f, 0.f };
        quadTransform.rotation = 45.f;
        quadTransform.scale = { 500.f ,1000.f };
        Engine->DrawQuad(0, 1, quadTransform);

        quadTransform.location = { 300.f, 0.f };
        quadTransform.rotation = -45.f;
        quadTransform.scale = { 300.f, 300.f };
        Engine->DrawQuad(0, 2, quadTransform);

        Engine->DrawQuadsOffsetScale(0, quadTextureIndexes.data(), quadOffsetScales.data(), quadCount);

        Engine->EndFrame({ 0.f, 0.f, 0.f, 1.f });
        auto stop = std::chrono::high_resolution_clock::now();
        deltaTime = std::max((f32)(std::chrono::duration_cast<std::chrono::milliseconds>(stop - start)).count(), 0.5f);
        fps = 1.f / (deltaTime / 1000.f);
        std::cout << "FPS: " << fps << std::endl;
    }

    Engine->Cleanup();

    return 0;
}

// particle simulation example
int main(int argc, char** argv)
{
    diamond* Engine = new diamond();
    
    int particleCount = 100000;
    Engine->Initialize(800, 600, "Diamond Particle Simulation Example");

    std::array<diamond_compute_buffer_info, 2> cpBuffers {
        diamond_compute_buffer_info(sizeof(diamond_test_compute_buffer2), true, true),
        diamond_compute_buffer_info(sizeof(diamond_test_compute_buffer), false, false) // used as the buffer for velocities
    };
    diamond_compute_pipeline_create_info cpCreateInfo = {};
    cpCreateInfo.bufferCount = cpBuffers.size();
    cpCreateInfo.bufferInfoList = cpBuffers.data();
    cpCreateInfo.computeShaderPath = "../shaders/sim.comp.spv";
    cpCreateInfo.groupCountX = ceil(particleCount / 64.0);

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
            Engine->DrawFromCompute(0, 0, computeData.size()); // we still need to tell vulkan to draw the particles
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
            Engine->BindVertices(0, computeData.data(), computeData.size());
            Engine->Draw(0, computeData.size(), -1, trans);
        }

        Engine->EndFrame({ 0.f, 0.f, 0.f, 1.f });
        frameCount++;
        //std::cout << "FPS: " << Engine->FPS() << std::endl;
    }

    Engine->Cleanup();

    return 0;
}

// mandelbrot set example
int main3(int argc, char** argv)
{
    diamond* Engine = new diamond();
    
    Engine->Initialize(800, 600, "Diamond Mandelbrot Set Example");

    int imageSize = 2048;
    std::array<diamond_compute_image_info, 1> cpImages {
        diamond_compute_image_info(imageSize, imageSize, 8)
    };
    diamond_compute_pipeline_create_info cpCreateInfo = {};
    cpCreateInfo.imageCount = cpImages.size();
    cpCreateInfo.imageInfoList = cpImages.data();
    cpCreateInfo.bufferCount = 0;
    cpCreateInfo.bufferInfoList = nullptr;
    cpCreateInfo.computeShaderPath = "../shaders/mandel.comp.spv";
    cpCreateInfo.groupCountX = ceil(imageSize / 8);
    cpCreateInfo.groupCountY = ceil(imageSize / 8);
    cpCreateInfo.usePushConstants = true;
    cpCreateInfo.pushConstantsDataSize = sizeof(diamond_test_compute_constants);

    diamond_graphics_pipeline_create_info gpCreateInfo = {};
    gpCreateInfo.vertexShaderPath = "../shaders/mandel.vert.spv";
    gpCreateInfo.fragmentShaderPath = "../shaders/mandel.frag.spv";
    gpCreateInfo.maxVertexCount = 1000;
    gpCreateInfo.maxIndexCount = 1000;
    Engine->CreateGraphicsPipeline(gpCreateInfo);

    Engine->CreateComputePipeline(cpCreateInfo);

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
        Engine->DrawQuad(0, 1, quadTransform);

        Engine->EndFrame({ 0.f, 0.f, 0.f, 1.f });
        //std::cout << "FPS: " << Engine->FPS() << std::endl;
    }

    Engine->Cleanup();

    return 0;
}