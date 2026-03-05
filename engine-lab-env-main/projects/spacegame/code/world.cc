#include "world.h"

World* World::instance = nullptr;

World* World::GetInstance()
{
    if (instance == nullptr)
    {
        instance = new World;
    }
    return instance;
}