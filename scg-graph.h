#pragma once
#include "scg-utility.h"
#include "scg-console.h"
#include "scg-settings.h"

namespace scg {
	
	class client_area {
	public:

		struct pixel {

			pixel(char Display, int Color = 0, bool Transparent = false) : changed(true), color_info(Color), data(Display), transparent(Transparent) {

			}

			void Display() {
				SetTextDisplay(color_info);
				putchar(data);
			}

			operator char() {
				return data;
			}

			char operator = (char data) {
				this->data = data;
				changed = true;
			}

			//int color_info = 0;
			property<int> color_info = property<int>(
				[this](int &buf) -> int {
				return buf;
			}, [this](int set, int &buf) {
				changed = true;
				buf = set;
			}
				);

			char data;
			bool transparent = true;	 // Will not override previous one if do so
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
					auto op = Other.data[other_i][other_j];
					if (!op.transparent) {
						data[i][j] = op;
					}
				}
			}
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

		array_2d<pixel> data;

	private:
		//...

	};

}