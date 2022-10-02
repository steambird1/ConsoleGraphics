#pragma once
#include "scg-utility.h"
#include "scg-graph.h"

namespace scg {

	constexpr console_pos width = 80;
	constexpr console_pos height = 25;

	constexpr console_size tab_size = 4;

	// Now sets keys for operations.
	// For keycode: https://www.cnblogs.com/lxwphp/p/9548823.html
	constexpr key_id switch_windows = 9;				// tab
	constexpr key_id go_prev_control = 37;				// left arrow
	constexpr key_id go_next_control = 39;				// right arrow
	constexpr key_id active_button = 13;				// enter

	// Now sets colors.
	// For colors: scg-console.h
	// Should be like: [mode]_[control type]
	const pixel_color inactive_window = pixel_colors::Generate(text_blue + text_background, text_white + text_foreground);
	const pixel_color active_window = pixel_colors::Generate(text_blue + text_intensity + text_background, text_black + text_foreground);
	const pixel_color background_window = pixel_colors::Generate(text_white + text_intensity + text_background);
	const pixel_color text_label = pixel_colors::Generate(text_black + text_foreground, background_window);
	const pixel_color background_master = pixel_colors::Generate(text_black + text_background, text_white + text_intensity + text_foreground);
	const pixel_color inactived_button = pixel_colors::Generate(text_white + text_intensity + text_background, text_black + text_foreground);
	const pixel_color actived_button = pixel_colors::Generate(text_red + text_background, text_white + text_intensity + text_background);
	const pixel_color deactived_button = pixel_colors::Generate(text_black + text_intensity + text_background, text_white + text_foreground);
}