#pragma once
#include "scg-utility.h"
#include <cstdio>
using namespace std;

namespace scg {

	// Refer to: https://learn.microsoft.com/zh-cn/windows/console/console-virtual-terminal-sequences

	constexpr char escape = 0x1B;

	constexpr char move_up = 'A';
	constexpr char move_down = 'B';
	constexpr char move_right = 'C';
	constexpr char move_forward = move_right;
	constexpr char move_left = 'D';
	constexpr char move_backward = move_left;

	void MoveRelativeCursor(console_pos LineSize, char Method) {
		printf("%c[%d%c", escape, int(LineSize), Method);
	}

	// Specified (x,y) will be the start point
	// x,y in [0, size)
	void MoveAbsoluteCursor(coords pos) {
		printf("%c[%d;%dH", escape, int(pos.x+1), int(pos.y+1));
	}

	constexpr char display_enable = 'h';
	constexpr char display_disable = 'l';
	constexpr int display_flash = 12;
	constexpr int display_show = 25;
	constexpr int display_backup = 1049;	// Use another buffer?

	void SetCursorDisplay(int DisplayMode, char DisplayOption) {
		printf("%c[?%d%c", escape, DisplayMode, DisplayOption);
	}

	// Use '+' to connect following information:
	// For example, set green foreground color: SetTextDisplay(text_green + text_intensity + text_foreground); or SetTextDisplay(92);

	constexpr int text_default = 0;
	constexpr int text_bold = 1;
	constexpr int text_nobold = 22;
	constexpr int text_underline = 4;
	constexpr int text_nounderline = 24;
	constexpr int text_reverse = 7;
	constexpr int text_noreverse = 27;
	
	constexpr int text_black = 0;
	constexpr int text_red = 1;
	constexpr int text_green = 2;
	constexpr int text_yellow = 3;
	constexpr int text_blue = 4;
	constexpr int text_mag = 5;
	constexpr int text_cyan = 6;
	constexpr int text_white = 7;
	
	constexpr int text_intensity = 60;
	constexpr int text_foreground = 30;
	constexpr int text_background = 40;

	void SetTextDisplay(int DisplayMode = text_default) {
		printf("%c[%dm", escape, DisplayMode);
	}

	// Please call this function before use anything!
	void SetEscapeOutput() {
		win_handle hout = GetStdHandle(STD_OUTPUT_HANDLE);
		if (hout == INVALID_HANDLE_VALUE) throw scg_exception("Invalid output handle");
		win_uint res;
		if (!GetConsoleMode(hout, &res)) throw scg_exception("Cannot get console information");
		res |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		if (!SetConsoleMode(hout, res)) throw scg_exception("Cannot set console mode");
	}

};