#include "../diamond.h"
#include <iostream>
#include "../util/defs.h"
#include <gtc/matrix_transform.hpp>
#include <chrono>

int main(int argc, char** argv)
{
    diamond* Engine = new diamond();
    
    Engine->Initialize(800, 600, "Diamond Test", "../shaders/test.vert.spv", "../shaders/test.frag.spv");

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
        Engine->BeginFrame(diamond_camera_mode::Orthographic, { 0.f, 0.f });
        
        diamond_transform quadTransform;
        quadTransform.location = { 0.f, 0.f };
        quadTransform.rotation = 45.f;
        quadTransform.scale = { 500.f, 500.f };
        Engine->DrawQuad(1, quadTransform);

        quadTransform.location = { 300.f, 0.f };
        quadTransform.rotation = -45.f;
        quadTransform.scale = { 300.f, 300.f };
        Engine->DrawQuad(2, quadTransform);

        Engine->DrawQuadsOffsetScale(quadTextureIndexes.data(), quadOffsetScales.data(), quadCount);

        Engine->EndFrame();
        auto stop = std::chrono::high_resolution_clock::now();
        deltaTime = std::max((f32)(std::chrono::duration_cast<std::chrono::milliseconds>(stop - start)).count(), 0.5f);
        fps = 1.f / (deltaTime / 1000.f);
        std::cout << "FPS: " << fps << std::endl;
    }

    Engine->Cleanup();

    return 0;
}