#pragma once
#include "render/model.h"
#include "componentbase.h"
#include "entity.h"
#include <vector>
#include <queue>
using namespace Entities;

class EntityPool
{
public:
    EntityPool()
    {
        //
        for (int i = 0; i < 200; i++)
        {
            entityPool[i] = new Entity(i);
        }
    }
    Entity* GetFreeEntity()
    {
        // return a free entity in the pool
        int freeEntity = freeEntities.front();
        freeEntities.pop();
        return entityPool[freeEntity];
    }
    void FreeEntity(int index)
    {
        // make the entity stored at the index available
        // reset its components, they should be freed from their corresponding pools prior to this
        entityPool[index]->components.clear();
        freeEntities.push(index);
    }
    // To free an entity, I would call: entities.FreeEntity(myEntity.poolIndex);

private:
    /// array containing pre-allocated entities
    Entity* entityPool[200];
    /// stores freed indices
    std::queue<int> freeEntities;
};

