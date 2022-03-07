#include <iostream>
#include <cmath>

#include <Domain.hpp>

using namespace std;

int main() {
	cout << "Domain test" << endl;

	using DomainType = Domain<TNL::Meshes::Topologies::Triangle>;

	DomainType domain;
	domain.generateRectangularDomain(10, 10, 1, 1);
	domain.write("domain1.vtk");
	DomainType domain2;
	domain2.loadFromMesh("domain1.vtk");
	const auto layer = domain2.addRealLayer(DomainType::Layer::Cell);
	domain2.getRealLayer(DomainType::Layer::Cell, layer).forAllElements([] (int i, float& v) {
		v = sin(i * 0.1);
	});
	domain2.write("domain2.vtk");

	auto domain3 = move(domain2);
	domain3.clear();
	domain3.generateRectangularDomain(20, 20, .5, .5);
	domain3.write("domain3.vtk");
	return 0;
}
