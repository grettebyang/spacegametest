#include "entity.h"
#include "componentbase.h"
#include "core/random.h"
#include "world.h"
#include "render/particlesystem.h"
#include <iostream>
#include "render/physics.h"
#include "spacegameapp.h"

struct SpawnPlayerMsg;

//Start functions

void CollisionMeshComponent::Start()
{
    transform = (TransformComponent*)owner->GetComponent('TFCC');
}

void RenderModelComponent::Start()
{
    transform = (TransformComponent*)owner->GetComponent('TFCC');
}

void CameraComponent::Start()
{
    cam = CameraManager::GetCamera(CAMERA_MAIN);
    transform = (TransformComponent*)owner->GetComponent('TFCC');
};

void RigidBodyComponent::Start()
{
    transform = (TransformComponent*)owner->GetComponent('TFCC');
};

void ParticleEmitterComponent::Start()
{
    transform = (TransformComponent*)owner->GetComponent('TFCC');
    rigidbody = (RigidBodyComponent*)owner->GetComponent('RBCC');
    for (int i = 0; i < particleEmitters.size(); i++)
    {
        particleEmitters[i]->data = {
            .origin = glm::vec4(transform->position + (vec3(transform->transform[2]) * emitterOffset),1),
            .dir = glm::vec4(glm::vec3(-transform->transform[2]), 0),
            .startColor = glm::vec4(0.38f, 0.76f, 0.95f, 1.0f) * 2.0f,
            .endColor = glm::vec4(0,0,0,1.0f),
            .numParticles = numParticles,
            .theta = glm::radians(0.0f),
            .startSpeed = 1.2f,
            .endSpeed = 0.0f,
            .startScale = 0.025f,
            .endScale = 0.0f,
            .decayTime = 2.58f,
            .randomTimeOffsetDist = 2.58f,
            .looping = 1,
            .emitterType = 1,
            .discRadius = 0.020f
        };
    }
};

void ConstantRotationComponent::Start()
{
    transform = (TransformComponent*)owner->GetComponent('TFCC');
    //Randomize rotation
    spin = quat(.005f * normalize(vec3(Core::RandomFloatNTP(), Core::RandomFloatNTP(), Core::RandomFloatNTP())));
};

void ThrusterComponent::Start()
{
    cam = (CameraComponent*)owner->GetComponent('CCCC');
    rigidbody = (RigidBodyComponent*)owner->GetComponent('RBCC');
    transform = (TransformComponent*)owner->GetComponent('TFCC');
    respawn = (RespawnComponent*)owner->GetComponent('RSCC');
};

void LaserComponent::Start()
{
    transform = (TransformComponent*)owner->GetComponent('TFCC');
};

void ProjectileComponent::Start()
{
    time = 0;
    transform = (TransformComponent*)owner->GetComponent('TFCC');
};

void RespawnComponent::Start()
{
    rigidbody = (RigidBodyComponent*)owner->GetComponent('RBCC');
    transform = (TransformComponent*)owner->GetComponent('TFCC');
};

void PackerComponent::Start()
{
    transform = (TransformComponent*)owner->GetComponent('TFCC');
    laser = (LaserComponent*)owner->GetComponent('LSCC');
    respawn = (RespawnComponent*)owner->GetComponent('RSCC');
};

//Get free component functions

TransformComponent* TransformComponent::GetFreeComponent()
{
    return World::GetInstance()->transformPool.GetFreeComponent();
}

CollisionMeshComponent* CollisionMeshComponent::GetFreeComponent()
{
    return World::GetInstance()->collisionPool.GetFreeComponent();
}

RenderModelComponent* RenderModelComponent::GetFreeComponent()
{
    return World::GetInstance()->rendermodelPool.GetFreeComponent();
}

ConstantRotationComponent* ConstantRotationComponent::GetFreeComponent()
{
    return World::GetInstance()->constantrotationPool.GetFreeComponent();
}

CameraComponent* CameraComponent::GetFreeComponent()
{
    return World::GetInstance()->cameraPool.GetFreeComponent();
}

RigidBodyComponent* RigidBodyComponent::GetFreeComponent()
{
    return World::GetInstance()->rigidbodyPool.GetFreeComponent();
}

ThrusterComponent* ThrusterComponent::GetFreeComponent()
{
    return World::GetInstance()->thrusterPool.GetFreeComponent();
}

ParticleEmitterComponent* ParticleEmitterComponent::GetFreeComponent()
{
    return World::GetInstance()->particleemitterPool.GetFreeComponent();
}

LaserComponent* LaserComponent::GetFreeComponent()
{
    return World::GetInstance()->laserPool.GetFreeComponent();
}

ProjectileComponent* ProjectileComponent::GetFreeComponent()
{
    return World::GetInstance()->projectilePool.GetFreeComponent();
}

RespawnComponent* RespawnComponent::GetFreeComponent()
{
    return World::GetInstance()->respawnPool.GetFreeComponent();
}

PackerComponent* PackerComponent::GetFreeComponent()
{
    return World::GetInstance()->packerPool.GetFreeComponent();
}

//Destroy functions

void RigidBodyComponent::Destroy()
{
    World::GetInstance()->rigidbodyPool.FreeComponent(poolIndex);
}

void ThrusterComponent::Destroy()
{
    World::GetInstance()->thrusterPool.FreeComponent(poolIndex);
}

void ParticleEmitterComponent::Destroy()
{
    for (int i = 0; i < particleEmitters.size(); i++)
    {
        ParticleSystem::Instance()->RemoveEmitter(particleEmitters[i]);
    }
    World::GetInstance()->particleemitterPool.FreeComponent(poolIndex);
}

void TransformComponent::Destroy()
{
    return World::GetInstance()->transformPool.FreeComponent(poolIndex);
}

void CollisionMeshComponent::Destroy()
{
    Physics::DestroyCollider(collider);
    return World::GetInstance()->collisionPool.FreeComponent(poolIndex);
}

void RenderModelComponent::Destroy()
{
    return World::GetInstance()->rendermodelPool.FreeComponent(poolIndex);
}

void ConstantRotationComponent::Destroy()
{
    return World::GetInstance()->constantrotationPool.FreeComponent(poolIndex);
}

void CameraComponent::Destroy()
{
    return World::GetInstance()->cameraPool.FreeComponent(poolIndex);
}

void LaserComponent::Destroy()
{
    return World::GetInstance()->laserPool.FreeComponent(poolIndex);
}

void ProjectileComponent::Destroy()
{
    return World::GetInstance()->projectilePool.FreeComponent(poolIndex);
}

void RespawnComponent::Destroy()
{
    return World::GetInstance()->respawnPool.FreeComponent(poolIndex);
}

void PackerComponent::Destroy()
{
    return World::GetInstance()->packerPool.FreeComponent(poolIndex);
}

//Update functions

void RigidBodyComponent::Update(float dt)
{
    if (CheckCollision())
    {
        //Respawn ship after being destroyed
        collided = true;
   
        //Destroy self
        //owner->DestroySelf();
    }
    else
    {
        vec3 desiredVelocity = vec3(0, 0, currentSpeed);
        desiredVelocity = transform->transform * vec4(desiredVelocity, 0.0f);

        this->linearVelocity = mix(this->linearVelocity, desiredVelocity, dt * accelerationFactor);

        transform->position += this->linearVelocity * dt * 10.0f;

        collided = false;
    }
};

void LaserComponent::Update(float dt)
{
    fired = false;
    if (kbd->pressed[Key::Space])
    {
        //Fire laser along ship basis
        direction = transform->transform[2];
        normalize(direction);
        fired = true;

        //Add a laser entity with velocity direction * laserSpeed
        World::GetInstance()->AddLaser(transform->transform, transform->orientation, transform->position, destructive);
    }
};

void ProjectileComponent::Update(float dt)
{
    time += dt;
    //Move at constant velocity while checking for collision
    ray = Physics::Raycast(transform->position, velocity, velocity.length()*2, 1);
    if (ray.hit)
    {
        //if the collider is another ship, destroy the ship, then destroy the projectile
        if(destructive)
        {
            Entity* e = static_cast<Entity*>(ray.userData);
            //e->DestroySelf();
            for (int i = 0; i < e->components.size(); i++)
            {
                if (e->components[i]->Id() == (uint32_t)'RSCC')
                {
                    RespawnComponent* rsc = (RespawnComponent*)e->components[i];
                    rsc->Respawn();
                }
            }
        }
        owner->DestroySelf();
    }
    transform->position += velocity * dt;
    transform->transform = translate(transform->position) * (mat4)transform->orientation;

    //If gone "out of bounds" destroy anyway
    //for now just see if it's been going for long enough
    if (time >= 5.0f)
    {
        owner->DestroySelf();
    }
}

Game::SpawnPlayerMsg PackerComponent::GetDataPacket()
{
    Game::SpawnPlayerMsg msg;
    if (respawn->respawning)
    {
        msg.head.msgType = 0x126;
        ready = false; //this is so we don't send multiple respawn messages while the ship is respawning
    }
    else
    {
        msg.head.msgType = 0x124;
    }
    msg.pos = transform->position;
    msg.rotation = transform->orientation;
    msg.transform = transform->transform;
    msg.networkId = nwid;
    return msg;
};

Game::SpawnPlayerMsg PackerComponent::GetLaserSpawn()
{
    Game::SpawnPlayerMsg msg;
    if (laser->fired)
    {
        msg.head.msgType = 0x125;
        msg.pos = transform->position;
        msg.rotation = transform->orientation;
        msg.transform = transform->transform;
        return msg;
    }
    msg.head.msgType = -1;
    return msg;
}