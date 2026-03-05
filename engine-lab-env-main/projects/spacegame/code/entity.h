#pragma once
#include "render/model.h"
#include "core/idpool.h"

class ComponentBase;

struct EntityID
{
    uint32_t index : 22; // 4M concurrent colliders
    uint32_t generation : 10; // 1024 generations per index

    constexpr static EntityID Create(uint32_t id)
    {
        EntityID ret{ id & 0x003FFFFF, (id & 0xFFC00000) >> 22 };
        return ret;
    }
    static EntityID Create(uint32_t index, uint32_t generation)
    {
        EntityID ret;
        ret.index = index;
        ret.generation = generation;
        return ret;
    }
    explicit constexpr operator uint32_t() const
    {
        return ((generation << 22) & 0xFFC00000ul) + (index & 0x003FFFFFul);
    }
    static constexpr EntityID Invalid()
    {
        return Create(0xFFFFFFFF);
    }
    constexpr uint32_t HashCode() const
    {
        return index;
    }
    const bool operator==(const EntityID& rhs) const { return uint32_t(*this) == uint32_t(rhs); }
    const bool operator!=(const EntityID& rhs) const { return uint32_t(*this) != uint32_t(rhs); }
    const bool operator<(const EntityID& rhs) const { return index < rhs.index; }
    const bool operator>(const EntityID& rhs) const { return index > rhs.index; }
};


class Entity
{
public:
    Entity() 
    {
        id = EntityID();
    };
    ~Entity()
    {
        for (int i = 0; i < components.size(); i++)
        {
            delete components[i];
        }
    };
    void Update(float dt);

    template <class compType>
    compType* AddComponent()
    {
        compType* cmp = compType::GetFreeComponent();
        cmp->owner = this;
        
        components.push_back(cmp);
        return cmp;
    }

    void DeleteComponent(int index);

    void InitializeComponents();
    ComponentBase* GetComponent(uint32_t compId);

    void DestroySelf();

    int poolIndex;
    EntityID id;
    std::vector<ComponentBase*> components;
};

class EntityPool
{
public:
    EntityPool()
    {
        for (int i = 0; i < 200; i++)
        {
            entityPool[i].poolIndex = i;
            freeEntities.push(i);
        }
    }
    ~EntityPool()
    {
        delete[] entityPool;
    }
    Entity* GetFreeEntity()
    {
        // return a free entity in the pool
        int freeEntity = freeEntities.front();
        freeEntities.pop();
        return &entityPool[freeEntity];
    }
    void FreeEntity(int index)
    {
        // make the entity stored at the index available
        // reset its components, they should be freed from their corresponding pools prior to this
        entityPool[index].components.clear();
        freeEntities.push(index);
    }
    // To free an entity, I would call: entities.FreeEntity(myEntity.poolIndex);

private:
    /// array containing pre-allocated entities
    Entity entityPool[200];
    /// stores freed indices
    std::queue<int> freeEntities;
};

