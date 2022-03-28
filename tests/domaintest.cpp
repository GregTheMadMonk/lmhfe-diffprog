#include <iostream>
#include <cmath>

#include <Domain.hpp>

using namespace std;

int main() {
	cout << "Domain test" << endl;

	using DomainType = Domain<TNL::Meshes::Topologies::Triangle>;

	DomainType domain;

	domain.generateRectangularDomain(10, 10, 1, 1);
	domain.write("domain1.vtu");
	DomainType domain2;
	domain2.loadFromMesh("domain1.vtu");
	const auto layer = domain2.layers.cell.template add<float>();
	domain2.layers.cell.template get<float>(layer).forAllElements([] (int i, float& v) {
		v = sin(i * 0.1);
	});
	const auto layer2 = domain2.layers.edge.template add<float>();
	domain2.layers.edge.template get<float>(layer2).forAllElements([] (int i, float& v) {
		v = sin(.1 * i);
	});
	domain2.write("domain2.vtu");

	auto domain3 = domain2;
	domain3.clear();
	domain3.generateRectangularDomain(20, 20, .5, .5);
	domain3.write("domain3.vtu");

	auto domain4 = move(domain2);

	try {
		domain2.layers.edge.template get<float>(layer2);
	} catch (...) {
		cout << "Move worked, couldn't access domain2's layers!" << endl;
	}

	domain4.write("domain4.vtu");
	domain2 = move(domain4); // This crashes because (why?)

	try {
		domain2.layers.edge.template get<float>(layer2);
	} catch (...) {
		cout << "Data didn't move back!" << endl;
	}

	return 0;
}
