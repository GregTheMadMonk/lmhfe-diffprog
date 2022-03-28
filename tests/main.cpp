// STL headers
#include <iostream>

// Local headers
#include <MHFE.hpp>

using namespace std;

int main() {
	cout << "MHFE" << endl;

	using Topology = TNL::Meshes::Topologies::Triangle;
	using MHFE = MHFE::MHFE<Topology>;

	MHFE mhfe;
	mhfe.domain.generateRectangularDomain(20, 10, .5, 1);
	mhfe.prepare();

	mhfe.domain.template forBoundary<MHFE::DomainType::dimensions() - 1>([&] (int edge) {
		const auto p1 = mhfe.domain.getMesh().getPoint(mhfe.domain.template getSubentityIndex<1, 0>(edge, 0));
		const auto p2 = mhfe.domain.getMesh().getPoint(mhfe.domain.template getSubentityIndex<1, 0>(edge, 1));

		const auto r = p2 - p1;

		if (r[0] == 0) {
			mhfe.domain.layers.edge.template get<int>(MHFE::Layer::DIRICHLET_MASK)[edge] = 1;
			mhfe.domain.layers.edge.template get<float>(MHFE::Layer::DIRICHLET)[edge] = (p1[0] == 0) * 1;
			mhfe.domain.layers.edge.template get<float>(MHFE::Layer::TP)[edge] = (p1[0] == 0) * 1;
		} else if (r[1] == 0) {
			mhfe.domain.layers.edge.template get<int>(MHFE::Layer::NEUMANN_MASK)[edge] = 1;
			mhfe.domain.layers.edge.template get<float>(MHFE::Layer::NEUMANN)[edge] = 0;
		}
	});

	mhfe.init();

	do {
		cout << "\r[" << setw(10) << left << mhfe.t << "/" << right << mhfe.T << "]";
		cout.flush();

		mhfe.snapshot("saved");
	} while (mhfe.step());

	cout << endl << "DONE" << endl;
	return 0;
}
