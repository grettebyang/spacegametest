#pragma once
#include "entity.h"
#include "componentbase.h"
#include "core/idpool.h"
#include "spacegameapp.h"
using namespace std;
using namespace Game;

class World
{
private:
    World() {};
    ~World() 
    {
        for (int i = 0; i < activeEntities.size(); i++)
        {
            delete activeEntities[i];
        }
    };

    static World* instance;

    //Here is the list of entities to loop over, which will each have an id
    std::vector<Entity*> activeEntities;

    //Entity id pool
    Util::IdPool<EntityID> entityIdPool;

    //Network id pool
    //This contains key-value pairs of network ids and local entity ids
    //Anything that will have information communicated between client and server will have an id in this list
    std::unordered_map<int, EntityID> networkIds;
    int curNWID = 0;

    EntityPool entityPool;


public:
    World(const World& obj) = delete;

    Entity* GetEntity(int index)
    {
        return activeEntities[index];
    }

    Entity* AddEntity()
    {
        Entity* entity = entityPool.GetFreeEntity();
        EntityID id;
        if (entityIdPool.Allocate(id))
        {
            activeEntities.push_back(entity);
        }
        else
        {
            activeEntities[id.index] = entity;
        }
        entity->id = id;
        return entity;
    }

    void DeleteEntity(Entity* entity)
    {
        //Reset entity attributes and deallocate
        //activeEntities.erase(activeEntities.begin() + entity->id.index);
        entityIdPool.Deallocate(entity->id);
        entityPool.FreeEntity(entity->poolIndex);
    }

    static World* GetInstance();

    void UpdateEntities(double dt)
    {
        for (int i = 0; i < activeEntities.size(); i++)
        {
            if (entityIdPool.IsValid(activeEntities[i]->id))
            {
                activeEntities[i]->Update(dt);
            }
        }
    }

    void UpdateNetworkEntities(Game::SpawnPlayerMsg *msg)
    {
        //msg is a packet of info of a single entity
        //first check if the network id exists in the list, and if it doesn't, spawn it in and instantiate
        if (networkIds.find(msg->networkId) == networkIds.end()) // entity doesn't exist here yet so we have to spawn it in
        {
            //determine if it's a laser or a ship;

            //instantiate this entity
            EntityID id = AddSpaceship(msg->transform, msg->pos);
            networkIds.insert({ msg->networkId, id });
            Entity* e = activeEntities[id.index];
            PackerComponent* pc = (PackerComponent*)e->components[e->components.size() - 1];
            pc->InitializeFromPacket(msg);
        }
        else //entity does exist so we can just update it like normal
        {
            EntityID id = networkIds[msg->networkId];
            Entity* e = activeEntities[id.index];
            PackerComponent* pc = (PackerComponent*)e->components[e->components.size() - 1]; //packer component will always be last in list
            pc->UpdateFromPacket(msg);
        }
    }

    void RespawnShip(int nwid)
    {
        EntityID id = networkIds[nwid];
        Entity* e = activeEntities[id.index];
        PackerComponent* pc = (PackerComponent*)e->components[e->components.size() - 1]; //packer component will always be last in list
        pc->Respawn();
    }

    vector<Game::SpawnPlayerMsg> PackNetworkEntities()
    {
        vector<Game::SpawnPlayerMsg> msgs;
        for (auto i = networkIds.begin(); i != networkIds.end(); i++)
        {
            Entity* e = activeEntities[i->second.index];
            PackerComponent* pc = (PackerComponent*)e->components[e->components.size() - 1]; //packer component will always be last in list
            if (pc->ready)
            {
                Game::SpawnPlayerMsg msg = pc->GetDataPacket();
                //if (msg.head.msgType == 0x124 && i->first == excludeId)
                //{
                //    continue;
                //}
                msgs.push_back(msg);
            }
        }
        return msgs;
    }

    int AddNetworkID(EntityID id)
    {
        int nwid = curNWID;
        networkIds.insert({ nwid, id });
        Entity* e = activeEntities[id.index];
        PackerComponent* pc = (PackerComponent*)e->components[e->components.size() - 1];
        pc->nwid = nwid;
        curNWID++;
        return nwid;
    }

    void AddNetworkIDClient(EntityID eid, int nwid) 
    {
        networkIds.insert({ nwid, eid });
        Entity* e = activeEntities[eid.index];
        PackerComponent* pc = (PackerComponent*)e->components[e->components.size() - 1];
        pc->nwid = nwid;
    }

    //Add a spaceship from the server
    EntityID AddSpaceship(glm::mat4 transform, vec3 position)
    {
        Entity* ship = AddEntity();
        //Add components to entity
        TransformComponent* tc = ship->AddComponent<TransformComponent>();
        tc->itransform = transform * translate(position);
        tc->iposition = position;
        ship->AddComponent<RigidBodyComponent>();
        CollisionMeshComponent* cmc = ship->AddComponent<CollisionMeshComponent>();
        cmc->collider = Physics::CreateCollider(shipCollider, tc->itransform, 1, ship);
        cmc->staticCollider = false;

        uint32_t numParticles = 2048;
        Render::ParticleEmitter* particleEmitterLeft = new Render::ParticleEmitter(numParticles);
        Render::ParticleEmitter* particleEmitterRight = new Render::ParticleEmitter(numParticles);
        ParticleSystem::Instance()->AddEmitter(particleEmitterLeft);
        ParticleSystem::Instance()->AddEmitter(particleEmitterRight);

        ParticleEmitterComponent* pec = ship->AddComponent<ParticleEmitterComponent>();
        pec->particleEmitters.push_back(particleEmitterLeft);
        pec->particleEmitters.push_back(particleEmitterRight);
        pec->numParticles = numParticles;

        ship->AddComponent<RespawnComponent>();

        RenderModelComponent* rmc = ship->AddComponent<RenderModelComponent>();
        rmc->model = shipModel;
        
        PackerComponent* pc = ship->AddComponent<PackerComponent>();
        pc->ready = true;

        ship->InitializeComponents();

        return ship->id;
    }

    void AddLaser(glm::mat4 transform, glm::quat orientation, vec3 position, bool server)
    {
        Entity* laser = AddEntity();
        //Add components to entity
        TransformComponent* tc = laser->AddComponent<TransformComponent>();
        tc->itransform = translate(position) * (mat4)orientation;
        tc->iposition = position;
        tc->iorientation = orientation;

        ProjectileComponent* pc = laser->AddComponent<ProjectileComponent>();
        pc->velocity = transform[2] * 100.0f;
        pc->destructive = server;

        RenderModelComponent* rmc = laser->AddComponent<RenderModelComponent>();
        rmc->model = laserModel;

        laser->InitializeComponents();
    }

    ComponentPool<CollisionMeshComponent> collisionPool;
    ComponentPool<RenderModelComponent> rendermodelPool;
    ComponentPool<RigidBodyComponent> rigidbodyPool;
    ComponentPool<TransformComponent> transformPool;
    ComponentPool<CameraComponent> cameraPool;
    ComponentPool<ParticleEmitterComponent> particleemitterPool;
    ComponentPool<ThrusterComponent> thrusterPool;
    ComponentPool<ConstantRotationComponent> constantrotationPool;
    ComponentPool<LaserComponent> laserPool;
    ComponentPool<ProjectileComponent> projectilePool;
    ComponentPool<RespawnComponent> respawnPool;
    ComponentPool<PackerComponent> packerPool;

    Physics::ColliderMeshId shipCollider = Physics::LoadColliderMesh("assets/space/spaceship_col.glb");
    ModelId shipModel = LoadModel("assets/space/spaceship.glb");
    ModelId laserModel = LoadModel("assets/space/laser.glb");
};
