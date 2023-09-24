#pragma once
#include "scg-utility.h"
#include "scg-graph.h"
#include "scg-settings.h"
#include "scg-console.h"
#include <map>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
using namespace std;

/*
All in all: Handling graphics' command:

1. Draw
2. 'HasChanges = true'	(Unless you do this, the control will never be updated)

In this file should NOT contain direct "text_" color information!
*/

namespace scg {

	// These are declarations.
	class control;
	class master;
	using control_symbol = string;	// Must use string to register controls -- like Windows.

	class pre_render_event_args : public event_args {
	public:

		pre_render_event_args(master *MasterPointer = nullptr) : MasterPointer(MasterPointer) {

		}

		master *MasterPointer;
	};

	class resize_event_args : public event_args {
	public:

		resize_event_args(console_size NewHeight = 0, console_size NewWidth = 0) : NewHeight(NewHeight), NewWidth(NewWidth) {

		}

		bool Valid() const {
			return NewHeight != 0 && NewWidth != 0;
		}

		console_size NewHeight, NewWidth;
	};

	/*
	This is a pure virtual (abstract) class, which means you can't create
	object from it -- You should inherit it.

	Later: Put layout system here (by using something like +=)
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
		virtual void ProcessKey(keyboard_state KeyInfo) = 0;
		virtual control_symbol operator += (control_place others) {
			sub_controls[others.syn] = others;
			auto &omc = others.MyControl();
			omc.my_coords = others.pos;
			omc.father = this;
			omc.my_symbol = others.syn;
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
			ClearControlArea(ControlToMove);
			origin.pos = MoveTarget;
			origin.MyControl().my_coords = MoveTarget;
			origin.MyControl().HasChanges = true;
			origin.MyControl().OnMove.RunEvent(event_args());
			UpdateSubControls(&self_area, false, true);
		}
		virtual void ResizeControl(control_symbol ControlToMove, console_size NewHeight, console_size NewWidth) {
			if (!sub_controls.count(ControlToMove)) return;
			auto &origin = sub_controls[ControlToMove];
			auto &orect = origin.MyControl().GetClientArea();
			auto &self_area = this->GetClientArea();
			ClearControlArea(ControlToMove);
			origin.MyControl().OnResize.RunEvent(resize_event_args(NewHeight, NewWidth));
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
			} while ((!current_active->second.MyControl().Enabled) || (!current_active->second.MyControl().Visible));
		}
		virtual void ActiveNext() {
			if (sub_controls.size() <= 0) return;	// Already no element!
			bool have_enable = false;
			for (auto &i : sub_controls) {
				if (i.second.MyControl().Enabled) {
					have_enable = true;
					break;
				}
			}
			if (!have_enable) {
				current_active = sub_controls.begin();
				return;	// Can't set!
			}
			size_t yield = 0;
			do {
				current_active++;
				if (yield >= sub_controls.size()) return;	// All elements are not ready
				yield++;
				if (current_active == sub_controls.end()) current_active = sub_controls.begin();
			} while ((!current_active->second.MyControl().Enabled) || (!current_active->second.MyControl().Visible));
		}

		// Common event for all controls:
		event<event_args> LostFocus;
		event<event_args> GotFocus;
		event<pre_render_event_args> PreRender;	// Call at the first render
		event<event_args> OnMove;
		event<resize_event_args> OnResize;
		// Warning: in AfterDraw the display will be not updated!
		event<event_args> AfterDraw;	// After drawing (mostly for input box)

		//bool HasChanges = true;	// For initial loader
		property<bool> HasChanges = property<bool>(
			[this](bool &tmp) -> bool {
			return IHasChanges;
		},
			[this](bool sets, bool &tmp) {
			IHasChanges = sets;
		}
			);
		bool Enabled = true;	// true if can be selected
		// true if the control can be shown
		property<bool> Visible = property<bool>(
			[this](bool &tmp) -> bool {
			return IVisible;
		},
			[this](bool sets, bool &tmp) {
			if (IVisible != sets) {
				IVisible = sets;
				this->HasChanges = true;
			}
		}

			);

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

		// Will not validate
		virtual void ClearControlArea(control_symbol ControlToMove) {
			auto &origin = sub_controls[ControlToMove];
			auto &orect = origin.MyControl().GetClientArea();
			auto &self_area = this->GetClientArea();
			for (console_pos i = origin.pos.x; i < origin.pos.x + orect.SizeH; i++) {
				for (console_pos j = origin.pos.y; j < origin.pos.y + orect.SizeW; j++) {
					self_area[i][j] = spixel(' ', my_background.color_info);
				}
			}
		}

		coords GetBaseCoords() {
			if (father == nullptr) return my_coords;
			return my_coords + father->GetBaseCoords();
		}

		coords my_coords;

		// Unused now:
		bool IProcessActive = false;

		bool IVisible = true;
		bool IHasChanges = true;

		virtual bool CurrentActivationAvailable() {
			return sub_controls.size() && (current_active != sub_controls.end());
		}

		virtual void UpdateSubControls(client_area *cl = nullptr, bool ProcessActive = false, bool ForceAll = false) {
			auto &mc_area = (cl == nullptr) ? (this->GetClientArea()) : (*cl);
			for (auto &i : sub_controls) {
				if (ProcessActive) {
					if (i.first == current_active->first) continue;	// Draw active control at last
				}
				auto &mc = i.second.MyControl();
				if (!mc.Visible) {
					auto &origin = i.second;
					auto &orect = mc.GetClientArea();
					auto &self_area = mc_area;
					for (console_pos i = origin.pos.x; i < origin.pos.x + orect.SizeH; i++) {
						for (console_pos j = origin.pos.y; j < origin.pos.y + orect.SizeW; j++) {
							self_area[i][j] = spixel(' ', my_background.color_info);
						}
					}
					mc.HasChanges = false;
					HasChanges = true;
				}
			}
			for (auto &i : sub_controls) {
				if (ProcessActive) {
					if (i.first == current_active->first) continue;	// Draw active control at last
				}
				auto &mc = i.second.MyControl();
				if (!mc.Visible) continue;
				mc.UpdateSubControls();
				if (mc.HasChanges || ForceAll) {
					mc_area.MergeWith(mc.GetClientArea(), i.second.pos.x, i.second.pos.y);
					mc.HasChanges = false;
					HasChanges = true;
				}
			}
			if (ProcessActive || ForceAll) {
				auto &i = *current_active;
				auto &mc = i.second.MyControl();
				mc.UpdateSubControls();
				if (mc.HasChanges) {
					if (mc.Visible) {
						mc_area.MergeWith(mc.GetClientArea(), i.second.pos.x, i.second.pos.y);
					}
					else {
						// Actually active control should not be hidden ... It's undefined behavior.
						auto &origin = i.second;
						auto &orect = mc.GetClientArea();
						auto &self_area = mc_area;
						for (console_pos i = origin.pos.x; i < origin.pos.x + orect.SizeH; i++) {
							for (console_pos j = origin.pos.y; j < origin.pos.y + orect.SizeW; j++) {
								self_area[i][j] = spixel(' ', my_background.color_info);
							}
						}
					}
					mc.HasChanges = false;
					HasChanges = true;
				}
			}
		}

		controls sub_controls;
		controls_it current_active;
		control *father = nullptr;
		control_symbol my_symbol;
		//client_area mc_area;
	};

	class invisible_control : public control {
	public:

		invisible_control() {
			this->IVisible = false;
			this->IProcessActive = false;
			this->Enabled = property<bool>([](bool &in) -> bool {return false; }, [](bool set, bool &in) {});
			this->Visible = property<bool>([](bool &in) -> bool {return false; }, [](bool set, bool &in) {});
		}

		virtual client_area& GetClientArea() {
			return empty_area;
		}
		virtual void ProcessKey(keyboard_state KeyInfo) {
			// Do nothing
		}

	private:
		client_area empty_area = client_area(1, 1);	// 0,0 seem to be fail
	};

	// For easier access
	using control_set = control::control_place;


	class Protection {
	public:
		static void RaiseError(string Description) {
			ErrorRaised = true;
			ResetConsole();
			SetTextDisplay();
			MoveAbsoluteCursor(coords(0, 0));
			system("color 1f");
			printf("An critical error has occured and Console Graphics has been \n");
			printf("shut down to protect your application.                      \n");
			printf("                                                            \n");
			printf("Technial information: Error:                                \n%s", Description.c_str());
			while (1) {
				this_thread::yield();
			}
		}
	private:
		static bool ErrorRaised;
	};
	bool Protection::ErrorRaised = false;


	/*
	This class is about master application. It should have 1 only.
	*/
	class master : public control {
	public:
		master() : mc_area(client_area(height, width)) {
			this->IProcessActive = true;
		}
		virtual client_area& GetClientArea() {
			// Must be locked
			render_lock.lock();
			UpdateSubControls(&mc_area, true);
			render_lock.unlock();
			return mc_area;
		}
		void MasterRender() {
			GetClientArea().Draw();
			for (auto &i : sub_controls) {
				i.second.MyControl().AfterDraw.RunEvent(event_args());
			}
		}
		virtual void ProcessKey(keyboard_state KeyInfo) {
			// Jump to another window if it is, or send it down to active one
			if (CurrentActivationAvailable()) {
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
			MasterRender();
		}
		// Call this for prompt bar
		void BarPrompt(string PromptText, pixel_color Style) {
			ClearPrompt(Style);
			console_pos ptr = 0;
			for (auto &i : PromptText) {
				mc_area[height - 1][ptr++] = i;
			}
			GetClientArea().Draw();
		}
		void BarClean() {
			for (console_pos i = 0; i < width; i++) mc_area[height - 1][i] = spixel(' ', 0);
		}
#if defined(_WIN32)
		string BarInput(string PromptText, pixel_color Style, int BufferSize = 2048) {
			ClearPrompt(Style);
			console_pos ptr = 0;
			for (auto &i : PromptText) {
				mc_area[height - 1][ptr++] = i;
			}
			GetClientArea().Draw();
			// After this: move cursor and listen!
			MoveAbsoluteCursor(coords(height - 1, ptr));
			char *s = new char[BufferSize];
			SetCursorDisplay(display_show, display_enable);
			fgets(s, BufferSize - 1, stdin);
			SetCursorDisplay(display_show, display_disable);
			BarClean();
			GetClientArea().Draw();
			return s;
		}
#else
		// WARNING: DEPRECATED UNDER LINUX
		string BarInput(string PromptText, pixel_color Style, int BufferSize = 2048, bool UseNormalMode = true) {
			ClearPrompt(Style);
			console_pos ptr = 0;
			for (auto &i : PromptText) {
				mc_area[height - 1][ptr++] = i;
			}
			GetClientArea().Draw();
			// After this: move cursor and listen!
			MoveAbsoluteCursor(coords(height - 1, ptr));
			char *s = new char[BufferSize];
			if (UseNormalMode) LinuxGetchSupporter::ResetConsoleSettings();
			SetCursorDisplay(display_show, display_enable);
			fgets(s, BufferSize - 1, stdin);
			if (UseNormalMode) LinuxGetchSupporter::ApplySCGSettings();
			SetCursorDisplay(display_show, display_disable);
			BarClean();
			GetClientArea().Draw();
			return s;
		}
#endif
		// Call this after all of work
		void MainLoop() {
			try {
				ResetConsole();
				SetEscapeOutput();
				SetCursorDisplay(display_show, display_disable);
				if (sub_controls.size() <= 0) {
					Protection::RaiseError("SCG Exception: Must have at least one sub-control");
					return;
				}
				for (auto &i : sub_controls) {
					i.second.MyControl().PreRender.RunEvent(pre_render_event_args(this));
				}
				current_active->second.MyControl().GotFocus.RunEvent(event_args());
				GetClientArea().Draw();	// Initial drawer
				while (true) {
					ProcessKey(GetKeyboard());
					//this_thread::yield();
				}
			}
			catch (scg_exception e) {
				Protection::RaiseError(string("SCG Exception:") + e.what());
			}
			catch (exception ne) {
				Protection::RaiseError(string("C++ Exception:") + ne.what());
			}
		}
	private:
		void ClearPrompt(pixel_color Style) {
			for (console_pos i = 0; i < width; i++) mc_area[height - 1][i] = spixel(' ', Style);
		}
		// Buffered
		client_area mc_area;
		mutex render_lock;
	};


	class window : public control {
	public:
		window(console_size Height, console_size Width) : mc_area(client_area(Height, Width)) {
			my_background = background_window;
			mc_area.Fillup(spixel(' ', background_window));
			// For title bar
			// Default LostFocus Drawer
			this->CurrentStyle = inactive_window;
			PreRender += [this](pre_render_event_args e) {
				this->my_master = e.MasterPointer;
				DrawBar(inactive_window);
				// Send DOWN PreRender()
				for (auto &i : sub_controls) {
					i.second.MyControl().PreRender.RunEvent(pre_render_event_args(this->my_master));
				}
				HasChanges = true;
			};
			OnResize += [this](resize_event_args e) {
				// Window don't invalidate its area!
				mc_area.Resize(e.NewHeight, e.NewWidth);
				//this->Width.fdata = e.NewWidth;
				//this->Height.fdata = e.NewHeight;
				mc_area.Fillup(client_area::pixel(' ', my_background.color_info));
				DrawBar(this->CurrentStyle);
				this->UpdateTitle(this->Title);
				HasChanges = true;
			};
			LostFocus += [this](event_args e) {
				// Set window title bar to inactive state
				this->CurrentStyle = inactive_window;
				DrawBar(inactive_window);
				HasChanges = true;
			};
			// Default GotFocus Drawer
			GotFocus += [this](event_args e) {
				// Set window title bar to active state
				this->CurrentStyle = active_window;
				DrawBar(active_window);
				HasChanges = true;
			};
			AfterDraw += [this](event_args e) {
				for (auto &i : sub_controls) {
					i.second.MyControl().AfterDraw.RunEvent(event_args());
				}
			};
		}
		virtual client_area& GetClientArea() {
			// Should be like this: Update all sons
			// Will become foreground if the order of dictionary of the name is bigger (like: 'Z' > 'A').
			// (For window: Has its borders (1 line))
			UpdateSubControls(&mc_area);
			return mc_area;
		}
		virtual void ProcessKey(keyboard_state KeyInfo) {
			// For active control ...
			switch (KeyInfo) {
			case go_prev_control:
				if (CurrentActivationAvailable()) current_active->second.MyControl().LostFocus.RunEvent(event_args());
				ActivePrevious();
				if (CurrentActivationAvailable()) current_active->second.MyControl().GotFocus.RunEvent(event_args());
				break;
			case go_next_control:
				if (CurrentActivationAvailable()) current_active->second.MyControl().LostFocus.RunEvent(event_args());
				ActiveNext();
				if (CurrentActivationAvailable()) current_active->second.MyControl().GotFocus.RunEvent(event_args());
				break;
			default:
				if (CurrentActivationAvailable()) {
					current_active->second.MyControl().ProcessKey(KeyInfo);
				}
				break;
			}
		}

		// Get or set the title of the whole window
		property<string> Title = property<string>([this](string &tmp) -> string {
			return tmp;
		}, [this](string sets, string &tmp) {
			tmp = sets;
			UpdateTitle(sets);
			// Draw title bar
		});
		
	private:

		void DrawBar(pixel_color Color) {
			for (console_size i = 0; i < mc_area.SizeW; i++) mc_area[0][i].color_info = Color;
		}

		void UpdateTitle(string TitleText) {
			if (mc_area.SizeW - 1 <= TitleText.length()) return;
			for (size_t i = 0; i < TitleText.length(); i++) mc_area[0][i] = TitleText[i];
			HasChanges = true;
		}

		pixel_color CurrentStyle;

		client_area mc_area;
		master *my_master;
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
			this->OnResize += [this](resize_event_args e) {
				mc_area.Resize(e.NewHeight, e.NewWidth);
				this->Width.fdata = e.NewWidth;
				this->Height.fdata = e.NewHeight;
				mc_area.Fillup(client_area::pixel(' ', my_background.color_info));
				this->RedrawText();
			};
		}

		virtual client_area& GetClientArea() {
			// Do nothing for child
			return mc_area;
		}
		virtual void ProcessKey(keyboard_state KeyInfo) {
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
			InvalidateText();
			buf = sets;
			RedrawText();
		}
			);

		property<console_size> Width = property<console_size>([this](console_size &buf) -> console_size {
			return buf;
		}, [this](console_size sets, console_size &buf) {
			InvalidateText();
			buf = sets;
			RedrawText();
		}
		);

		property<string> Text = property<string>([this](string &buf) -> string {
			return buf;
		}, [this](string sets, string &buf) {
			InvalidateText();
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
		
		void InvalidateText() {
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
				mc_area[CurrentX][CurrentY] = ' ';
				CurrentY++;
			}
			HasChanges = true;
		}

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
			mc_area.Fillup(spixel(' ', inactived_button));
			this->Height = Height;
			this->Width = Width;
			//this->Style = text_label;
			//this->HaveGotFocus = false;
			//this->IsActived = true;
			this->Text = Text;
			PreRender += [this](event_args e) {
				this->Enabled = true;
				this->DrawBar(inactived_button);
			};
			LostFocus += [this](event_args e) {
				// Set window title bar to inactive state
				HaveGotFocus = false;
				this->CurrentStyle = inactived_button;
				this->DrawBar(inactived_button);
			};
			// Default GotFocus Drawer
			GotFocus += [this](event_args e) {
				// Set window title bar to active state
				HaveGotFocus = true;
				this->CurrentStyle = actived_button;
				this->DrawBar(actived_button);
			};
			OnResize += [this](resize_event_args e) {
				mc_area.Resize(e.NewHeight, e.NewWidth);
				this->Width.fdata = e.NewWidth;
				this->Height.fdata = e.NewHeight;
				mc_area.Fillup(client_area::pixel(' ', my_background.color_info));
				RedrawText();
				DrawBar(CurrentStyle);
			};
		}

		virtual client_area& GetClientArea() {
			// Do nothing for child
			return mc_area;
		}

		virtual void ProcessKey(keyboard_state KeyInfo) {
			if (KeyInfo == active_button && Enabled) {
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
			this->CurrentStyle = sets;
			HasChanges = true;
		});

		property<bool> IsActived = property<bool>([this](bool &buf) -> bool {
			return Enabled;
		}, [this](bool sets, bool &buf) {
			Enabled = sets;
			if (sets) {
				if (HaveGotFocus) {
					this->DrawBar(actived_button);
					this->CurrentStyle = actived_button;
				}
				else {
					this->DrawBar(inactived_button);
					this->CurrentStyle = inactived_button;
				}
			}
			else {
				this->DrawBar(deactived_button);
				this->CurrentStyle = deactived_button;
			}
		});

		event<event_args> OnClick;

	protected:

		bool HaveGotFocus = false;
		pixel_color CurrentStyle = inactived_button;

		void DrawBar(pixel_color BarColor) {
			for (console_size i = 0; i < mc_area.SizeH; i++) {
				for (console_size j = 0; j < mc_area.SizeW; j++) {
					auto &m = mc_area[i][j];
					m.color_info = BarColor;
				}
			}
			HasChanges = true;
		}

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

	class input : public control {
	public:
		input(console_size Height, console_size Width, string Text = "") : mc_area(client_area(Height, Width)) {
			mc_area.Fillup(spixel(' ', background_input));
			this->Text = Text;
			this->Height = Height;
			this->Width = Width;
			this->GotFocus += [this](event_args e) {
				SetCursorDisplay(display_show, display_enable);
				MoveAbsoluteCursor(my_coords);
			};
			this->LostFocus += [this](event_args e) {
				SetCursorDisplay(display_show, display_disable);
			};
			this->AfterDraw += [this](event_args e) {
				MoveAbsoluteCursor(GetBaseCoords() + coords(MyCurrentX, MyCurrentY));
			};
			this->OnResize += [this](resize_event_args e) {
				mc_area.Resize(e.NewHeight, e.NewWidth);
				this->Width.fdata = e.NewWidth;
				this->Height.fdata = e.NewHeight;
				mc_area.Fillup(client_area::pixel(' ', my_background.color_info));
				this->DrawBar();
				this->RedrawText();
			};
		}

		virtual client_area& GetClientArea() {
			// Do nothing for child
			return mc_area;
		}

		virtual void ProcessKey(keyboard_state KeyInfo) {
			// Process input
			if (KeyInfo.PrimaryKey >= 32 && KeyInfo.PrimaryKey <= 126 && KeyInfo.ExtendedKey == 0) {
				// Acceptable input ...
				this->Text = this->Text.GetValue() + char(KeyInfo.PrimaryKey);
			}
			else if (KeyInfo.PrimaryKey == delete_key) {
				// Delete ...
				string s = this->Text.GetValue();
				if (s.length())
					this->Text = s.substr(0, s.length() - 1);
			}
			else if (KeyInfo.PrimaryKey == 9) {
				// Tab ...
				this->Text = this->Text.GetValue() + '\t';
			}
			else if (KeyInfo.PrimaryKey == 13) {
				// Enter ...
				this->Text = this->Text.GetValue() + '\n';
			}
		}

		virtual control_symbol operator += (control_place others) {
			throw scg_exception("Cannot add sub control for a textbox");
		}

		virtual bool operator -= (control_symbol others) {
			throw scg_exception("Cannot remove sub control from a textbox");
		}

		virtual void ActiveNext() {
			throw scg_exception("Cannot active sub control of a textbox");
		}

		virtual void ActivePrevious() {
			throw scg_exception("Cannot active sub control of a textbox");
		}

		property<console_size> Height = property<console_size>([this](console_size &buf) -> console_size {
			return buf;
		}, [this](console_size sets, console_size &buf) {
			InvalidateText();
			buf = sets;
			RedrawText();
		}
		);

		property<console_size> Width = property<console_size>([this](console_size &buf) -> console_size {
			return buf;
		}, [this](console_size sets, console_size &buf) {
			InvalidateText();
			buf = sets;
			RedrawText();
		}
		);

		property<string> Text = property<string>([this](string &buf) -> string {
			return buf;
		}, [this](string sets, string &buf) {
			InvalidateText();
			buf = sets;
			OnChange.RunEvent(event_args());
			RedrawText();
		});

		property<pixel_color> Style = property<pixel_color>([this](pixel_color &buf) -> pixel_color {
			return buf;
		}, [this](pixel_color sets, pixel_color &buf) {
			buf = sets;
			DrawBar();
			HasChanges = true;
		});

		event<event_args> OnChange;

	protected:

		void DrawBar() {
			console_size MyHeight = Height, MyWidth = Width;
			for (console_pos i = 0; i < MyHeight; i++) {
				for (console_pos j = 0; j < MyWidth; j++) {
					mc_area[i][j].color_info = this->Style.fdata;
				}
			}
			HasChanges = true;
		}

		void InvalidateText() {
			string TextData = this->Text;
			console_size MyHeight = Height, MyWidth = Width;
			console_pos CurrentX = 0, CurrentY = 0;	// X: linear pos, Y: wide pos.
			for (auto &i : TextData) {
				if (i == '\n') {
					CurrentX++;
					CurrentY = 0;
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
				mc_area[CurrentX][CurrentY] = ' ';
				CurrentY++;
			}
			HasChanges = true;
		}

		console_pos MyCurrentX, MyCurrentY;

		void RedrawText() {
			string TextData = this->Text;
			console_size MyHeight = Height, MyWidth = Width;
			console_pos CurrentX = 0, CurrentY = 0;	// X: linear pos, Y: wide pos.
			for (auto &i : TextData) {
				if (i == '\n') {
					CurrentX++;
					CurrentY = 0;
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
			MyCurrentX = CurrentX;
			MyCurrentY = CurrentY;
			HasChanges = true;
		}

		client_area mc_area;
	};

	// Including both radio and multiple checkbox.
	class checkbox : public control {
	public:

		struct radio_group {
			vector<checkbox*> others;
		};

		checkbox(string Text, console_size Height, console_size Width, bool Checked = false, radio_group *RadioGroup = nullptr) : mc_area(client_area(Height, Width)) {
			mc_area.Fillup(spixel(' ', text_label));
			this->Height = Height;
			this->Width = Width;
			this->Text = Text;
			if (RadioGroup != nullptr) {
				this->RadioGroup = RadioGroup;
				RadioGroup->others.push_back(this);
				this->IsRadio = true;
			}
			else {
				this->IsRadio = false;
			}
			this->IsChecked.fdata = Checked;
			this->DrawStatus(Checked);
			PreRender += [this](event_args e) {
				this->Enabled = true;
				this->DrawBar(inactived_button);
				CurrentStyle = inactived_button;
			};
			LostFocus += [this](event_args e) {
				// Set window title bar to inactive state
				HaveGotFocus = false;
				this->DrawBar(inactived_button);
				CurrentStyle = inactived_button;
			};
			// Default GotFocus Drawer
			GotFocus += [this](event_args e) {
				// Set window title bar to active state
				HaveGotFocus = true;
				this->DrawBar(actived_button);
				CurrentStyle = actived_button;
			};
			OnResize += [this](resize_event_args e) {
				mc_area.Resize(e.NewHeight, e.NewWidth);
				this->Width.fdata = e.NewWidth;
				this->Height.fdata = e.NewHeight;
				mc_area.Fillup(client_area::pixel(' ', my_background.color_info));
				this->DrawStatus(this->IsChecked);
				this->DrawBar(this->CurrentStyle);
			};
		}

		virtual client_area& GetClientArea() {
			// Do nothing for child
			return mc_area;
		}
		virtual void ProcessKey(keyboard_state KeyInfo) {
			if (KeyInfo == active_button && Enabled) {
				this->IsChecked = !this->IsChecked;
			}
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
			InvalidateText();
			buf = sets;
			RedrawText();
		}
		);

		property<console_size> Width = property<console_size>([this](console_size &buf) -> console_size {
			return buf;
		}, [this](console_size sets, console_size &buf) {
			InvalidateText();
			buf = sets;
			RedrawText();
		}
		);

		property<string> Text = property<string>([this](string &buf) -> string {
			return buf;
		}, [this](string sets, string &buf) {
			InvalidateText();
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
			CurrentStyle = sets;
			HasChanges = true;
		});


		property<bool> IsRadioCheckbox = property<bool>([this](bool &tmp) -> bool {
			return this->IsRadio;
		}, [this](bool sets, bool &tmp) {
			throw scg_exception("Cannot set whether the control is radio");
		});

		// Status is for: Checked or not
		property<bool> IsChecked = property<bool>([this](bool &tmp) -> bool {
			return tmp;
		}, [this](bool sets, bool &tmp) {
			if (sets && this->IsRadio) {
				// Set others
				if (this->RadioGroup == nullptr) {
					throw scg_exception("Bad radio group");
				}
				for (auto &i : this->RadioGroup->others) {
					if (i->my_symbol == this->my_symbol) continue;
					i->IsChecked = false;
				}
			}
			if (tmp != sets) {
				DrawStatus(sets);
				tmp = sets;
				OnStatusChange.RunEvent(event_args());
			}
		});

		event<event_args> OnStatusChange;

	protected:

		bool IsRadio = false;
		radio_group *RadioGroup = nullptr;

		bool HaveGotFocus = false;

		pixel_color CurrentStyle;

		void DrawBar(pixel_color BarColor) {
			for (console_size i = 0; i < mc_area.SizeH; i++) {
				console_size j = 0;
				if (i == 0) j = rsize_checkbox + 1;
				for (; j < mc_area.SizeW; j++) {
					auto &m = mc_area[i][j];
					m.color_info = BarColor;
				}
			}
			HasChanges = true;
		}

		void DrawStatus(bool NewStatus) {
			console_pos Current = 0;
			if (NewStatus) {
				if (this->IsRadio) {
					for (auto &i : radio_checkbox) {
						mc_area[0][Current] = i;
						Current++;
					}
				}
				else {
					for (auto &i : multiple_checkbox) {
						mc_area[0][Current] = i;
						Current++;
					}
				}
			}
			else {
				for (auto &i : unselected_checkbox) {
					mc_area[0][Current] = i;
					Current++;
				}
			}
			HasChanges = true;
		}

		void InvalidateText() {
			string TextData = this->Text;
			console_size MyHeight = Height, MyWidth = Width;
			console_pos CurrentX = 0, CurrentY = rsize_checkbox + 1;	// X: linear pos, Y: wide pos.
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
				mc_area[CurrentX][CurrentY] = ' ';
				CurrentY++;
			}
			HasChanges = true;
		}

		void RedrawText() {
			string TextData = this->Text;
			console_size MyHeight = Height, MyWidth = Width;
			console_pos CurrentX = 0, CurrentY = rsize_checkbox + 1;	// X: linear pos, Y: wide pos.
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


	private:
		client_area mc_area;
	};

	using check_group = checkbox::radio_group;

	class timer : public invisible_control {

	public:

		class ignition_failure_exception : public exception {
		public:
			using exception::exception;
		};

		void RunMe() {
			this->ToCall.RunEvent(event_args());
			if ((!NoRendering) && this->my_master != nullptr) {
				this->my_master->MasterRender();
			}
		}

		void IgniteMe(master *to_be) {
			if (this->my_master != nullptr) {
				return;
			}
			this->my_master = to_be;
			this->my_thr = thread([this]() {
				while (true) {
					if (this->Enabled) {
						RunMe();
					}
					this_thread::sleep_for(chrono::milliseconds(this->Interval));
				}
			});
			if (this->my_thr.joinable()) {
				this->my_thr.detach();
			}
			else {
				throw ignition_failure_exception();
			}
			
		}

		timer(int Interval = 100, bool Enabled = true) : Interval(Interval), Enabled(Enabled) {
/*#if !defined(__GNUC__)
			invisible_control::invisible_control();
#endif*/
			this->PreRender += [this](pre_render_event_args e) {
				this->IgniteMe(e.MasterPointer);
			};
		}

		bool NoRendering = false;
		event<event_args> ToCall;
		bool Enabled;
		int Interval;

	private:
		master *my_master = nullptr;
		thread my_thr;
	};

};