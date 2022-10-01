#pragma once
#include "scg-utility.h"
#include "scg-graph.h"
#include "scg-settings.h"
#include <map>
#include <string>
#include <thread>
using namespace std;

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

			control_place(control *Target, control_symbol Name, coords ControlPos) : target(Target), syn(Name), pos(ControlPos) {

			}

			operator control&() {
				return *target;
			}

			control& MyControl() {
				return *target;
			}
		};

		using controls = map<control_symbol, control_place>;
		using controls_it = controls::iterator;

		virtual client_area& GetClientArea() = 0;
		virtual void ProcessKey(key_id KeyInfo) = 0;
		virtual control_symbol operator += (control_place others) {
			sub_controls[others.syn] = others;
			return others.syn;
		}
		virtual bool operator -= (control_symbol others) {
			if (!sub_controls.count(others)) return false;
			sub_controls.erase(others);
			return true;
		}
		virtual void ActiveNext() {
			current_active++;
			if (current_active == sub_controls.end()) current_active = sub_controls.begin();
		}

		// Common event for all controls:
		event<event_args> LostFocus;
		event<event_args> GotFocus;

		bool HasChanges = false;

	protected:
		controls sub_controls;
		controls_it current_active;
	};

	class window : public control {
	public:
		window(console_size Height, console_size Width) : mc_area(client_area(Height, Width)) {
			// Default LostFocus Drawer
			LostFocus += [this](event_args e) {
				// Set window title bar to inactive state ...

				HasChanges = true;
			};
			// Default GotFocus Drawer
			GotFocus += [this](event_args e) {
				// Set window title bar to active state ...

				HasChanges = true;
			};
		}
		virtual client_area& GetClientArea() {
			return mc_area;
		}
		virtual void ProcessKey(key_id KeyInfo) {
			
		}
		
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
			return mc_area;
		}
		// Will become foreground if the order of dictionary of the name is bigger (like: 'Z' > 'A').
		void MergeClientArea() {
			for (auto &i : sub_controls) {
				if (i.second.MyControl().HasChanges) {
					mc_area.MergeWith(i.second.MyControl().GetClientArea());
				}
			}
		}
		virtual void ProcessKey(key_id KeyInfo) {
			// Jump to another window if it is, or send it down to active one
			if (KeyInfo == switch_windows) {
				current_active->second.MyControl().LostFocus.RunEvent(event_args());
				ActiveNext();
				current_active->second.MyControl().GotFocus.RunEvent(event_args());
			}
			else {
				current_active->second.MyControl().ProcessKey(KeyInfo);
			}
			// After processing key, draw (Only for master!)
			
		}
		// Call this after all of work
		void MainLoop() {
			while (true) {
				
				this_thread::yield();
			}
		}
	private:
		// Buffered
		client_area mc_area;
	};

};