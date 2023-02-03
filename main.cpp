// For test propose.
#include "scg.h"
#include <iostream>
using namespace scg;

int main() {

	master m;

	window w = window(10, 15);
	w.Title = "Window";

	check_group ch;

	checkbox c = checkbox("Example", 1, 12, false, &ch);
	checkbox d = checkbox("Another", 1, 12, true, &ch);

	button l = button("Test", 1, 4);
	input ip = input(5, 5);

	l.OnClick += [&](event_args e) {
		m.BarPrompt("Button is clicked", pixel_colors::Generate(text_green + text_background, text_white + text_intensity + text_foreground));
	};

	c.OnStatusChange += [&](event_args e) {
		if (c.IsChecked) {
			m.BarPrompt("Example is checked", pixel_colors::Generate(text_blue + text_background, text_white + text_intensity + text_foreground));
			l.Text = "HeyGuy";
			w.ResizeControl("Label1", 1, 6);
			l.IsActived = true;
		}
		else {
			m.BarClean();
			l.IsActived = false;
		}
	};

	ip.OnChange += [&](event_args e) {
		m.BarPrompt(string("Input: ") + ip.Text.GetValue(), pixel_colors::Generate(text_black + text_background, text_white + text_intensity + text_foreground, text_bold));
	};

	w += control_set(&c, "Check1", coords(1, 1));
	w += control_set(&d, "Check2", coords(2, 1));
	w += control_set(&l, "Label1", coords(3, 1));
	w += control_set(&ip, "Input1", coords(4, 1));

	//button BlueBetter = button("BlueBetter", 1, 10);
	//w += control_set(&BlueBetter, "BlueBetter", coords(1, 0));
	
	m += control_set(&w, "Main", coords(3, 3));

	window antw = window(8,8);
	antw.Title = "Timer";
	label lb = label("0", 1, 5);
	long long counter = 0;
	timer t = timer(1000);
	t.ToCall += [&](event_args e) {
		counter++;
		lb.Text = to_string(counter);
		//m.BarPrompt(string("Timer: ") + to_string(counter), pixel_colors::Generate(text_black + text_background, text_white + text_intensity + text_foreground));
	};
	
	antw += control_set(&lb, "Label2", coords(1, 1));

	m += control_set(&t, "Timer1", coords(1, 1));
	m += control_set(&antw, "Window2", coords(11, 12));

	m.MainLoop();
	return 0;
}