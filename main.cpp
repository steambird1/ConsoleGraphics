// For test propose.
#include "scg.h"
#include <iostream>
using namespace scg;

int main() {

	master m;

	//window w = window(5, 15);
	//w.Title = "Window";
	button BlueBetter = button("BlueBetter", 1, 10);
	//w += control_set(&BlueBetter, "BlueBetter", coords(1, 0));
	
	m += control_set(&BlueBetter, "Main", coords(3, 3));

	m.MainLoop();
	return 0;
}