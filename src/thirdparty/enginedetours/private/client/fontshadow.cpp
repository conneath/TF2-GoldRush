#include <enginedetours.hpp>
#include <safetyhook.hpp>

#include <convar.h>
#include <VGuiMatSurface/IMatSystemSurface.h>

extern IMatSystemSurface *g_pMatSystemSurface;

safetyhook::InlineHook hook;

void detours_fontshadow_forced_offset_changed( IConVar *var, const char *pOldValue, float flOldValue )
{
	g_pMatSystemSurface->ResetFontCaches();
}
ConVar detours_fontshadow_forced_offset("detours_fontshadow_forced_offset", "0", FCVAR_ARCHIVE, "Forces a shadow offset. Use 2 or 3 for extra readability.", true, 0.0, true, 15.0, &detours_fontshadow_forced_offset_changed);

namespace
{
	#pragma pack(push, 1) // Valve actually uses a unsigned char array so this _needs_ to be packed
	struct Color
	{
		unsigned char r = 0;
		unsigned char g = 0;
		unsigned char b = 0;
		unsigned char a = 0;
	};
	#pragma pack(pop)

	void overlay_rgba(const Color* color, Color* dest)
	{
		
		float color_norm_r = color->r / 255.0;
		float color_norm_g = color->g / 255.0;
		float color_norm_b = color->b / 255.0;
		float color_norm_a = color->a / 255.0;

		float dest_norm_r = dest->r / 255.0;
		float dest_norm_g = dest->g / 255.0;
		float dest_norm_b = dest->b / 255.0;
		float dest_norm_a = dest->a / 255.0;
		
		float new_alpha = color_norm_a + dest_norm_a * (1 - color_norm_a);
		if (new_alpha == 0.0) return;

		unsigned char r = (color_norm_r*color_norm_a + dest_norm_r*dest_norm_a * (1 - color_norm_a)) / new_alpha * 255;
		unsigned char g = (color_norm_g*color_norm_a + dest_norm_g*dest_norm_a * (1 - color_norm_a)) / new_alpha * 255;
		unsigned char b = (color_norm_b*color_norm_a + dest_norm_b*dest_norm_a * (1 - color_norm_a)) / new_alpha * 255;

		*dest = Color(r, g, b, (int)(new_alpha*255));
	}


	void shadowdet(int rgba_width, int rgba_height, Color* rgba, int shadow_offset)
	{
		if (!shadow_offset) return;
		int forced_offset = detours_fontshadow_forced_offset.GetInt();
		shadow_offset = forced_offset? forced_offset : shadow_offset;
	
		Color* render_with_shadow = new Color[rgba_height * rgba_width];
		for (int i = 0; i < rgba_height*rgba_width; i++)
		{ // Initialize
			render_with_shadow[i] = Color();
		}
	
		for (int column_index = 0; column_index < (rgba_width); column_index++)
		{
			for (int row_index = 0; row_index < (rgba_height); row_index++)
			{
				Color* src = &rgba[((row_index * rgba_width) + column_index)];
				Color* dest = &render_with_shadow[((row_index * rgba_width) + column_index)];
	
				// Might've already put a shadow pixel here, mix instead:
				overlay_rgba(src, dest);
	
				for (int i = 1; i <= shadow_offset; i++)
				{
					if ((column_index + i) >= rgba_width || (row_index + i) >= rgba_height) break;
					Color* shadow_dest = &render_with_shadow[(((row_index+i) * rgba_width) + ((column_index+i)))];
					Color shadow_source = Color(0, 0, 0, src->a);
	
					overlay_rgba(&shadow_source, shadow_dest);
				}
			}
		}
	
		// Copy back
		for(int column_index = 0; column_index < rgba_width; column_index++ ) {
			for(int row_index = 0; row_index < rgba_height; row_index++) {
				Color* src = &render_with_shadow[((row_index * rgba_width) + column_index)];
				Color* dest = &rgba[((row_index * rgba_width) + column_index)];
	
				*dest = *src;
			}
		}
	
		delete[] render_with_shadow;
	}
}

void InitFontShadowDetour()
{
	unsigned char pattern[] = {0x85, 0xc9, 0x0f, 0x84, 0xb3, 0x00, 0x00, 0x00, 0x44, 0x8d, 0x46, 0xff, 0x89, 0xc8, 0x44, 0x39, 0xc1, 0x0f, 0x8f, 0xa4, 0x00, 0x00, 0x00};
	auto result = enginedetours::CreateInlineHook(enginedetours::Library::VGUIMATSURFACE, pattern, sizeof(pattern), (void*)&shadowdet, "FontShadow");
	if(result.has_value()) hook = std::move(result.value());
}
