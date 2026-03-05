#pragma once
//------------------------------------------------------------------------------
/**
	Space game application

	(C) 20222 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/app.h"
#include "render/window.h"
#include <enet/enet.h>
using namespace glm;

class PackerComponent;


namespace Game
{

struct MessageHeader
{
	int msgType;
};

struct SpawnPlayerMsg
{
	MessageHeader head;
	vec3 pos;
	quat rotation;
	mat4 transform;
	int networkId;
};

//void SendMessage()
//{
//	SpawnPlayerMessage msg;
//	msg.head.msgType = 0x123;
//	//msg.pos = ...
//
//	enet_send(&msg, sizeof(SpawnPlayerMsg), address);
//}

class SpaceGameApp : public Core::App
{
public:
	/// constructor
	SpaceGameApp();
	/// destructor
	~SpaceGameApp();

	// function pointers
	void (SpaceGameApp::* NWPackets)() {};
	int (SpaceGameApp::* NWDisconnect)() {};

	/// open app
	bool Open();
	/// run app
	void Run();
	/// exit app
	void Exit();
	/// setup client
	int SetupClient(const char* adrss);
	/// setup server
	int SetupServer();
	/// server loop
	void ReceivePackets();
	/// client loop
	void SendPackets();
	/// server disconnect
	int DisconnectServer();
	/// client disconnect
	int DisconnectClient();
	/// packet received
	void PacketReceived(SpawnPlayerMsg* msg);

	//player ship
	PackerComponent* playerPacker;

	bool isServer;

private:

	/// show some ui things
	void RenderUI();

	Display::Window* window;

	// enet

	ENetAddress address;
	ENetEvent event;
	ENetPeer* peer; //server we are connecting to if we are client
	ENetHost* host = nullptr;
};
} // namespace Game