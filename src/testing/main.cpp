#include "../diamond.h"
#include <iostream>
#include "../util/defs.h"
#include <gtc/matrix_transform.hpp>

int main(int argc, char** argv)
{
    diamond* Engine = new diamond();

    Engine->Initialize(800, 600, "Diamond Test");

    Engine->RegisterTexture("../images/test.png");
    Engine->RegisterTexture("../images/test2.png"); 
    Engine->SyncTextureUpdates();

    while (Engine->IsRunning())
    {
        Engine->BeginFrame(camera_mode::Orthographic);

        std::vector<vertex> vertices =
        {
            {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, -1},
            {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, -1},
            {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, -1},
            {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, -1}
        };
        const std::vector<u16> indices =
        {
            0, 1, 2, 2, 3, 0
        };
        Engine->BindVertices(vertices.data(), vertices.size());
        Engine->BindIndices(indices.data(), indices.size());
        Engine->DrawIndexed(indices.size(), vertices.size(), 0, {{ 100.f, 100.f }, 45.f, { 500.f, 500.f }});
        
        vertices = 
        {
            {{-0.25f, -0.25f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, -1},
            {{0.25f, -0.25f}, {0.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, -1},
            {{0.25f, 0.25f}, {1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, -1},
            {{-0.25f, 0.25f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, -1}
        };
        Engine->BindVertices(vertices.data(), vertices.size());
        Engine->DrawIndexed(indices.size(), vertices.size(), 1, {{ 0.f, 0.f }, 0.f, { 500.f, 500.f }});

        Engine->EndFrame();
    }

    Engine->Cleanup();

    return 0;
}