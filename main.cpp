// For test propose.
#include "scg.h"
#include <iostream>
using namespace scg;

int main() {

	master m;
	window w = window(5, 15);
	w.Title = "Test!";
	m += control_set(&w, "Example", coords(2, 2));
	window an = window(4, 20);
	an.Title = "20 Widths";
	m += control_set(&an, "Fxample", coords(15, 15));
	window c = window(10, 20);
	c.Title = "10x20 Window Test";
	m += control_set(&c, "1020", coords(9, 9));

	m.MainLoop();

	return 0;
}