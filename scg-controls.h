#pragma once
#include "scg-utility.h"
#include "scg-graph.h"
#include "scg-settings.h"
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
			if (current_active->first == others) ActiveNext();
			sub_controls.erase(others);
			return true;
		}
		virtual void ActiveNext() {
			if (sub_controls.size() <= 0) return;	// Already no element!
			do {
				current_active++;
				if (current_active == sub_controls.end()) current_active = sub_controls.begin();
			} while (!current_active->second.MyControl().Enabled);
		}

		// Common event for all controls:
		event<event_args> LostFocus;
		event<event_args> GotFocus;
		event<event_args> PreRender;	// Call at the first render

		bool HasChanges = true;	// For initial loader
		bool Enabled = true;	// true if can be selected

	protected:
		controls sub_controls;
		controls_it current_active;
	};

	// For easier access
	using control_set = control::control_place;

	class window : public control {
	public:
		window(console_size Height, console_size Width) : mc_area(client_area(Height, Width)) {
			mc_area.Fillup(client_area::pixel(' ', pixel_colors::Generate(text_white + text_background)));
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
			for (auto &i : sub_controls) {
				auto &cont = i.second.MyControl();
				if (cont.HasChanges) {
					mc_area.MergeWith(cont.GetClientArea(), i.second.pos.x, i.second.pos.y);
					cont.HasChanges = false;
					HasChanges = true;
				}
			}
			return mc_area;
		}
		virtual void ProcessKey(key_id KeyInfo) {
			// For active control ...
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

	/*
	This class is about master application. It should have 1 only.
	*/
	class master : public control {
	public:
		master() : mc_area(client_area(height, width)) {
			
		}
		virtual client_area& GetClientArea() {
			for (auto &i : sub_controls) {
				if (i.first == current_active->first) continue;	// Draw active control at last
				auto &mc = i.second.MyControl();
				if (mc.HasChanges) {
					mc_area.MergeWith(mc.GetClientArea(), i.second.pos.x, i.second.pos.y);
					mc.HasChanges = false;
					HasChanges = true;
				}
			}
			auto &i = *current_active;
			auto &mc = i.second.MyControl();
			if (mc.HasChanges) {
				mc_area.MergeWith(mc.GetClientArea(), i.second.pos.x, i.second.pos.y);
				mc.HasChanges = false;
				HasChanges = true;
			}
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
			SetEscapeOutput();
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