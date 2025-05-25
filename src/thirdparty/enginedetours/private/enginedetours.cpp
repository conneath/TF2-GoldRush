#include <enginedetours.hpp>

#include <Color.h>
#include <convar.h>
#include "tier0/dbg.h"

#include <cstring>
#include <optional>

#if defined(LINUX)
#include <dlfcn.h>
#include <linux/limits.h>
#endif

using namespace enginedetours;

ConVar enginedetours::detours_extra_info( "detours_extra_info", "1", FCVAR_ARCHIVE, "Detours will print additional debug information" );
bool enginedetours::runningDedicatedServer = false;

struct LibInfo
{
	size_t start = 0;
	size_t end = 0;

	void* FindPattern(const unsigned char pattern[], int pattern_size)
	{
		if (this->start == 0)
		{
			return nullptr;
		}
	
		for (size_t ptr = this->start; ptr < (this->end - pattern_size); ptr++)
		{
			for (int byte_index = 0; byte_index < pattern_size; byte_index++)
			{
				if(pattern[byte_index] == *(unsigned char*)(ptr + byte_index))
				{
					if(byte_index+1 == pattern_size) return (void*)ptr;
					continue;
				}
				else
				{
					break;
				}
			}
		}
		return nullptr;
	}
};

LibInfo libs[Library::MAX_LIBS];

const char* GetLibraryFilename(Library lib, bool dedicated)
{
#if defined(LINUX)
	#define PLATFORM_FILE_EXTENSION ".so"
#elif defined(WINDOWS)
	#define PLATFORM_FILE_EXTENSION ".dll"
#endif
	switch (lib)
	{
		case ENGINE:
		{
			if (dedicated)
			{
				return "engine_srv" PLATFORM_FILE_EXTENSION;
			}
			else
			{
				return "engine" PLATFORM_FILE_EXTENSION;
			}
		}
		case VGUIMATSURFACE: return "vguimatsurface" PLATFORM_FILE_EXTENSION;
		default: return nullptr;
	}
#undef PLATFORM_FILE_EXTENSION
}

#if defined(LINUX)
int dl_phdr_info_callback(dl_phdr_info* info, size_t size, void* data)
{
	const Color msg_color = Color(50, 255, 255, 255);
	const char* match = strrchr(info->dlpi_name, '/');
	if (match == nullptr) return 0;
	match += 1; // We don't care about the / in the path.

	for (int i = 0; i < MAX_LIBS; i++)
	{
		const char* filename = GetLibraryFilename(static_cast<Library>(i), false);
		const char* filename_srv = GetLibraryFilename(static_cast<Library>(i), true);
		bool matched = (strcmp(filename, match) == 0);
		bool matched_srv = (strcmp(filename_srv, match) == 0);
		if (matched || matched_srv)
		{
			if (matched_srv)
			{
				runningDedicatedServer = true;
			}

			for (int header_index = 0; header_index < info->dlpi_phnum; header_index++)
			{
				const ElfW(Phdr) header = info->dlpi_phdr[header_index];
				
				if (header.p_flags == (PF_R | PF_X))
				{
					libs[i].start = (info->dlpi_addr + header.p_vaddr);
					libs[i].end = (info->dlpi_addr + header.p_vaddr + header.p_memsz);
					ConColorMsg(msg_color, "Lib: %s | Start: %lx | End: %lx\n", filename, libs[i].start, libs[i].end);
				}
			}
		}
	}

	return 0;
}
#endif

void enginedetours::InitLibs()
{
#if defined(LINUX)
	dl_iterate_phdr(&dl_phdr_info_callback, NULL);
#endif
}

std::optional<safetyhook::InlineHook> enginedetours::CreateInlineHook(Library lib, const unsigned char pattern[], int pattern_size, void* detour_function, char* name)
{
	void* func = libs[lib].FindPattern(pattern, pattern_size);
	if (func == nullptr)
	{
		Warning("Could not find pattern in lib %s to create detour %s.\n", GetLibraryFilename(lib, runningDedicatedServer), name);
		return {};
	}

	auto result = safetyhook::InlineHook::create(func, detour_function);
	if(result.has_value())
	{
		return std::optional<safetyhook::InlineHook>{std::move(result.value())};
	}
	else
	{
		Warning("Failed to create hook in lib %s for detour %s.\n", GetLibraryFilename(lib, runningDedicatedServer), name);
		return {};
	}
}