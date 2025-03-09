#include <enginedetours.hpp>
#include "fontshadow.hpp"

typedef void (*DetourInitFn)(void);

void ActivateClientEngineDetours()
{
	const DetourInitFn detours[] = {InitFontShadowDetour};

	enginedetours::InitLibs();

	for (auto init_func : detours)
	{
		init_func();
	}
}
