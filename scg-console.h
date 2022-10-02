#pragma once
#include "scg-utility.h"
#include <cstdio>
#include <set>
#include <conio.h>
using namespace std;

#define getch _getch
#define kbhit _kbhit

namespace scg {

	// Refer to: https://learn.microsoft.com/zh-cn/windows/console/console-virtual-terminal-sequences

	constexpr char escape = 0x1B;

	// Use these 2 things carefully!
	void SaveCursorPos() {
		printf("%c[7", escape);
	}

	void RestoreCursorPos() {
		printf("%c[8", escape);
	}

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

	class pixel_colors {
	public:
		static pixel_color Generate(int FirstColor = 0, int SecondColor = 0, int ThirdColor = 0) {
			return FirstColor | (SecondColor << 7) | (ThirdColor << 14);
		}
		static void Unpack(pixel_color ColorData) {
			// Must execute First Color.
			int ThirdColor = ColorData >> 14;
			int SecondColor = (ColorData & 0x3fff) >> 7;
			int FirstColor = ColorData & 0x7f;
			if (ThirdColor != 0) SetTextDisplay(ThirdColor);
			if (SecondColor != 0) SetTextDisplay(SecondColor);
			SetTextDisplay(FirstColor);
		}
	};

	// Please call this function before use anything!
	void SetEscapeOutput() {
		win_handle hout = GetStdHandle(STD_OUTPUT_HANDLE);
		if (hout == INVALID_HANDLE_VALUE) throw scg_exception("Invalid output handle");
		win_uint res;
		if (!GetConsoleMode(hout, &res)) throw scg_exception("Cannot get console information");
		res |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		if (!SetConsoleMode(hout, res)) throw scg_exception("Cannot set console mode");

		// Make the whole screen clean by:
		system("cls");
		system("color");
	}

	void ResetConsole() {
		system("cls & color 07");
	}

	const set<key_id> ExtendedKeys = { 224 };

	struct keyboard_state {
		// = get a keyboard state
		key_id PrimaryKey, ExtendedKey = 0;
		// If first one is Extended Key: Primary Key = Next Keycode, Extended Key = Extended (First) one

		keyboard_state(key_id PrimaryKey = 0, key_id ExtendedKey = 0) : PrimaryKey(PrimaryKey), ExtendedKey(ExtendedKey) {
			
		}

		// Get comparable key: Put Extended key into higher place
		operator key_id() {
			return (ExtendedKey << 8) | PrimaryKey;
		}

#define MakeKeyID(PrimaryKey,ExtendedKey) ((ExtendedKey << 8) | PrimaryKey)
	};

	keyboard_state GetKeyboard() {
		keyboard_state k;
		key_id g = getch();
		if (ExtendedKeys.count(g)) {
			k.ExtendedKey = g;
			k.PrimaryKey = getch();
		}
		else {
			k.PrimaryKey = g;
		}
		return k;
	}

};