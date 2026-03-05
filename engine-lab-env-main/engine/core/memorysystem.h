#pragma once
#include<stdlib.h>
#include<iostream>

using namespace std;

class Memory
{
public:
	Memory() { newCount = 0; deleteCount = 0; };
	void AddNew() { newCount++; };
	void AddDelete() { deleteCount++; };
	void CheckForMemoryLeaks()
	{
		if (newCount == deleteCount)
		{
			cout << "No memory leaks!";
		}
		else
		{
			cout << "MEMORY LEAK(S) DETECTED" << deleteCount << "\n";
			cout << "Number of allocations: " << newCount << "\n";
			cout << "Nubmer of deallocations: " << deleteCount << "\n";
		}
	}
private:
	int newCount;
	int deleteCount;
};