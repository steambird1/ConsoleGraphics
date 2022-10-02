#pragma once
#include "scg-utility.h"
#include "scg-graph.h"
#include "scg-settings.h"
#include "scg-console.h"
#include <map>
#include <string>
#include <thread>
#include <conio.h>
using namespace std;

// ISO C/C++ standards
#define getch _getch
#define kbhit _kbhit

/*
All in all: Handling graphics' command:

1. Draw
2. 'HasChanges = true'	(Unless you do this, the control will never be updated)

In this file should NOT contain direct "text_" color information!
*/

namespace scg {

	// These are declarations.
	class control;
	using control_symbol = string;	// Must use string to register controls -- like Windows.

	/*
	This is a pure virtual (abstract) class, which means you can't create
	object from it -- You should inherit it.
	*/
	class control {
	public:

		struct control_place {
			control *target;
			coords pos;
			control_symbol syn;

			control_place(control *Target = nullptr, control_symbol Name = "", coords ControlPos = nullptr) : target(Target), syn(Name), pos(ControlPos) {

			}

			operator control&() {
				return MyControl();
			}

			control& MyControl() {
				if (target == nullptr) throw scg_exception("Incorrect controller");
				return *target;
			}
		};

		using controls = map<control_symbol, control_place>;
		using controls_it = controls::iterator;

		virtual client_area& GetClientArea() = 0;
		virtual void ProcessKey(key_id KeyInfo) = 0;
		virtual control_symbol operator += (control_place others) {
			sub_controls[others.syn] = others;
			current_active = sub_controls.find(others.syn);
			return others.syn;
		}
		virtual bool operator -= (control_symbol others) {
			if (!sub_controls.count(others)) return false;
			// Get previous one's client area
			auto &to_delete = sub_controls[others];
			auto &td_client = to_delete.MyControl().GetClientArea();
			td_client.Fillup(spixel(' ', background_master));
			// Redraw all
			this->GetClientArea().MergeWith(td_client, to_delete.pos.x, to_delete.pos.y);
			for (auto &i : sub_controls) {
				if (i.first == others) {
					continue;
				}
				this->GetClientArea().MergeWith(i.second.MyControl().GetClientArea(), i.second.pos.x, i.second.pos.y);
			}
			// End of redraw
			to_delete.MyControl().IsDeleted = true;
			if (current_active->first == others) ActiveNext();
			sub_controls.erase(others);
			return true;
		}
		virtual void MoveControl(control_symbol ControlToMove, coords MoveTarget) {
			if (!sub_controls.count(ControlToMove)) return;
			auto &origin = sub_controls[ControlToMove];
			auto &orect = origin.MyControl().GetClientArea();
			auto &self_area = this->GetClientArea();
			for (console_pos i = origin.pos.x; i < origin.pos.x + orect.SizeH; i++) {
				for (console_pos j = origin.pos.y; j < origin.pos.y + orect.SizeW; j++) {
					self_area[i][j] = spixel(' ', my_background);
				}
			}
			origin.pos = MoveTarget;
			origin.MyControl().HasChanges = true;
			UpdateSubControls(&self_area, false, true);
		}
		virtual void ActivePrevious() {
			if (sub_controls.size() <= 0) return;	// Already no element!
			size_t yield = 0;
			do {
				if (current_active == sub_controls.begin()) current_active = sub_controls.end();
				current_active--;
				if (yield >= sub_controls.size()) return;	// All elements are not ready
				yield++;
			} while (!current_active->second.MyControl().Enabled);
		}
		virtual void ActiveNext() {
			if (sub_controls.size() <= 0) return;	// Already no element!
			size_t yield = 0;
			do {
				current_active++;
				if (yield >= sub_controls.size()) return;	// All elements are not ready
				yield++;
				if (current_active == sub_controls.end()) current_active = sub_controls.begin();
			} while (!current_active->second.MyControl().Enabled);
		}

		// Common event for all controls:
		event<event_args> LostFocus;
		event<event_args> GotFocus;
		event<event_args> PreRender;	// Call at the first render

		bool HasChanges = true;	// For initial loader
		bool Enabled = true;	// true if can be selected

		spixel my_background = background_master;
		
		property<bool> IsDeleted = property<bool>(
			[this](bool &buf) {
			return buf;
		}, [this](bool sets, bool &buf) {
			buf = sets;
			if (sets == true) {
				this->HasChanges = true;	// Remove old client area
				this->Enabled = false;
				this->GetClientArea().Fillup(spixel(' ', 0, true));	// Fill it with transparent
			}
		}
			);

	protected:

		// Unused now
		bool IProcessActive = false;

		virtual void UpdateSubControls(client_area *cl = nullptr, bool ProcessActive = false, bool ForceAll = false) {
			auto &mc_area = (cl == nullptr) ? (this->GetClientArea()) : (*cl);
			for (auto &i : sub_controls) {
				if (ProcessActive) {
					if (i.first == current_active->first) continue;	// Draw active control at last
				}
				auto &mc = i.second.MyControl();
				if (mc.HasChanges || ForceAll) {
					mc_area.MergeWith(mc.GetClientArea(), i.second.pos.x, i.second.pos.y);
					mc.HasChanges = false;
					HasChanges = true;
				}
				// Debug

				// End
			}
			if (ProcessActive || ForceAll) {
				auto &i = *current_active;
				auto &mc = i.second.MyControl();
				if (mc.HasChanges) {
					mc_area.MergeWith(mc.GetClientArea(), i.second.pos.x, i.second.pos.y);
					mc.HasChanges = false;
					HasChanges = true;
				}
			}
		}

		controls sub_controls;
		controls_it current_active;
		//client_area mc_area;
	};

	// For easier access
	using control_set = control::control_place;

	class window : public control {
	public:
		window(console_size Height, console_size Width) : mc_area(client_area(Height, Width)) {
			my_background = background_window;
			mc_area.Fillup(spixel(' ', background_window));
			// For title bar
			// Default LostFocus Drawer
			PreRender += [this](event_args e) {
				for (console_size i = 0; i < mc_area.SizeW; i++) mc_area[0][i].color_info = inactive_window;
				HasChanges = true;
			};
			LostFocus += [this](event_args e) {
				// Set window title bar to inactive state
				for (console_size i = 0; i < mc_area.SizeW; i++) mc_area[0][i].color_info = inactive_window;
				HasChanges = true;
			};
			// Default GotFocus Drawer
			GotFocus += [this](event_args e) {
				// Set window title bar to active state
				for (console_size i = 0; i < mc_area.SizeW; i++) mc_area[0][i].color_info = active_window;
				HasChanges = true;
			};
		}
		virtual client_area& GetClientArea() {
			// Should be like this: Update all sons
			// Will become foreground if the order of dictionary of the name is bigger (like: 'Z' > 'A').
			// (For window: Has its borders (1 line))
			UpdateSubControls(&mc_area);
			return mc_area;
		}
		virtual void ProcessKey(key_id KeyInfo) {
			// For active control ...
			switch (KeyInfo) {
			case go_prev_control:
				ActivePrevious();
				break;
			case go_next_control:
				ActiveNext();
				break;
			default:
				if (current_active != sub_controls.end()) {
					current_active->second.MyControl().ProcessKey(KeyInfo);
				}
				break;
			}
		}

		// Get or set the title of the whole window
		property<string> Title = property<string>([this](string &tmp) -> string {
			return tmp;
		}, [this](string sets, string &tmp) {
			if (mc_area.SizeW - 1 <= sets.length()) return;
			for (size_t i = 0; i < sets.length(); i++) mc_area[0][i] = sets[i];
			tmp = sets;
			HasChanges = true;
			// Draw title bar
		});
		
	private:
		client_area mc_area;
	};

	// Insert more controls ....

	// Label control (show some text).
	class label : public control {
	public:
		label(string Text, console_size Height, console_size Width) : mc_area(client_area(Height, Width)) {
			//this->Enabled = false;	// Can't be selected!
			mc_area.Fillup(spixel(' ', text_label));
			this->Height = Height;
			this->Width = Width;
			//this->Style = text_label;
			this->Text = Text;
			this->PreRender += [this](event_args e) {
				this->Enabled = false;
			};
		}

		virtual client_area& GetClientArea() {
			// Do nothing for child
			return mc_area;
		}
		virtual void ProcessKey(key_id KeyInfo) {
			// Do nothing for keys
		}

		virtual control_symbol operator += (control_place others) {
			throw scg_exception("Cannot add sub control for a label");
		}

		virtual bool operator -= (control_symbol others) {
			throw scg_exception("Cannot remove sub control from a label");
		}

		virtual void ActiveNext() {
			throw scg_exception("Cannot active sub control of a label");
		}

		virtual void ActivePrevious() {
			throw scg_exception("Cannot active sub control of a label");
		}

		property<console_size> Height = property<console_size>([this](console_size &buf) -> console_size {
			return buf;
		}, [this](console_size sets, console_size &buf) {
			buf = sets;
			RedrawText();
		}
			);

		property<console_size> Width = property<console_size>([this](console_size &buf) -> console_size {
			return buf;
		}, [this](console_size sets, console_size &buf) {
			buf = sets;
			RedrawText();
		}
		);

		property<string> Text = property<string>([this](string &buf) -> string {
			return buf;
		}, [this](string sets, string &buf) {
			buf = sets;
			RedrawText();
		});

		property<pixel_color> Style = property<pixel_color>([this](pixel_color &buf) -> pixel_color {
			return buf;
		}, [this](pixel_color sets, pixel_color &buf) {
			buf = sets;
			console_size MyHeight = Height, MyWidth = Width;
			for (console_pos i = 0; i < MyHeight; i++) {
				for (console_pos j = 0; j < MyWidth; j++) {
					mc_area[i][j].color_info = sets;
				}
			}
			HasChanges = true;
		});


	protected:
		
		void RedrawText() {
			string TextData = this->Text;
			console_size MyHeight = Height, MyWidth = Width;
			console_pos CurrentX = 0, CurrentY = 0;	// X: linear pos, Y: wide pos.
			for (auto &i : TextData) {
				if (i == '\n') {
					CurrentX++;
					continue;
				}
				if (i == '\t') {
					CurrentY += tab_size;
					continue;
				}
				if (CurrentY == MyWidth) {
					CurrentX++;
					CurrentY = 0;
				}
				if (CurrentX >= MyHeight) {
					break;	// Break the rest
				}
				mc_area[CurrentX][CurrentY] = i;
				CurrentY++;
			}
			HasChanges = true;
		}

		client_area mc_area;
	};

	// Button control
	class button : public control {
	public:

		button(string Text, console_size Height, console_size Width) : mc_area(client_area(Height, Width)) {
			//this->Enabled = false;	// Can't be selected!
			mc_area.Fillup(spixel(' ', text_label));
			this->Height = Height;
			this->Width = Width;
			//this->Style = text_label;
			this->Text = Text;
			PreRender += [this](event_args e) {
				for (console_size i = 0; i < mc_area.SizeW; i++) mc_area[0][i].color_info = inactive_window;
				HasChanges = true;
			};
			LostFocus += [this](event_args e) {
				// Set window title bar to inactive state
				for (console_size i = 0; i < mc_area.SizeW; i++) mc_area[0][i].color_info = inactive_window;
				HasChanges = true;
			};
			// Default GotFocus Drawer
			GotFocus += [this](event_args e) {
				// Set window title bar to active state
				for (console_size i = 0; i < mc_area.SizeW; i++) mc_area[0][i].color_info = active_window;
				HasChanges = true;
			};
		}

		virtual client_area& GetClientArea() {
			// Do nothing for child
			return mc_area;
		}

		virtual void ProcessKey(key_id KeyInfo) {
			if (KeyInfo == active_button) {
				OnClick.RunEvent(event_args());
			}
		}
	
		virtual control_symbol operator += (control_place others) {
			throw scg_exception("Cannot add sub control for a button");
		}

		virtual bool operator -= (control_symbol others) {
			throw scg_exception("Cannot remove sub control from a button");
		}

		virtual void ActiveNext() {
			throw scg_exception("Cannot active sub control of a button");
		}

		virtual void ActivePrevious() {
			throw scg_exception("Cannot active sub control of a button");
		}

		property<console_size> Height = property<console_size>([this](console_size &buf) -> console_size {
			return buf;
		}, [this](console_size sets, console_size &buf) {
			buf = sets;
			RedrawText();
		}
		);

		property<console_size> Width = property<console_size>([this](console_size &buf) -> console_size {
			return buf;
		}, [this](console_size sets, console_size &buf) {
			buf = sets;
			RedrawText();
		}
		);

		property<string> Text = property<string>([this](string &buf) -> string {
			return buf;
		}, [this](string sets, string &buf) {
			buf = sets;
			RedrawText();
		});

		property<pixel_color> Style = property<pixel_color>([this](pixel_color &buf) -> pixel_color {
			return buf;
		}, [this](pixel_color sets, pixel_color &buf) {
			buf = sets;
			console_size MyHeight = Height, MyWidth = Width;
			for (console_pos i = 0; i < MyHeight; i++) {
				for (console_pos j = 0; j < MyWidth; j++) {
					mc_area[i][j].color_info = sets;
				}
			}
			HasChanges = true;
		});

		event<event_args> OnClick;

	protected:

		void RedrawText() {
			string TextData = this->Text;
			console_size MyHeight = Height, MyWidth = Width;
			console_pos CurrentX = 0, CurrentY = 0;	// X: linear pos, Y: wide pos.
			for (auto &i : TextData) {
				if (i == '\n') {
					CurrentX++;
					continue;
				}
				if (i == '\t') {
					CurrentY += tab_size;
					continue;
				}
				if (CurrentY == MyWidth) {
					CurrentX++;
					CurrentY = 0;
				}
				if (CurrentX >= MyHeight) {
					break;	// Break the rest
				}
				mc_area[CurrentX][CurrentY] = i;
				CurrentY++;
			}
			HasChanges = true;
		}

		client_area mc_area;

	};

	/*
	This class is about master application. It should have 1 only.
	*/
	class master : public control {
	public:
		master() : mc_area(client_area(height, width)) {
			this->IProcessActive = true;
		}
		virtual client_area& GetClientArea() {
			UpdateSubControls(&mc_area, true);
			return mc_area;
		}
		virtual void ProcessKey(key_id KeyInfo) {
			// Jump to another window if it is, or send it down to active one
			if (current_active != sub_controls.end()) {
				if (KeyInfo == switch_windows) {
					current_active->second.MyControl().LostFocus.RunEvent(event_args());
					ActiveNext();
					current_active->second.MyControl().GotFocus.RunEvent(event_args());
				}
				else {
					current_active->second.MyControl().ProcessKey(KeyInfo);
				}
			}
			// After processing key, draw (Only for master!)
			GetClientArea().Draw();
		}
		// Call this after all of work
		void MainLoop() {
			ResetConsole();
			SetEscapeOutput();
			SetCursorDisplay(display_show, display_disable);
			for (auto &i : sub_controls) {
				i.second.MyControl().PreRender.RunEvent(event_args());
			}
			current_active->second.MyControl().GotFocus.RunEvent(event_args());
			GetClientArea().Draw();	// Initial drawer
			while (true) {
				ProcessKey(getch());
				//this_thread::yield();
			}
		}
	private:
		// Buffered
		client_area mc_area;
	};

};