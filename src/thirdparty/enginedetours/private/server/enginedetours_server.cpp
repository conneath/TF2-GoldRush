#include <enginedetours.hpp>

typedef void (*DetourInitFn)(void);

void ActivateServerEngineDetours()
{
	const DetourInitFn detours[] = {};

	enginedetours::InitLibs();

	for (auto init_func : detours)
	{
		init_func();
	}
}
