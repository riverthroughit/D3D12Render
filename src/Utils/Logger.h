#pragma once

#include <cstdio>
#include <debugapi.h>

class Logger
{
public:
	static void LogToOutput(char* Text)
	{
		OutputDebugStringA(Text);
	}
};
