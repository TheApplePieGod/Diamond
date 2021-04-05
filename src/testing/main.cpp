#include <Diamond/diamond.h>
#include <iostream>
#include "../util/defs.h"
#include <gtc/matrix_transform.hpp>
#include <chrono>

// basic example
int main2(int argc, char** argv)
{
    diamond* Engine = new diamond();
    
    Engine->Initialize(800, 600, 100000, 100000, "Diamond Basic Example", "../shaders/basic.vert.spv", "../shaders/basic.frag.spv");

    std::array<diamond_compute_buffer_info, 1> cpBuffers = { sizeof(diamond_test_compute_buffer) };
    diamond_compute_pipeline_create_info cpCreateInfo = {};
    std::vector<glm::vec2> computeData(100);
    cpCreateInfo.enabled = true;
    cpCreateInfo.bufferCount = cpBuffers.size();
    cpCreateInfo.bufferInfoList = cpBuffers.data();
    cpCreateInfo.computeShaderPath = "../shaders/basic.comp.spv";
    cpCreateInfo.groupCountX = floor(sizeof(diamond_test_compute_buffer) / sizeof(glm::vec2));
    Engine->UpdateComputePipeline(cpCreateInfo);
    Engine->MapComputeData(0, 0, sizeof(diamond_test_compute_buffer), computeData.data()); // map inital data

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
        
        // Compute shader pipeline example
        Engine->RunComputeShader();
        Engine->RetrieveComputeData(0, 0, sizeof(diamond_test_compute_buffer), computeData.data()); // retrieve updated data
        //std::cout << computeData[0].x; // debug print

        // Graphics pipeline example
        diamond_transform quadTransform;
        quadTransform.location = { 0.f, 0.f };
        quadTransform.rotation = 45.f;
        quadTransform.scale = { 500.f ,1000.f };
        Engine->DrawQuad(1, quadTransform);

        quadTransform.location = { 300.f, 0.f };
        quadTransform.rotation = -45.f;
        quadTransform.scale = { 300.f, 300.f };
        Engine->DrawQuad(2, quadTransform);

        Engine->DrawQuadsOffsetScale(quadTextureIndexes.data(), quadOffsetScales.data(), quadCount);

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
    
    Engine->Initialize(800, 600, 100000, 100000, "Diamond Particle Simulation Example", "../shaders/sim.vert.spv", "../shaders/sim.frag.spv");

    std::array<diamond_compute_buffer_info, 1> cpBuffers = { sizeof(diamond_test_compute_buffer) };
    diamond_compute_pipeline_create_info cpCreateInfo = {};
    std::vector<glm::vec2> computeData(100);
    cpCreateInfo.enabled = true;
    cpCreateInfo.bufferCount = cpBuffers.size();
    cpCreateInfo.bufferInfoList = cpBuffers.data();
    cpCreateInfo.computeShaderPath = "../shaders/sim.comp.spv";
    cpCreateInfo.groupCountX = floor(sizeof(diamond_test_compute_buffer) / sizeof(glm::vec2));
    Engine->UpdateComputePipeline(cpCreateInfo);
    Engine->MapComputeData(0, 0, sizeof(diamond_test_compute_buffer), computeData.data()); // map inital data

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
        
        // Compute shader pipeline example
        Engine->RunComputeShader();
        Engine->RetrieveComputeData(0, 0, sizeof(diamond_test_compute_buffer), computeData.data()); // retrieve updated data
        //std::cout << computeData[0].x; // debug print

        // Graphics pipeline example
        diamond_transform quadTransform;
        quadTransform.location = { 0.f, 0.f };
        quadTransform.rotation = 45.f;
        quadTransform.scale = { 500.f ,1000.f };
        Engine->DrawQuad(1, quadTransform);

        quadTransform.location = { 300.f, 0.f };
        quadTransform.rotation = -45.f;
        quadTransform.scale = { 300.f, 300.f };
        Engine->DrawQuad(2, quadTransform);

        Engine->DrawQuadsOffsetScale(quadTextureIndexes.data(), quadOffsetScales.data(), quadCount);

        Engine->EndFrame({ 0.f, 0.f, 0.f, 1.f });
        auto stop = std::chrono::high_resolution_clock::now();
        deltaTime = std::max((f32)(std::chrono::duration_cast<std::chrono::milliseconds>(stop - start)).count(), 0.5f);
        fps = 1.f / (deltaTime / 1000.f);
        std::cout << "FPS: " << fps << std::endl;
    }

    Engine->Cleanup();

    return 0;
}