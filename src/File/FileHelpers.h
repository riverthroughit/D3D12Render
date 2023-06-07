#pragma once

#include <string>
#include <experimental/filesystem>
#include "Utils/FormatConvert.h"

class TFileHelpers
{
public:
	static bool IsFileExit(const std::wstring& FileName)
	{
		return std::experimental::filesystem::exists(FileName);
	}

	static std::wstring EngineDir()
	{
		std::wstring EngineDir = FormatConvert::StrToWStr(std::string(SOLUTION_DIR)) + L"Engine/";

		return EngineDir;
	}
};
