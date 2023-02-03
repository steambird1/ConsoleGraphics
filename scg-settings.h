#pragma once
#include "scg-utility.h"
#include "scg-graph.h"
#include "scg-console.h"
#include <string>
using namespace std;

namespace scg {

	constexpr console_pos width = 80;
	constexpr console_pos height = 25;

	constexpr console_size tab_size = 4;

	// For liunx:
	const int escape_key = 27;
#if defined(_WIN32)
	const int delete_key = 8;
#else
	const int delete_key = 127;
#endif
	const int enter_key = 10;

	// Now sets keys for operations.
	// For keycode: https://www.cnblogs.com/lxwphp/p/9548823.html
	const key_id switch_windows = 9;					// tab
#if defined(_WIN32)
	const key_id go_prev_control = MakeKeyID(75, 224);	// left key
	const key_id go_next_control = MakeKeyID(77, 224);	// right key
	const key_id active_button = 13;					// enter
#else
	const key_id go_prev_control = MakeKeyID(68, escape_key);	// left key
	const key_id go_next_control = MakeKeyID(67, escape_key);	// right key
	const key_id active_button = enter_key;				// enter
#endif
	const key_id active_checkbox = active_button;

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
	const pixel_color background_input = pixel_colors::Generate(text_white + text_intensity + text_background, text_black + text_foreground);
	const pixel_color actived_checkbox = actived_button;
	const pixel_color deactived_checkbox = deactived_button;

	// Checkbox settings
	const console_size rsize_checkbox = 3;
	const string unselected_checkbox = "[ ]";
	const string multiple_checkbox = "[x]";
	const string radio_checkbox = "[.]";
}