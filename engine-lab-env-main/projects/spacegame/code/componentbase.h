#pragma once
#include "config.h"
#include "render/model.h"
#include "render/input/inputserver.h"
#include <queue>
#include <vector>
#include "render/cameramanager.h"
#include "render/physics.h"
#include "render/debugrender.h"
#include "render/particlesystem.h"
#include "spacegameapp.h"
using namespace glm;
using namespace Render;
using namespace Input;
using namespace std;
using namespace Physics;

class Entity;
struct SpawnPlayerMsg;

class ComponentBase
{
public:
    ComponentBase()
    {
        
    };
    virtual ~ComponentBase()
    {

    };
    virtual void Update(float dt)
    {

    };
    virtual void Start()
    {

    };
    virtual void Destroy()
    {

    };
    virtual uint32_t Id() = 0;
    int poolIndex;
    Entity *owner = nullptr;
};

template<typename comp>
class ComponentPool
{
public:
    ComponentPool()
    {
        for (int i = 0; i < 200; i++)
        {
            componentPool[i].poolIndex = i;
            freeComponent.push(i);
        }
    }
    ~ComponentPool()
    {
        delete[] componentPool;
    }
    void CreatePool()
    {

    }
    comp* GetFreeComponent()
    {
        // return a free component in the pool
        int freeComp = freeComponent.front();
        freeComponent.pop();
        return (comp*)&componentPool[freeComp];
    }
    void FreeComponent(int index)
    {
        // make the component stored at the index available
        freeComponent.push(index);
    }

private:
    /// array containing pre-allocated entities
    comp componentPool[200];
    /// stores freed indices
    std::queue<int> freeComponent;
};

class TransformComponent : public ComponentBase
{
public:
    ~TransformComponent()
    {

    };
    void Update(float dt)
    {

    };
    void Start()
    {
        position = iposition;
        orientation = iorientation;
        transform = itransform;
    };
    static TransformComponent* GetFreeComponent();
    void Destroy();
    uint32_t Id() { return 'TFCC'; }

    //Taken from spaceship class
    glm::vec3 iposition = glm::vec3(0);
    glm::quat iorientation = glm::identity<glm::quat>();
    glm::mat4 itransform = glm::mat4(1);

    glm::vec3 position = glm::vec3(0);
    glm::quat orientation = glm::identity<glm::quat>();
    glm::mat4 transform = glm::mat4(1);
};

class CollisionMeshComponent : public ComponentBase
{
public:
    ~CollisionMeshComponent()
    {

    };
    void Update(float dt)
    {
        //update the position of the collider so it follows the transform
        if (!staticCollider)
        {
            Physics::SetTransform(collider, transform->transform);
        }
    }
    void Start();
    static CollisionMeshComponent* GetFreeComponent();
    void Destroy();
    uint32_t Id() { return 'CMCC'; }
 
    TransformComponent* transform = nullptr;
    Physics::ColliderId collider = Physics::ColliderId();
    bool staticCollider = true;
};

class RenderModelComponent : public ComponentBase
{
public:
    ~RenderModelComponent()
    {

    };
    void Update(float dt)
    {
        RenderDevice::Draw(model, transform->transform);
    };
    void Start();
    static RenderModelComponent* GetFreeComponent();
    void Destroy();
    uint32_t Id() { return 'RMCC'; }

    Render::ModelId model = Render::ModelId();
    TransformComponent* transform = nullptr;
};

class ConstantRotationComponent : public ComponentBase
{
public:
    ~ConstantRotationComponent()
    {

    };
    void Update(float dt)
    {
        //Apply constant rotation
        transform->orientation = transform->orientation * spin;
        transform->transform = translate(transform->position) * (mat4)transform->orientation * (mat4)spin;
    };
    void Start();
    static ConstantRotationComponent* GetFreeComponent();
    void Destroy();
    uint32_t Id() { return 'PECC'; }

    TransformComponent* transform = nullptr;
    quat spin;
};

class CameraComponent : public ComponentBase
{
public:
    ~CameraComponent()
    {

    };
    void Update(float dt)
    {
        vec3 desiredCamPos = transform->position + vec3(transform->transform * vec4(0, camOffsetY, -4.0f, 0));
        this->camPos = mix(this->camPos, desiredCamPos, dt * cameraSmoothFactor);
        cam->view = lookAt(this->camPos, this->camPos + vec3(transform->transform[2]), vec3(transform->transform[1]));
    };
    void Start();
    void Destroy();
    static CameraComponent* GetFreeComponent();

    uint32_t Id() { return 'CCCC'; }

    // variables taken from spaceship class
    TransformComponent* transform = nullptr;
    Camera* cam = nullptr;
    glm::vec3 camPos = glm::vec3(0, 1.0f, -2.0f);
    const float camOffsetY = 1.0f;
    const float cameraSmoothFactor = 10.0f;
};

class RigidBodyComponent : public ComponentBase
{
public:
    ~RigidBodyComponent()
    {

    };
    void Update(float dt);
    void Start();
    void Destroy();
    static RigidBodyComponent* GetFreeComponent();
    bool CheckCollision()
    {
        glm::mat4 rotation = (glm::mat4)transform->orientation;
        bool hit = false;
        for (int i = 0; i < colliderEndPoints.size(); i++)
        {
            glm::vec3 pos = transform->position;
            glm::vec3 dir = transform->transform * glm::vec4(glm::normalize(colliderEndPoints[i]), 0.0f);
            float len = glm::length(colliderEndPoints[i]);
            Physics::RaycastPayload payload = Physics::Raycast(transform->position, dir, len, 1);

            // debug draw collision rays
            Debug::DrawLine(pos, pos + dir * len, 1.0f, glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1), Debug::RenderMode::AlwaysOnTop);

            if (payload.hit)
            {
                Debug::DrawDebugText("HIT", payload.hitPoint, glm::vec4(1, 1, 1, 1));
                hit = true;
            }
        }
        return hit;
    }
    uint32_t Id() { return 'RBCC'; }

    //Taken from spaceship class
    TransformComponent* transform = nullptr;
    const float normalSpeed = 1.0f;
    const float boostSpeed = normalSpeed * 2.0f;
    const float accelerationFactor = 1.0f;
    float currentSpeed = 0.0f;
    glm::vec3 linearVelocity = glm::vec3(0);
    //This should be a dynamic array of endpoints and data is passed in for each specific entity
    std::vector<glm::vec3> colliderEndPoints;

    bool collided;

};

class ParticleEmitterComponent : public ComponentBase
{
public:
    ~ParticleEmitterComponent()
    {

    };
    void Update(float dt)
    {
        float t = (rigidbody->currentSpeed / rigidbody->normalSpeed);
        for (int i = 0; i < particleEmitters.size(); i++)
        {
            particleEmitters[i]->data.origin = glm::vec4(vec3(transform->position + (vec3(transform->transform[0]) * ((-1 + 2*i) * thrusterPosOffset))) + (vec3(transform->transform[2]) * emitterOffset), 1);
            particleEmitters[i]->data.dir = glm::vec4(glm::vec3(-(transform->transform[2])), 0);

            particleEmitters[i]->data.startSpeed = 1.2 + (3.0f * t);
            particleEmitters[i]->data.endSpeed = 0.0f + (3.0f * t);
        }
    };
    void Start();
    void Destroy();
    static ParticleEmitterComponent* GetFreeComponent();
    uint32_t Id() { return 'PECC'; }

    TransformComponent* transform = nullptr;
    RigidBodyComponent* rigidbody = nullptr;
    std::vector<Render::ParticleEmitter*> particleEmitters;
    uint32_t numParticles;
    const float thrusterPosOffset = 0.365f;
    float emitterOffset = -0.5f;
};

class LaserComponent : public ComponentBase
{
public:
    ~LaserComponent()
    {

    };
    void Update(float dt);
    void Start();
    void Destroy();
    static LaserComponent* GetFreeComponent();
    uint32_t Id() { return 'LSCC'; }

    //Taken from spaceship class
    TransformComponent* transform = nullptr;
    Keyboard* kbd = Input::GetDefaultKeyboard();
    const float normalSpeed = 1.0f;
    vec3 direction;
    bool fired;
    bool destructive;
};

class ProjectileComponent : public ComponentBase
{
public:
    ~ProjectileComponent()
    {

    };
    void Update(float dt);
    void Start();
    void Destroy();
    static ProjectileComponent* GetFreeComponent();
    uint32_t Id() { return 'PJCC'; }

    //Taken from spaceship class
    TransformComponent* transform = nullptr;
    vec3 velocity;
    Physics::RaycastPayload ray;
    float time;
    bool destructive; //only lasers spawned on the server will be flagged as "destructive" lasers, lasers spawned on clients have no authority to destroy
};

class RespawnComponent : public ComponentBase
{
public:
    RespawnComponent() : ComponentBase()
    {
        explosionEmitter = new Render::ParticleEmitter(2048);
    }
    ~RespawnComponent()
    {

    };
    void Update(float dt)
    {
        if (!respawning && rigidbody->collided)
        {
            Respawn();
        }
        else if (respawning)
        {
            respawnTimer = std::max(respawnTimer - dt, 0.0f);
            if (respawnTimer == 0.0f)
            {
                //Reset position of ship
                transform->Start();
                respawning = false;
                ready = true;

                //Reset explosion
                ParticleSystem::Instance()->RemoveEmitter(explosionEmitter);
                explosionEmitter->data.startSpeed = 0.0f;
                explosionEmitter->data.endSpeed = 0.0f;
            }
        }

    };
    void Respawn()
    {
        respawnTimer = 2;
        respawning = true;

        //Explosion
        ParticleSystem::Instance()->AddEmitter(explosionEmitter);
        explosionEmitter->data.randomTimeOffsetDist = .5f;
        explosionEmitter->data.origin = glm::vec4(vec3(transform->position), 1);
        explosionEmitter->data.looping = 1;
        explosionEmitter->data.decayTime = .5;

        explosionEmitter->data.startSpeed = 1.2 + (30.0f);
        explosionEmitter->data.endSpeed = 0.1f;
    }
    void Start();
    void Destroy();
    static RespawnComponent* GetFreeComponent();
    uint32_t Id() { return 'RSCC'; }

    RigidBodyComponent* rigidbody = nullptr;
    TransformComponent* transform = nullptr;
    Render::ParticleEmitter* explosionEmitter;
    float respawnTimer = 0.0f;
    bool respawning = false;
    bool ready = false;
};

class ThrusterComponent : public ComponentBase
{
public:
    ~ThrusterComponent()
    {

    };
    void Update(float dt)
    {
        if (!respawn->respawning)
        {
            if (kbd->held[Key::W])
            {
                if (kbd->held[Key::Shift])
                    rigidbody->currentSpeed = mix(rigidbody->currentSpeed, this->boostSpeed, std::min(1.0f, dt * 30.0f));
                else
                    rigidbody->currentSpeed = mix(rigidbody->currentSpeed, this->normalSpeed, std::min(1.0f, dt * 90.0f));
            }
            else
            {
                rigidbody->currentSpeed = 0;
            }

            float rotX = kbd->held[Key::Left] ? 1.0f : kbd->held[Key::Right] ? -1.0f : 0.0f;
            float rotY = kbd->held[Key::Up] ? -1.0f : kbd->held[Key::Down] ? 1.0f : 0.0f;
            float rotZ = kbd->held[Key::A] ? -1.0f : kbd->held[Key::D] ? 1.0f : 0.0f;

            const float rotationSpeed = 1.8f * dt;
            rotXSmooth = mix(rotXSmooth, rotX * rotationSpeed, dt * cam->cameraSmoothFactor);
            rotYSmooth = mix(rotYSmooth, rotY * rotationSpeed, dt * cam->cameraSmoothFactor);
            rotZSmooth = mix(rotZSmooth, rotZ * rotationSpeed, dt * cam->cameraSmoothFactor);
            quat localOrientation = quat(vec3(-rotYSmooth, rotXSmooth, rotZSmooth));
            transform->orientation = transform->orientation * localOrientation;
            this->rotationZ -= rotXSmooth;
            this->rotationZ = clamp(this->rotationZ, -45.0f, 45.0f);
            mat4 T = translate(transform->position) * (mat4)transform->orientation;
            transform->transform = T * (mat4)quat(vec3(0, 0, rotationZ));
            this->rotationZ = mix(this->rotationZ, 0.0f, dt * cam->cameraSmoothFactor);
        }
    };
    void Start();
    void Destroy();
    static ThrusterComponent* GetFreeComponent();
    uint32_t Id() { return 'TCCC'; }

    //Taken from spaceship class
    RigidBodyComponent* rigidbody = nullptr;
    TransformComponent* transform = nullptr;
    CameraComponent* cam = nullptr;
    RespawnComponent* respawn = nullptr;
    Keyboard* kbd = Input::GetDefaultKeyboard();
    const float normalSpeed = 1.0f;
    const float boostSpeed = normalSpeed * 2.0f;
    float rotationZ = 0;
    float rotXSmooth = 0;
    float rotYSmooth = 0;
    float rotZSmooth = 0;
}; 

class PackerComponent : public ComponentBase
{
public:
    ~PackerComponent()
    {

    };
    void Update(float dt)
    {
        // Pack data into a shippable package
        if (respawn->ready)
        {
            ready = true;
            respawn->ready = false;
        }
    };
    Game::SpawnPlayerMsg GetDataPacket();
    void UpdateFromPacket(Game::SpawnPlayerMsg* msg)
    {
        transform->position = msg->pos;
        transform->orientation = msg->rotation;
        transform->transform = msg->transform;
    }
    void InitializeFromPacket(Game::SpawnPlayerMsg* msg)
    {
        transform->iposition = msg->pos;
        transform->iorientation = msg->rotation;
        transform->itransform = msg->transform;
        transform->position = msg->pos;
        transform->orientation = msg->rotation;
        transform->transform = msg->transform;
        nwid = msg->networkId;
        ready = true;
    }
    Game::SpawnPlayerMsg GetLaserSpawn();
    void Respawn()
    {
        //ready = false;
        respawn->Respawn();
    }
    void Start();
    void Destroy();
    static PackerComponent* GetFreeComponent();
    uint32_t Id() { return 'PPCC'; }

    TransformComponent* transform = nullptr;
    LaserComponent* laser = nullptr;
    RespawnComponent* respawn = nullptr;
    int nwid;
    bool ready = false;
};