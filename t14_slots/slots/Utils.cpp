#include <assert.h>
#include <Windows.h>
#include <time.h>

#include "Utils.h"

using namespace std;

static float clockSecs = 0;

void DebugPrint(const string& mssg1, const string& mssg2)
{
	OutputDebugString(mssg1.c_str());
	OutputDebugString("\n");
	if (!mssg2.empty()) {
		OutputDebugString(mssg2.c_str());
		OutputDebugString("\n");
	}
}

void Rnd::Seed(int val)
{
	if (val == -1)
		srand(static_cast<unsigned int>(time(NULL)));
	else
		srand(val);
}

int Rnd::GetRange(int min, int max)
{
	return (int)round( GetRange((float)min, (float)max) );
}

float Rnd::GetRange(float min, float max)
{
	assert(min < max);
	float alpha = (float)(rand()%RAND_MAX)/RAND_MAX;
	return min + (max - min)*alpha;
}

float GetClock()
{
	return clockSecs;
}

void AddSecsToClock(float secs)
{
	clockSecs += secs;
}

