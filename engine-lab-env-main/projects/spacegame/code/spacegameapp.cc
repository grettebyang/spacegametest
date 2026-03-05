//------------------------------------------------------------------------------
// spacegameapp.cc
// (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "config.h"
#include "spacegameapp.h"
#include <cstring>
#include "imgui.h"
#include "render/renderdevice.h"
#include "render/shaderresource.h"
#include <vector>
#include "render/textureresource.h"
#include "render/model.h"
#include "render/cameramanager.h"
#include "render/lightserver.h"
#include "render/debugrender.h"
#include "core/random.h"
#include "render/input/inputserver.h"
#include "core/cvar.h"
#include "render/physics.h"
#include <chrono>
#include "spaceship.h"
#include <unordered_map>
#include "componentbase.h"
#include "entity.h"
#include "core/idpool.h"
#include "render/particlesystem.h"
#include "world.h"
#include <enet/enet.h>
#include <iostream>

using namespace Display;
using namespace Render;

namespace Game
{

//------------------------------------------------------------------------------
/**
*/
SpaceGameApp::SpaceGameApp()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
SpaceGameApp::~SpaceGameApp()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
bool
SpaceGameApp::Open()
{
	App::Open();
	this->window = new Display::Window;
    this->window->SetSize(2500, 2000);

    if (this->window->Open())
	{
		// set clear color to gray
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

        RenderDevice::Init();

		// set ui rendering function
		this->window->SetUiRender([this]()
		{
			this->RenderUI();
		});

        
        return true;
	}
	return false;
}

//------------------------------------------------------------------------------
/**
*/
void
SpaceGameApp::Run()
{
    World *world = World::GetInstance();

    int w;
    int h;
    this->window->GetSize(w, h);
    glm::mat4 projection = glm::perspective(glm::radians(90.0f), float(w) / float(h), 0.01f, 1000.f);
    Camera* cam = CameraManager::GetCamera(CAMERA_MAIN);
    cam->projection = projection;

    // load all resources
    ModelId models[6] = {
        LoadModel("assets/space/Asteroid_1.glb"),
        LoadModel("assets/space/Asteroid_2.glb"),
        LoadModel("assets/space/Asteroid_3.glb"),
        LoadModel("assets/space/Asteroid_4.glb"),
        LoadModel("assets/space/Asteroid_5.glb"),
        LoadModel("assets/space/Asteroid_6.glb")
    };
    Physics::ColliderMeshId colliderMeshes[6] = {
        Physics::LoadColliderMesh("assets/space/Asteroid_1_physics.glb"),
        Physics::LoadColliderMesh("assets/space/Asteroid_2_physics.glb"),
        Physics::LoadColliderMesh("assets/space/Asteroid_3_physics.glb"),
        Physics::LoadColliderMesh("assets/space/Asteroid_4_physics.glb"),
        Physics::LoadColliderMesh("assets/space/Asteroid_5_physics.glb"),
        Physics::LoadColliderMesh("assets/space/Asteroid_6_physics.glb")
    };

    std::vector<std::tuple<ModelId, Physics::ColliderId, glm::mat4>> asteroids;
    
    // Setup asteroids near
    for (int i = 0; i < 100; i++)
    {
        size_t resourceIndex = (size_t)(Core::FastRandom() % 6);
        float span = 40.0f;
        glm::vec3 translation = glm::vec3(
            Core::RandomFloatNTP() * span,
            Core::RandomFloatNTP() * span,
            Core::RandomFloatNTP() * span
        );
        glm::vec3 rotationAxis = normalize(translation);
        float rotation = translation.x;
        glm::mat4 transform = glm::rotate(rotation, rotationAxis) * glm::translate(translation);

        Entity* ast = world->AddEntity();
        TransformComponent* tc = ast->AddComponent<TransformComponent>();
        tc->itransform = transform;
        tc->iposition = translation;
        ConstantRotationComponent* crc = ast->AddComponent<ConstantRotationComponent>();
        CollisionMeshComponent* cmc = ast->AddComponent<CollisionMeshComponent>();
        cmc->collider = Physics::CreateCollider(colliderMeshes[resourceIndex], transform, 1, ast);
        RenderModelComponent* rmc = ast->AddComponent<RenderModelComponent>();
        rmc->model = models[resourceIndex];
        ast->InitializeComponents();
    }

    // Setup asteroids far
    for (int i = 0; i < 50; i++)
    {
        size_t resourceIndex = (size_t)(Core::FastRandom() % 6);
        float span = 100.0f;
        glm::vec3 translation = glm::vec3(
            Core::RandomFloatNTP() * span,
            Core::RandomFloatNTP() * span,
            Core::RandomFloatNTP() * span
        );
        glm::vec3 rotationAxis = normalize(translation);
        float rotation = translation.x;
        glm::mat4 transform = glm::rotate(rotation, rotationAxis) * glm::translate(translation);

        Entity* ast = world->AddEntity();
        TransformComponent* tc = ast->AddComponent<TransformComponent>();
        tc->itransform = transform;
        tc->iposition = translation;
        ConstantRotationComponent* crc = ast->AddComponent<ConstantRotationComponent>();
        CollisionMeshComponent* cmc = ast->AddComponent<CollisionMeshComponent>();
        cmc->collider = Physics::CreateCollider(colliderMeshes[resourceIndex], transform, 1, ast);
        RenderModelComponent* rmc = ast->AddComponent<RenderModelComponent>();
        rmc->model = models[resourceIndex];
        ast->InitializeComponents();
    }

    // Setup skybox
    std::vector<const char*> skybox
    {
        "assets/space/bg.png",
        "assets/space/bg.png",
        "assets/space/bg.png",
        "assets/space/bg.png",
        "assets/space/bg.png",
        "assets/space/bg.png"
    };
    TextureResourceId skyboxId = TextureResource::LoadCubemap("skybox", skybox, true);
    RenderDevice::SetSkybox(skyboxId);
    
    Input::Keyboard* kbd = Input::GetDefaultKeyboard();

    const int numLights = 40;
    Render::PointLightId lights[numLights];
    // Setup lights
    for (int i = 0; i < numLights; i++)
    {
        glm::vec3 translation = glm::vec3(
            Core::RandomFloatNTP() * 20.0f,
            Core::RandomFloatNTP() * 20.0f,
            Core::RandomFloatNTP() * 20.0f
        );
        glm::vec3 color = glm::vec3(
            Core::RandomFloat(),
            Core::RandomFloat(),
            Core::RandomFloat()
        );
        lights[i] = Render::LightServer::CreatePointLight(translation, color, Core::RandomFloat() * 4.0f, 1.0f + (15 + Core::RandomFloat() * 10.0f));
    }

    //Add ship entity
    Entity* ship = world->AddEntity();
    //Add components to entity
    TransformComponent* tc = ship->AddComponent<TransformComponent>();
    ship->AddComponent<ThrusterComponent>();
    LaserComponent* lc = ship->AddComponent<LaserComponent>();
    lc->destructive = isServer;
    RigidBodyComponent* rbc = ship->AddComponent<RigidBodyComponent>();
    rbc->colliderEndPoints = std::vector<glm::vec3>{
        glm::vec3(1.40173, 0.0, -0.225342),  // left wing back
        glm::vec3(1.33578, 0.0, 0.088893),  // left wing front
        glm::vec3(0.227107, -0.200232, -0.588618),  // left back engine bottom
        glm::vec3(0.227107, 0.228809, -0.588618),  // left back engine top
        glm::vec3(0.391073, -0.130853, 1.28339),  // left weapon
        glm::vec3(0.134787, 0.0, 1.68965),  // left front
        glm::vec3(0.134787, 0.250728, 0.647422),  // left wind shield

        glm::vec3(-1.40173, 0.0, -0.225342),  // right wing back
        glm::vec3(-1.33578, 0.0, 0.088893),  // right wing front
        glm::vec3(-0.227107, -0.200232, -0.588618),  // right back engine bottom
        glm::vec3(-0.227107, 0.228809, -0.588618),  // right back engine top
        glm::vec3(-0.391073, -0.130853, 1.28339),  // right weapon
        glm::vec3(-0.134787, 0.0, 1.68965),  // right front
        glm::vec3(-0.134787, 0.250728, 0.647422),  // right wind shield

        glm::vec3(0.0, 0.525049, -0.392836),  // top back
        glm::vec3(0.0, 0.739624, 0.102582),  // top fin
        glm::vec3(0.0, -0.244758, 0.284825),  // bottom
    };

    CollisionMeshComponent* cmc = ship->AddComponent<CollisionMeshComponent>();
    cmc->collider = Physics::CreateCollider(world->shipCollider, tc->itransform, 1, ship);
    cmc->staticCollider = false;

    ship->AddComponent<CameraComponent>();

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
    rmc->model = world->shipModel;

    //always last in components list:
    PackerComponent* pc = ship->AddComponent<PackerComponent>();

    ship->InitializeComponents();


    playerPacker = pc;

    std::clock_t c_start = std::clock();
    double dt = 0.01667f;

    // game loop
    while (this->window->IsOpen())
	{
        auto timeStart = std::chrono::steady_clock::now();
		glClear(GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
        
        this->window->Update();

        if (kbd->pressed[Input::Key::Code::End])
        {
            ShaderResource::ReloadShaders();
        }

        //ENet
        if (host != nullptr)
        {
            (this->*NWPackets)();
        }

        //iterate through all entities and update them

        world->UpdateEntities(dt);

        // Draw some debug text
        Debug::DrawDebugText("FOOBAR", glm::vec3(0), {1,0,0,1});

        // Execute the entire rendering pipeline
        RenderDevice::Render(this->window, dt);

		// transfer new frame to window
		this->window->SwapBuffers();

        auto timeEnd = std::chrono::steady_clock::now();
        dt = std::min(0.04, std::chrono::duration<double>(timeEnd - timeStart).count());

        if (kbd->pressed[Input::Key::Code::Escape])
            this->Exit();
	}
}

//------------------------------------------------------------------------------
/**
*/
void
SpaceGameApp::Exit()
{
    if ((this->*NWDisconnect)() == EXIT_SUCCESS)
    {
        this->window->Close();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
SpaceGameApp::RenderUI()
{
	if (this->window->IsOpen())
	{
        ImGui::Begin("Debug");
        Core::CVar* r_draw_light_spheres = Core::CVarGet("r_draw_light_spheres");
        int drawLightSpheres = Core::CVarReadInt(r_draw_light_spheres);
        if (ImGui::Checkbox("Draw Light Spheres", (bool*)&drawLightSpheres))
            Core::CVarWriteInt(r_draw_light_spheres, drawLightSpheres);
        
        Core::CVar* r_draw_light_sphere_id = Core::CVarGet("r_draw_light_sphere_id");
        int lightSphereId = Core::CVarReadInt(r_draw_light_sphere_id);
        if (ImGui::InputInt("LightSphereId", (int*)&lightSphereId))
            Core::CVarWriteInt(r_draw_light_sphere_id, lightSphereId);
        
        ImGui::End();

        //Network
        ImGui::SetNextWindowSize(ImVec2(300, 120));
        ImGui::Begin("Network");

        Core::CVar* r_draw_network_address = Core::CVarGet("r_draw_network_address");
        static char networkAddress[16] = "";
        ImGui::Checkbox("Host", &isServer);
        //ImGui::Text((const char*)address.host);
        ImGui::InputText("Network Address", (char*)&networkAddress, 16);

        if (ImGui::Button("Connect"))
        {
            //Connect to this address
            if (!isServer)
            {
        	    SetupClient(networkAddress);
            }
            else
            {
        	    SetupServer();
            }
        }

        ImGui::End();

        Debug::DispatchDebugTextDrawing();
	}
}

int
SpaceGameApp::SetupClient(const char* adrss)
{
    isServer = false;
    //initialize enet library
    if (enet_initialize() != 0)
    {
        fprintf(stderr, "An error occurred while initializing ENet.\n");
        return EXIT_FAILURE;
    }
    //clean up used enet resources
    atexit(enet_deinitialize);

    host = enet_host_create(NULL, 1, 1, 0, 0);

    if (host == NULL)
    {
        fprintf(stderr, "An error occurred while trying to create an ENet client host.\n");
        return EXIT_FAILURE;
    }

    enet_address_set_host(&address, adrss);
    address.port = 7777;

    peer = enet_host_connect(host, &address, 1, 0);
    if (peer == NULL)
    {
        fprintf(stderr, "No available peers for initiating an ENet connection.\n");
        return EXIT_FAILURE;
    }

    if (enet_host_service(host, &event, 5000) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT)
    {
        puts("Connection to 127.0.0.1:7777 succeeded.");
    }
    else
    {
        enet_peer_reset(peer);
        puts("Connection to 127.0.0.1:7777 failed,");
        return EXIT_SUCCESS;
    }

    NWDisconnect = &SpaceGameApp::DisconnectClient;
    NWPackets = &SpaceGameApp::SendPackets;

    //Change the collision mask on the ship
    Entity* e = playerPacker->owner;
    for (int i = 0; i < e->components.size(); i++)
    {
        if (e->components[i]->Id() == (uint32_t)'CMCC')
        {
            e->DeleteComponent(i);
        }
    }

    return 1;
}

int
SpaceGameApp::SetupServer()
{
    isServer = true;
    //initialize enet library
    if (enet_initialize() != 0)
    {
        fprintf(stderr, "An error occurred while initializing ENet.\n");
        return EXIT_FAILURE;
    }
    atexit(enet_deinitialize);

    address.host = ENET_HOST_ANY;
    address.port = 7777;

    host = enet_host_create(&address, 32, 1, 0, 0);

    if (host == NULL)
    {
        fprintf(stderr, "An error occurred while tring to create an Enet server.\n");
        return EXIT_FAILURE;
    }
    NWDisconnect = &SpaceGameApp::DisconnectServer;
    NWPackets = &SpaceGameApp::ReceivePackets;

    //The ship needs to be added to the network entities list
    playerPacker->ready = true;
    //it will also need to be sent out to all clients/the server so that they can individually spawn them locally and keep track of them in their own network entities list
    World::GetInstance()->AddNetworkID(playerPacker->owner->id);

    return 1;
}

void
SpaceGameApp::ReceivePackets() //server
{
    while (enet_host_service(host, &event, 10) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_CONNECT:
            printf("A new client connected from %x:%u.\n",
                event.peer->address.host,
                event.peer->address.port);
            {
                // Add the client ship in server world
                int nwid = World::GetInstance()->AddNetworkID(World::GetInstance()->AddSpaceship(glm::mat4(1), glm::vec3(0, host->connectedPeers * 30, 30)));

                // send a packet to the newly connected client to reposition its ship and initialize it on the list of network entities
                //Game::SpawnPlayerMsg msg = playerPacker->GetDataPacket();
                Game::SpawnPlayerMsg msg;
                msg.head.msgType = 0x122; //this means it is initializing the ship for the first time
                msg.pos[0] = 0;
                msg.pos[1] = host->connectedPeers * 30;
                msg.pos[2] = 30;
                msg.transform = glm::mat4(1);
                msg.rotation = glm::identity<glm::quat>();
                msg.networkId = nwid;

                ENetPacket* packet = enet_packet_create(&msg, sizeof(SpawnPlayerMsg), ENET_PACKET_FLAG_RELIABLE);
                enet_peer_send(event.peer, 0, packet);

                //Send out a message to each client to spawn ship in 
            }
            break;
        case ENET_EVENT_TYPE_RECEIVE:
            /*printf("A packet of length %u containing %s was received from %x on channel %u.\n",
                event.packet->dataLength,
                event.packet->data,
                event.peer->address.host,
                event.peer->address.port,
                event.channelID);*/
            //do something with the packet
            {
                ENetPacket* packet = event.packet;
                SpawnPlayerMsg* msg = (SpawnPlayerMsg*)packet->data;
                int nwid = msg->networkId;
                PacketReceived(msg);
                enet_packet_destroy(packet);

                //send packets to peer of every entity in the list EXCEPT for this peer's player ship
                //vector<Game::SpawnPlayerMsg> msgs = World::GetInstance()->PackNetworkEntities(nwid);
                //for (int i = 0; i < msgs.size(); i++)
                //{
                //    SpawnPlayerMsg msg = msgs[i];
                //    ENetPacket* packet = enet_packet_create(&msg, sizeof(SpawnPlayerMsg), ENET_PACKET_FLAG_RELIABLE);
                //    enet_peer_send(event.peer, 0, packet);
                //}
            }
            break;
        case ENET_EVENT_TYPE_DISCONNECT:
            printf("%x:%u disconnnected.\n",
                event.peer->address.host,
                event.peer->address.port);
            //Some way to eliminate ships as players drop out?
            break;
        }
    }

    //byte recvBuffer[1024];

    //int bytesRecvd = enet_peer_receive(recvBuffer, 0);

    //MessageHeader* head = (MessageHeader*)recvBuffer;
    //switch (head->msgType)
    //{
    //case 0x123: //SpawnPlayerMsg
    //    SpawnPlayerMsg * msg = (SpawnPlayerMsg*)recvBuffer;
    //    break;
    //default:
    //    break;
    //}

    ////send a packet to all connected peers
    vector<Game::SpawnPlayerMsg> msgs = World::GetInstance()->PackNetworkEntities();
    for (int i = 0; i < msgs.size(); i++)
    {
        SpawnPlayerMsg msg = msgs[i];
        ENetPacket* packet = enet_packet_create(&msg, sizeof(SpawnPlayerMsg), ENET_PACKET_FLAG_RELIABLE);
        enet_host_broadcast(host, 0, packet);
    }

    //If laser is spawned
    Game::SpawnPlayerMsg lmsg = playerPacker->GetLaserSpawn();
    if (lmsg.head.msgType >= 0)
    {
        ENetPacket* laserPacket = enet_packet_create(&lmsg, sizeof(SpawnPlayerMsg), ENET_PACKET_FLAG_RELIABLE);
        enet_host_broadcast(host, 0, laserPacket);
    }

}
void
SpaceGameApp::PacketReceived(SpawnPlayerMsg* msg)
{
    switch (msg->head.msgType)
    {
    case 0x122: //initialize player ship
        playerPacker->InitializeFromPacket(msg);
        World::GetInstance()->AddNetworkIDClient(playerPacker->owner->id, msg->networkId);
        break;
    case 0x123: //SpawnPlayerMsg
        World::GetInstance()->AddSpaceship(msg->transform, msg->pos);
        break;
    case 0x124:
        //Update the position of the client ship on the server
        if(msg->networkId >= 0 && msg->networkId != playerPacker->nwid) //make sure the message is not about the player ship
        {
            World::GetInstance()->UpdateNetworkEntities(msg);
        }
        break;
    case 0x125:
        //Spawn a lasers
        World::GetInstance()->AddLaser(msg->transform, msg->rotation, msg->pos, isServer);
        if (isServer)
        {
            //The server is responsible for serving out the lasers to all clients
            enet_host_broadcast(host, 0, enet_packet_create(msg, sizeof(SpawnPlayerMsg), ENET_PACKET_FLAG_RELIABLE));
        }
        break;
    case 0x126:
        //Respawn this ship
        World::GetInstance()->RespawnShip(msg->networkId);
        break;
     default:
        break;
     }
}

void
SpaceGameApp::SendPackets() //client
{
    while (enet_host_service(host, &event, 10) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_RECEIVE:
           /* printf("A packet of length %u containing %s was received from %x on channel %u.\n",
                event.packet->dataLength,
                event.packet->data,
                event.peer->address.host,
                event.peer->address.port,
                event.channelID);*/
            {
                ENetPacket* packet = event.packet;
                SpawnPlayerMsg* msg = (SpawnPlayerMsg*)packet->data;
                PacketReceived(msg);
                enet_packet_destroy(packet);
            }
            break;
        }
    }

    //Send a packet if not respawning
    if (playerPacker->ready)
    {
        Game::SpawnPlayerMsg msg = playerPacker->GetDataPacket();
        ENetPacket* packet = enet_packet_create(&msg, sizeof(SpawnPlayerMsg), ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(peer, 0, packet);

        //If laser is spawned
        Game::SpawnPlayerMsg lmsg = playerPacker->GetLaserSpawn();
        if (lmsg.head.msgType >= 0)
        {
            ENetPacket* laserPacket = enet_packet_create(&lmsg, sizeof(SpawnPlayerMsg), ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(peer, 0, laserPacket);
        }
    }
}

int
SpaceGameApp::DisconnectClient()
{
    enet_peer_disconnect(peer, 0);

    while (enet_host_service(host, &event, 3000) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_RECEIVE:
            enet_packet_destroy(event.packet);
            break;
        case ENET_EVENT_TYPE_DISCONNECT:
            puts("Disconnection succeeded.");
            break;
        }
    }
    return EXIT_SUCCESS;
}

int
SpaceGameApp::DisconnectServer()
{
    enet_host_destroy(host);
    return EXIT_SUCCESS;
}
} // namespace Game
