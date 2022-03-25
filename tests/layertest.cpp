#include <iostream>

#include <LayerManager.hpp>

using namespace std;

int main() {
	Layer l;
	std::vector<float> v{ 0, .1, .2, .3, .5, .8, .9, .8, .8, .4, .5 };
	cout << l.getSize() << endl;
	l.setSize(10);

	cout << l.getSize() << endl;
	l.setFrom(v);
	return 0;
}
