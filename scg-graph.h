#pragma once
#include "scg-utility.h"
#include "scg-console.h"
#include "scg-settings.h"
#include <mutex>
using namespace std;

namespace scg {

	// Note: These functions generates color information for client_area::pixel.
	// This function put 3 color information into 1 integer.
	
	class client_area {
	public:

		// 'change' must be set after output!
		struct pixel {

			// 'Color' must from pixel_colors::Generate()
			pixel(char Display = '\0', pixel_color Color = 0, bool Transparent = false) : changed(true), data(Display) {
				color_info = Color;
				transparent = Transparent;
			}

			void Display() {
				// Before print all pixels
				SetTextDisplay();
				pixel_colors::Unpack(color_info);
				putchar(data);
			}

			operator char() {
				return data;
			}

			char operator = (char data) {
				this->data = data;
				changed = true;
				return data;
			}

			//int color_info = 0;
			property<pixel_color> color_info = property<pixel_color>(
				[this](int &buf) -> int {
				return buf;
			}, [this](int set, int &buf) {
				if (buf != set) {
					changed = true;
					buf = set;
				}
			}
				);

			property<bool> transparent = property<bool>(
				[this](bool &buf) -> bool {
				return buf;
			}, [this](bool set, bool &buf) {
				if (buf != set) {
					changed = true;
					buf = set;
				}
			}
			);

			char data;
			//bool transparent = true;	 // Will not override previous one if do so
			bool changed = false;
		};

		client_area(console_size SizeH, console_size SizeW) : data(array_2d<pixel>(SizeH, SizeW)) {
			
		}

		// Higher priority must merge later
		void MergeWith(client_area Other, console_pos StartX = 0, console_pos StartY = 0) {
			console_pos EndX = StartX + Other.SizeH;
			console_pos EndY = StartY + Other.SizeW;
			if (EndX >= SizeH) return;
			if (EndY >= SizeW) return;
			for (console_pos i = StartX; i < EndX; i++) {
				for (console_pos j = StartY; j < EndY; j++) {
					console_pos other_i = i - StartX, other_j = j - StartY;
					auto &op = Other.data[other_i][other_j];
					if (!op.transparent) {
						data[i][j] = op;
					}
				}
			}
		}

		// Must move to specified place first.
		void Draw(bool InvaildateAll = false, bool NoChanging = false, bool IgnoreLock = false) {
			if (!IgnoreLock) client_area::output_lock.lock();
			SaveCursorPos();
			for (console_pos i = 0; i < SizeH; i++) {
				for (console_pos j = 0; j < SizeW; j++) {
					auto &op = data[i][j];
					if (((!op.transparent) && op.changed) || InvaildateAll) {
						MoveAbsoluteCursor(coords(i, j));
						op.Display();
						if (!NoChanging) op.changed = false;
					}
				}
			}
			RestoreCursorPos();
			if (!IgnoreLock) client_area::output_lock.unlock();
		}

		void Fillup(pixel PixelData) {
			data.FillWith(PixelData);
		}

		property<console_pos> SizeH = property<console_pos>(
			[this](console_pos &reserved) -> console_pos {
			return data.Size1D;
		}, [](console_pos set, console_pos &reserved) {
			throw scg_exception("Cannot write this property!");
		}
		);

		property<console_pos> SizeW = property<console_pos>(
			[this](console_pos &reserved) -> console_pos {
			return data.Size2D;
		}, [](console_pos set, console_pos &reserved) {
			throw scg_exception("Cannot write this property!");
		}
			);

		operator array_2d<pixel>&() {
			return data;
		}

		array_2d<pixel>::__array_helper operator [] (console_pos Line) {
			return data[Line];
		}

		array_2d<pixel> data;

	private:
		//...
		static mutex output_lock;

	};
	mutex client_area::output_lock;

	using spixel = client_area::pixel;

}