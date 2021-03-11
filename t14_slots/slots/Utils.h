#pragma once
#include <string>



/*
Send text to the debug output window
Second parameter can be ignored
*/
void DebugPrint(const std::string& mssg1, const std::string& mssg2 = "");

//seed and generate random numbers
struct Rnd
{
	static void Seed(int val = -1);
	static int GetRange(int min, int max);
	static float GetRange(float min, float max);
};

float GetClock();
void AddSecsToClock(float secs);
