//------------------------------------------------------------------------------
// main.cc
// (C) 2015-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "config.h"
#include "spacegameapp.h"
#include "core/memorysystem.h"
#include "core/arenaallocator.h"
#include <enet/enet.h>
#include <iostream>

Memory memSystem;

void* operator new(size_t size)
{
	memSystem.AddNew();
	void* p = malloc(size);
	return p;
}

void* operator new[](size_t size)
{
	memSystem.AddNew();
	void* p = malloc(size);
	return p;
}

void operator delete(void* ptr)
{
	memSystem.AddDelete();
	free(ptr);
}

void operator delete[](void* ptr)
{
	memSystem.AddDelete();
	free(ptr);
}

int
main(int argc, const char** argv)
{
	Game::SpaceGameApp app;
	ArenaAllocator arena(65536);
	void* memory = arena.Allocate(sizeof(int), alignof(void*));
	int* mem2 = (int*)arena.Allocate(sizeof(int), alignof(int*));
	int* mem3 = (int*)arena.Allocate(sizeof(int), alignof(int*));
	int* mem4 = arena.Allocate<int>();
	cout << memory << endl;
	arena.Reset();

	if (app.Open())
	{
		//Is this client or server?
		//if (std::string(argv[1]) == "client")
		//{
		//	app.SetupClient("");
		//}
		//else if (std::string(argv[1]) == "server")
		//{
		//	app.SetupServer();
		//}

		app.Run();
		app.Close();
	}

	memSystem.CheckForMemoryLeaks();
	//app.Exit();
}