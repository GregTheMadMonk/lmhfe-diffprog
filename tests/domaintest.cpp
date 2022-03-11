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
	const auto layer = domain2.layers.cell.template add<float>();
	domain2.layers.cell.template get<float>(layer).data.forAllElements([] (int i, float& v) {
		v = sin(i * 0.1);
	});
	const auto layer2 = domain2.layers.edge.template add<float>();
	domain2.layers.edge.template get<float>(layer2).data.forAllElements([] (int i, float& v) {
		v = sin(.1 * i);
	});
	domain2.write("domain2.vtk");

	auto domain3 = move(domain2);
	domain3.clear();
	domain3.generateRectangularDomain(20, 20, .5, .5);
	domain3.write("domain3.vtk");
	return 0;
}
