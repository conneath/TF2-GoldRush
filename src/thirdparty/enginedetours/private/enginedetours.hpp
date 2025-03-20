#ifndef ENGINE_DETOURS_H
#define ENGINE_DETOURS_H
#pragma once

#if defined(LINUX)
#include <link.h>
#endif

#include <safetyhook.hpp>

class ConVar;

namespace enginedetours
{
	// Use this convar in your detour if you want to print extra debug information
	extern ConVar detours_extra_info;
	extern bool runningDedicatedServer;

	enum Library {
		ENGINE = 0,
		VGUIMATSURFACE,
	
		MAX_LIBS
	};


	void InitLibs();
	std::optional<safetyhook::InlineHook> CreateInlineHook(Library lib, const unsigned char pattern[], int pattern_size, void* detour_function, char* name);
};

#endif // ENGINE_DETOURS_H