#include "entity.h"
#include "componentbase.h"
#include "world.h"

void Entity::Update(float dt)
{
    for (int i = 0; i < components.size(); i++)
    {
        components[i]->Update(dt);
    }
}

ComponentBase* Entity::GetComponent(uint32_t compId)
{
    for (int i = 0; i < components.size(); i++)
    {
        if (components[i]->Id() == compId)
        {
            return components[i];
        }
    }
    return nullptr;
}

void Entity::InitializeComponents()
{
    for (int i = 0; i < components.size(); i++)
    {
        components[i]->Start();
    }
}

void Entity::DestroySelf()
{
    for (int i = 0; i < components.size(); i++)
    {
        components[i]->Destroy();
    }
    World::GetInstance()->DeleteEntity(this);
}

void Entity::DeleteComponent(int index)
{
    components[index]->Destroy();
    erase(components, components[index]);
}