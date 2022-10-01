#pragma once
#include "scg-utility.h"
#include "scg-graph.h"

namespace scg {

	constexpr console_pos width = 80;
	constexpr console_pos height = 25;

	// Now sets keys for operations.
	// For keycode: https://www.cnblogs.com/lxwphp/p/9548823.html
	constexpr key_id switch_windows = 9;				// tab

	// Now sets colors.
	// For colors: scg-console.h
	const pixel_color inactive_window = pixel_colors::Generate(text_blue + text_background, text_white + text_foreground);
	const pixel_color active_window = pixel_colors::Generate(text_blue + text_intensity + text_background, text_black + text_foreground);
}