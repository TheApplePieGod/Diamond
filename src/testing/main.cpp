#include "../diamond.h"
#include <iostream>

int main(int argc, char** argv)
{
    diamond Engine;
    
    Engine.Initialize(800, 600, "Diamond Test");

    while (Engine.IsRunning())
    {
        Engine.BeginFrame();
        Engine.EndFrame();
    }

    Engine.Cleanup();

    return 0;
}