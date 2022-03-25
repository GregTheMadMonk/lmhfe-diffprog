#pragma once

// STL headers
#include <memory>
#include <tuple>

// TNL headers
#include <TNL/Matrices/SparseMatrix.h>

// Local headers
#include "Domain.hpp"

#define MHFE_TEMPLATE	template <typename CellTopology, typename Device, typename Real, typename Index>
#define MHFE_TARGS	<CellTopology, Device, Real, Index>

namespace MHFE {

template <typename CellTopology, typename Device = TNL::Devices::Host, typename Real = float, typename Index = int>
struct MHFE {
	// Type aliases
	using Matrix	= TNL::Matrices::SparseMatrix<Real, Device, Index>;
	using MatrixP	= std::shared_ptr<Matrix>; // TNL solvers accept shared pointers to a matrix
	using DomainType= Domain<CellTopology, Device, Real, Index>;

	DomainType domain;

	// Layer info
	enum Layer : Index {
		/* Real cell layers */
		P		= 0,	// P over cells
		/* TODO: Layers for problem coefficients a&c */
		/* Index cell layers */
		// Nothing
		/* Real edge layers */
		DIRICHLET	= 0,	// P[edge] = DIRICHLET[edge] where mask != 0
		NEUMANN		= 1,	// qη[edge] = Q[edge] = NEUMANN[edge] where mask != 0
		TP		= 2,	// P over edges (TP)
		RIGHT		= 6,	// System right-part
		/* Index edge layers */
		DIRICHLET_MASK	= 3,
		NEUMANN_MASK	= 4,
		ROW_CAPACITIES	= 5,	// System matrix row capacities
	};

	// Current solution time
	Real t = 0;

	// Problem parameters
	Real tau= 0.005;// Time step
	Real T	= 10;	// Time interval length

	// Coefficients
	Real a = 1;
	Real c = 1;

	// Prepare necessary domain data layers
	void prepare();

	// Initialize
	void init();

	// MHFE step
	bool step();

	// Create a snapshot in a folder
	void snapshot(const std::string& dir) { domain.write(dir + "/" + std::to_string(t) + ".vtu"); }
}; // <-- struct MHFE

template <typename CellTopology, typename Device, typename Real, typename Index>
std::tuple<Real, Real> B_inv(	const Domain<CellTopology, Device, Real, Index>& domain,
				const Index& edge1,
				const Index& edge2,
				const Index& cell	) {
	throw std::runtime_error("B_inv is not implemented for this topology!");
}

template <typename Device, typename Real, typename Index>
std::tuple<Real, Real> B_inv(	const Domain<TNL::Meshes::Topologies::Triangle, Device, Real, Index>& domain,
				const Index& edge1,
				const Index& edge2,
				const Index& cell	) {
	using DomainType = Domain<TNL::Meshes::Topologies::Triangle, Device, Real, Index>;
	using PointType = typename DomainType::MeshType::PointType;
	PointType r1(0, 0), r2(0, 0);

	Real sqSum = 0;

	for (Index k = 0; k < 3; ++k) {
		const auto edge = domain.template getSubentityIndex<2, 1>(cell, k);
		const auto p1 = domain.getMesh().getPoint(domain.template getSubentityIndex<1, 0>(edge, 0));
		const auto p2 = domain.getMesh().getPoint(domain.template getSubentityIndex<1, 0>(edge, 1));

		const auto r = p2 - p1;

		if (edge == edge1)	r1 = r;
		else if (edge == edge2)	r2 = r;

		sqSum += (r, r);
	}

	const auto area = domain.getCellMeasure(cell);
	const Real l = sqSum / 48.0 / area;

	return std::make_tuple((r1, r2) / area + 1.0 / l / 3.0, l);
}

} // <-- namespace MHFE

#include "MHFE_impl.hpp"
