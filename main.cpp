// For test propose.
#include "scg.h"
#include <iostream>
using namespace scg;

int main() {

	master m;

	window an = window(4, 20);
	an.Title = "20 Widths";
	m += control_set(&an, "Fxample", coords(15, 15));

	window w = window(5, 15);
	w.Title = "Test!";
	label l = label("Test", 1, 5);
	w += control_set(&l, "LabelTest", coords(2, 2));
	w.LostFocus += [&](event_args e) {
		l.Text = "NO! ";
		//l.Style = pixel_colors::Generate(text_background + text_cyan + text_intensity, text_foreground + text_white + text_intensity);
		//m.MoveControl("1020", coords(10, 20));
	};

	// Add label for this example window:
	
	m += control_set(&w, "Example", coords(2, 2));

	window c = window(10, 20);
	c.Title = "10x20 Window Test";

	button bb = button("Example 1", 1, 10);
	button bc = button("Example 2", 1, 10);
	label t = label("Test", 1, 5);

	bb.OnClick += [&](event_args e) {
		t.Text = "Press";
		bc.Text = "Press me!";
		bc.IsActived = !bc.IsActived;
		c.MoveControl("Button2", coords(7, 4));
	};

	input ip = input(2, 8);

	c += control_set(&bb, "Button1", coords(2, 2));
	c += control_set(&bc, "Button2", coords(3, 2));
	c += control_set(&t, "Label1", coords(4, 2));
	c += control_set(&ip, "Input1", coords(5, 1));

	m += control_set(&c, "1020", coords(4, 4));

	m.MainLoop();

	return 0;
}