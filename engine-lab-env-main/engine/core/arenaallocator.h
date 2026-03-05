#pragma once
#include<stdlib.h>
#include<iostream>
using namespace std;

class ArenaAllocator
{
private:
	void* arena;
	size_t arenaSize;
	size_t pointer;

public:
	ArenaAllocator(size_t size)
	{
		pointer = 0;
		arena = malloc(size);
		arenaSize = size;
	};

	~ArenaAllocator()
	{
		cout << "Cleared arena" << endl;
		free(arena);
	}

	template<typename TYPE>
	TYPE* Allocate()
{
		const size_t size = sizeof(TYPE);
		const size_t allignment = alignof(TYPE);
		//reallign if necessary
		size_t allignAmnt = pointer % allignment;
		if (allignAmnt != 0)
		{
			allignAmnt = allignment - allignAmnt;
		}
		//Check if it is over capacity
		if (size + allignAmnt + pointer > arenaSize)
		{
			cout << "Arena is full" << endl;
			return NULL;
		}

		void* ptr = (char*)arena + pointer + allignAmnt;
		pointer += allignAmnt + size;
		cout << "Allocated " << ptr << endl;
		return (TYPE*)ptr;
	}

	void* Allocate(size_t size, size_t allignment)
	{
		//reallign if necessary
		size_t allignAmnt = pointer % allignment;
		if (allignAmnt != 0)
		{
			allignAmnt = allignment - allignAmnt;
		}
		//Check if it is over capacity
		if (size + allignAmnt + pointer > arenaSize)
		{
			cout << "Arena is full" << endl;
			return NULL;
		}

		void* ptr = (char*)arena + pointer + allignAmnt;
		pointer += allignAmnt + size;
		cout << "Allocated " << ptr << endl;
		return ptr;
	}

	void Reset() 
	{
		cout << "Reset arena" << endl;
		pointer = 0;
	}
};