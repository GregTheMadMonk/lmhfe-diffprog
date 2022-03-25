#pragma once

// STL headers
// ...

// TNL headers
#include <TNL/Solvers/LinearSolverTypeResolver.h>
#include <TNL/Timer.h>

// Local headers
// ...

namespace MHFE {

MHFE_TEMPLATE
inline void MHFE MHFE_TARGS::prepare() {
	if (domain.isClean())
		throw std::runtime_error("Cannot prepare solver to work with an empty domain!");

	// Clear old domain layers, and create new
	domain.layers.cell.clear();
	domain.layers.edge.clear();

	domain.layers.cell.template add<Real>(0);	// Index 0, P
	domain.layers.edge.template add<Real>(0);	// Index 0, DIRICHLET
	domain.layers.edge.template add<Real>(0);	// Index 1, NEUMANN
	domain.layers.edge.template add<Real>(0);	// Index 2, TP
	domain.layers.edge.template add<int>(0);	// Index 3, DIRICHLET_MASK
	domain.layers.edge.template add<int>(0);	// Index 4, NEUMANN_MASK
	domain.layers.edge.template add<Index>(0);	// Index 5, ROW_CAPACITIES
	domain.layers.edge.template add<Real>(0);	// Index 6, RIGHT
}

MHFE_TEMPLATE
inline void MHFE MHFE_TARGS::init() {
	const auto dMask = domain.layers.edge.template get<int>(Layer::DIRICHLET_MASK).getConstView();
	const auto nMask = domain.layers.edge.template get<int>(Layer::NEUMANN_MASK).getConstView();
	auto capacities = domain.layers.edge.template get<Index>(Layer::ROW_CAPACITIES).getView();

	// Check that border conditions are set
	domain.template forBoundary<DomainType::dimensions() - 1>([&] (Index edge) {
		if ((dMask[edge] == 0) && (nMask[edge] == 0))
			throw std::runtime_error("No Dirichlet or Neumann conditions set on border " + std::to_string(edge));
	});

	// Fill capacities
	domain.template forAll<DomainType::dimensions() - 1>([&] (Index edge) {
		capacities[edge] = 1;
		if ((dMask[edge] != 0) && (nMask[edge] == 0)) return;

		const auto cells = domain.getSuperentitiesCount(edge);
		for (int cell = 0; cell < cells; ++cell) {
			const auto gCellIdx = domain.getSuperentityIndex(edge, cell);
			capacities[edge] += domain.getSubentitiesCount(gCellIdx) - 1;
		}
	});
}

MHFE_TEMPLATE
inline bool MHFE MHFE_TARGS::step() {
	const auto edges = domain.template getEntitiesCount<DomainType::dimensions() - 1>();

	// Copy the previous solution
	auto& P = domain.layers.cell.template get<Real>(Layer::P);
	const auto PPrev(P);

	// System matrix
	MatrixP m = std::make_shared<Matrix>(edges, edges);
	m->setRowCapacities(domain.layers.edge.template get<Index>(Layer::ROW_CAPACITIES));
	m->forAllElements([] (int rowIdx, int localIdx, int& colIdx, Real& v) { v = 0; });

	auto mView = m->getView();

	// Needed layer views
	auto rightView = domain.layers.edge.template get<Real>(Layer::RIGHT).getView();
	auto TPView = domain.layers.edge.template get<Real>(Layer::TP).getView();
	const auto dMask = domain.layers.edge.template get<int>(Layer::DIRICHLET_MASK).getView();
	const auto nMask = domain.layers.edge.template get<int>(Layer::NEUMANN_MASK).getView();
	const auto dView = domain.layers.edge.template get<Real>(Layer::DIRICHLET).getView();
	const auto nView = domain.layers.edge.template get<Real>(Layer::NEUMANN).getView();

	// Reset the right part
	rightView.forAllElements([] (Index i, Real& v) { v = 0; });

	const auto Q_part = [&] (const Index& cell, const Index& edge, const Real& right) {
		const auto edgeCount = domain.getSubentitiesCount(cell);
		const auto area = domain.getCellMeasure(cell);

		const auto lambda = c * area / tau;

		for (Index lei = 0; lei < edgeCount; ++lei) {
			const Index gEdge = domain.getSubentityIndex(cell, lei);
			const auto [B, l] = B_inv(domain, edge, gEdge, cell);
			const auto alpha = 3.0 / l;
			const auto beta = lambda + a * alpha;
			const auto delta = a * (B - a / l / l / beta);
			const auto lumping = 0;
			mView.addElement(edge, gEdge, delta + lumping);
		}

		rightView[edge] += right;
	};

	domain.template forAll<DomainType::dimensions() - 1>([&] (Index edge) {
		const auto eCells = domain.getSuperentitiesCount(edge);

		if (dMask[edge] + nMask[edge] != 0) {
			if (dMask[edge] != 0) {
				mView.addElement(edge, edge, 1);
				rightView[edge] += dView[edge];
			}
			if (nMask[edge] != 0) {
				for (Index lCell = 0; lCell < eCells; ++lCell) {
					const auto cell = domain.getSuperentityIndex(edge, lCell);
					Q_part(cell, edge, 0);
				}
				rightView[edge] += nView[edge];
			}

			return;
		}

		for (Index lCell = 0; lCell < eCells; ++lCell) {
			const auto cell = domain.getSuperentityIndex(edge, lCell);
			const auto area = domain.getCellMeasure(cell);
			Q_part(cell, edge, c * area * TPView[edge] / 3.0 / tau);
		}
	});

	auto stepSolver = TNL::Solvers::getLinearSolver<Matrix>("gmres");
	// auto stepPrecond = TNL::Solvers::getPreconditioner<Matrix>("ilu0");
	// stepPrecond->update(m);
	stepSolver->setMatrix(m);
	// stepSolver->setPreconditioner(stepPrecond);

	using IterativeSolverMonitorType = TNL::Solvers::IterativeSolverMonitor< Real, Index >;
	IterativeSolverMonitorType monitor;
	TNL::Solvers::SolverMonitorThread mmonitorThread(monitor);
	monitor.setRefreshRate(10);  // refresh rate in milliseconds
	monitor.setVerbose(1);
	monitor.setStage( "Stage:" );
	TNL::Timer timer;
	monitor.setTimer( timer );
	timer.start();
	stepSolver->setSolverMonitor(monitor);

	stepSolver->solve(rightView, TPView);
	monitor.stopMainLoop();

	std::cout << TPView << std::endl;

	const auto PPrevView = PPrev.getConstView();
	P.forAllElements([&] (Index cell, Real PCell) {
		const auto area = domain.getCellMeasure(cell);
		const auto lambda = c * area / tau;
		const auto [B, l] = B_inv(domain, 0, 0, cell);
		const auto beta = lambda + a * 3.0 / l;

		PCell = PPrevView[cell] * lambda / beta;

		const auto edgeCount = domain.getSubentitiesCount(cell);
		for (Index lei = 0; lei < edgeCount; ++lei)
			PCell += a * TPView[domain.getSubentityIndex(cell, lei)] / beta / l;
	});

	t += tau;
	return t < T;
}

} // <-- namespace MHFE
