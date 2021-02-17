#include "../diamond.h"
#include <iostream>
#include "../util/defs.h"

int main(int argc, char** argv)
{
    diamond* Engine = new diamond();
    
    Engine->Initialize(800, 600, "Diamond Test");

    while (Engine->IsRunning())
    {
        Engine->BeginFrame();

        std::vector<vertex> vertices =
        {
            {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
            {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
            {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
        };
        const std::vector<u16> indices = {
            0, 1, 2, 2, 3, 0
        };
        Engine->BindVertices(vertices.data(), vertices.size());
        Engine->BindIndices(indices.data(), indices.size());
        Engine->DrawIndexed(indices.size(), vertices.size());

        vertices = 
        {
            {{-0.25f, -0.25f}, {1.0f, 1.0f, 0.0f}},
            {{0.25f, -0.25f}, {0.0f, 1.0f, 1.0f}},
            {{0.25f, 0.25f}, {1.0f, 0.0f, 1.0f}},
            {{-0.25f, 0.25f}, {1.0f, 1.0f, 1.0f}}
        };
        Engine->BindVertices(vertices.data(), vertices.size());
        Engine->DrawIndexed(indices.size(), vertices.size());

        Engine->EndFrame();
    }

    Engine->Cleanup();

    return 0;
}