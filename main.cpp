// For test propose.
#include "scg.h"
#include <iostream>
using namespace scg;

int main() {
	
	auto a = array_2d<int>(3, 4);

	a[2][3] = 4;
	a[3][2] = 3;

	//a.Size1D = 4;	// Raises excaption!

	cout << a.Size1D << "," << a.Size2D << endl << a[2][3] << "," << a[3][2] << endl;

	return 0;
}