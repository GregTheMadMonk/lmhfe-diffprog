#pragma once

#include <iostream>	// Here for debugging

#include <filesystem>
#include <fstream>
#include <memory>	// std::shared_ptr
#include <string>

#include <TNL/Algorithms/ParallelFor.h>
#include <TNL/Containers/StaticVector.h>
#include <TNL/Containers/Vector.h>
#include <TNL/Matrices/SparseMatrix.h>
#include <TNL/Pointers/SharedPointer.h>
#include <TNL/Solvers/LinearSolverTypeResolver.h>

template <typename Real, typename Device, typename Index = int>
class mhfe {
	public:
	using Matrix	= TNL::Matrices::SparseMatrix<Real, Device, Index>;
	using Matrix_p	= std::shared_ptr<Matrix>;
	using v2d	= TNL::Containers::StaticVector<2, Real>;
	using Vector	= TNL::Containers::Vector<Real, Device, Index>;
	using IVector	= TNL::Containers::Vector<Index, Device, Index>;

	// Variables for solution
	Vector P;	// Solution - average vaules over elements
	Vector P_prev;	// Solution on a previous step
	Vector TP;	// Average values over element edges
	Matrix_p m;	// System matrix
	Vector right;	// System right-hand side vector

	// Current time value
	Real t;

	// Directory where to save files
	std::string saved_dir = "./saved";

	/*
	 * Element indices:
	 *   ix=0   ix=1   ix=2   ix=3
	 * +------+------+------+------+
	 * |\  1  |\  3  |\   5 |\   7 |
	 * |  \   |  \   |  \   |  \   |  iy = 0
	 * | 0  \ | 2  \ | 4  \ | 6  \ |
	 * +------+------+------+------+
	 * |\  9  |\  11 |\  13 |\  15 |
	 * |  \   |  \   |  \   |  \   |  iy = 1
	 * | 8  \ |10  \ | 12 \ | 14 \ |
	 * +------+------+------+------+
	 * "edge" value
	 * +--0---+--0---+---0--+--0---+
	 * |\     |\     |\     |\     |
	 * 1  2   1  2   1  2   1  2   1
	 * |    \ |    \ |    \ |    \ |
	 * +--0---+--0---+--0---+--0---+
	 * |\     |\     |\     |\     |
	 * 1  2   1  2   1  2   1  2   1
	 * |    \ |    \ |    \ |    \ |
	 * +--0---+--0---+--0---+--0---+
	 *
	 * Edge values correspond to edges in the paper as:
	 * A_k -> 0
	 * A_j -> 1
	 * A_i -> 2
	 *
	 * Edge indices:
	 * +--0---+--1---+---2--+--3---+
	 * |\     |\     |\     |\     |
	 * 4  5   6  7   8  9   10 11  12
	 * |    \ |    \ |    \ |    \ |
	 * +--13--+--14--+--15--+--16--+
	 * |\     |\     |\     |\     |
	 * 17 18  19 20  21 22  23 24  25
	 * |    \ |    \ |    \ |    \ |
	 * +-26---+--27--+--28--+--29--+
	 */

	public:
	enum Edge : Index {
		EDGE_I = 2,
		EDGE_J = 1,
		EDGE_K = 0
	};

	static Index __cuda_callable__ element_index(const Index& ix, const Index& iy, const Index& upper, const Index& Nx_a) {
		return 2 * (ix + iy * Nx_a) + upper;
	}
	static void __cuda_callable__ get_element_params(Index index, Index& ix, Index& iy, Index& upper, const Index& Nx, const Index& Ny) {
		upper = index % 2;
		index /= 2;
		ix = index % Nx;
		iy = index / Nx;
	}
	static Index __cuda_callable__ edge_index(Index ix, const Index& iy, const Index& upper, const Edge& edge, const Index& Nx, const Index& ny) {
		const auto per_row = 3 * Nx + 1;
		switch (edge) {
			case EDGE_K:
				return ix + per_row * (iy + 1 - upper);
			case EDGE_J:
				return 2 * ix + Nx + upper * 2 + per_row * iy;
			case EDGE_I:
				return 2 * ix + Nx + 1 + per_row * iy;
			default: // Not happening, here to get rid of a warning
				return INT_MAX;
		}
	};
	static Index __cuda_callable__ get_neighbour_elements(Index edge, Index& e1, Index& e2, Edge& type, const Index& Nx, const Index& Ny) {
		// TODO: Optimize
		Index ret = 0;
		for (Index ix = 0; ix < Nx; ++ix) for (Index iy = 0; iy < Ny; ++iy)
			for (Index u = 0; u < 2; ++u)
				for (int e = 0; e < 3; ++e) if (edge_index(ix, iy, u, (Edge)e, Nx, Ny) == edge) {
					type = (Edge)e;
					if (++ret > 1) e2 = element_index(ix, iy, u, Nx);
					else e1 = element_index(ix, iy, u, Nx);
				}
		return ret;
	};
	static v2d __cuda_callable__ edge_vector(const Edge& edge, const Index& upper, const Real& dx, const Real& dy) {
		v2d ret;

		switch (edge) {
			case EDGE_K:
				ret = -v2d(dx, 0);
				break;
			case EDGE_J:
				ret = -v2d(0, dy);
				break;
			case EDGE_I:
				ret = v2d(dx, dy);
				break;
		}

		return ret;
	};
	static v2d __cuda_callable__ edge_midpoint(const Index& edge, const Real& dx, const Real& dy, const Index& Nx, const Index& Ny) {
		Index e1, e2;
		Edge type;
		get_neighbour_elements(edge, e1, e2, type, Nx, Ny);
		Index ix, iy, u;
		get_element_params(e1, ix, iy, u, Nx, Ny);

		Real x = dx * ix;
		Real y = dy * iy;
		switch (type) {
			case EDGE_I:
				x += dx / 2;
				y += dy / 2;
				break;
			case EDGE_J:
				y += dy / 2;
				x += dx * u;
				break;
			case EDGE_K:
				x += dx / 2;
				y += dy * (1 - u);
				break;
		}

		return v2d(x, y);
	}

	static Index __cuda_callable__ get_elems(const Index& Nx, const Index& Ny) {
		return Nx * Ny * 2;
	}
	static Index __cuda_callable__ get_edges(const Index& Nx, const Index& Ny) {
		return (3 * Nx + 1) * Ny + Nx;
	}

	// Problem parameters
	// Domain dimensions
	Real X = 20;
	Real Y = 10;

	// Grid rectangles count
	Index Nx = 20;
	Index Ny = 10;

	// Time step and length
	Real tau= 2;
	Real T	= 10;

	// Coefficients
	Real a = 1;
	Real c = 1;

	void init() {
		const auto elems = Nx * Ny * 2;
		P = Vector(elems, 0);
		P_prev = Vector(elems, 0);
		const auto edges = (3 * Nx + 1) * Ny + Nx;
		TP = Vector(edges, 0);

		m = std::make_shared<Matrix>();
		m->setDimensions(edges, edges);
		right = Vector(edges, 0);

		t = 0;
	}
	
	bool step() {
		// Prepare some constants
		const Real dx = X / Nx;
		const Real dy = Y / Ny;

		const Real element_area = dx * dy / 2;

		const Real lambda = c * element_area / tau;
		// l_ij + l_jk + l_ik = dx^2 + dy^2 + (dx^2 + dy^2)
		// element_area = dx * dy / 2
		// l = (l_ij + l_jk + l_ik) / 48 / element_area = (dx^2 + dy^2) / 24 / (dx * dy / 2)
		const Real l = (dx / dy + dy / dx) / 48;
		const Real alpha = 3.0 / l;
		const Real beta = lambda + a * alpha;

		const auto elems = get_elems(Nx, Ny);
		const auto edges = get_edges(Nx, Ny);

#ifdef HAVE_CUDA
		// Create local variables to replace some of members since host member variables
		// can't be accessed from a CUDA kernel
		const auto Nx = this->Nx;
		const auto Ny = this->Ny;

		const auto X = this->X;
		const auto y = this->Y;

		const auto a = this->a;
		const auto c = this->c;

		const auto tau = this->tau;
#endif

		// Back up the solution
		P_prev = P;

		// Clear containers
		m->setDimensions(edges, edges); // This resets the values for the matrix
		IVector caps(edges);
		auto cv = caps.getView();
		auto get_capacity = [=] __cuda_callable__ (int edge) mutable {
			Index e1, e2;
			Edge type;
			switch (get_neighbour_elements(edge, e1, e2, type, Nx, Ny)) {
				case 2:
					cv[edge] = 5;
					break;
				case 1:
					Index ix, iy, u;
					get_element_params(e1, ix, iy, u, Nx, Ny);
					if (((ix == 0) || (ix == Nx - 1)) && (type == EDGE_J))
						cv[edge] = 1;
					else
						cv[edge] = 3;
					break;
			}
		};
		TNL::Algorithms::ParallelFor<Device>::exec(0, edges, get_capacity);
		m->setRowCapacities(caps);
		right = Vector(edges, 0);
		P = Vector(elems, 0);

		auto m_view = m->getView();

		auto fill_m_part = [=] __cuda_callable__ (int edge, int element, const Edge& type) mutable {
			Index ix, iy, u;
			get_element_params(element, ix, iy, u, Nx, Ny);
			
			const auto r = edge_vector(type, u, dx, dy);

			for (int eei = 0; eei < 3; ++eei) {
				const auto ree = edge_vector((Edge)eei, u, dx, dy);
				const Real B_inv = (r, ree) / element_area + 1.0 / l / 3;
				const Real delta = a * (B_inv - 1.0 / l / 3);
				const auto eei_index = edge_index(ix, iy, u, (Edge)eei, Nx, Ny);
				m_view.addElement(edge, eei_index, delta + (eei_index == edge) * c * element_area / 3.0 / tau);
			}
		};
		auto fill_m = [=] __cuda_callable__ (int edge) mutable {
			Index e1, e2;
			Edge type;
			switch (get_neighbour_elements(edge, e1, e2, type, Nx, Ny)) {
				case 2:
					fill_m_part(edge, e1, type);
					fill_m_part(edge, e2, type);
					break;
				case 1:
					Index ix, iy, u;
					get_element_params(e1, ix, iy, u, Nx, Ny);
					if (((iy == 0) || (iy == Ny - 1)) && (type == EDGE_K))
						fill_m_part(edge, e1, type);
					break;
			}
		};
		TNL::Algorithms::ParallelFor<Device>::exec(0, edges, fill_m);

		// m is ready, construct the right-hand vector
		auto right_view = right.getView();
		auto TP_view = TP.getView();
		auto fill_right = [=] __cuda_callable__ (int edge) mutable {
			Index e1, e2;
			Edge type;
			const Real count = get_neighbour_elements(edge, e1, e2, type, Nx, Ny);
			right_view[edge] = c * count * element_area * TP_view[edge] / 3 / tau;
		};
		TNL::Algorithms::ParallelFor<Device>::exec(0, edges, fill_right);

		// Right-hand side of domain (x = X): P = 0 -> TP = 0
		auto reset_x_0 = [=] __cuda_callable__ (int col, int iy) mutable {
			// Only 'upper' triangles have the needed side
			const auto edge = edge_index(Nx - 1, iy, 1, EDGE_J, Nx, Ny);
			right_view[edge] = 0;
			if (col == edge) m_view.setElement(edge, col, 1); // 1 * TP = 0
		};
		TNL::Algorithms::ParallelFor2D<Device>::exec(0, 0, edges, Ny, reset_x_0);

		// Left-hand side of domain (x = 0): P = 1 -> TP = 1
		auto reset_x_X = [=] __cuda_callable__ (int col, int iy) mutable {
			// Only 'lower' triangles have the needed side
			const auto edge = edge_index(0, iy, 0, EDGE_J, Nx, Ny);
			right_view[edge] = 1;
			if (col == edge) m_view.setElement(edge, col, 1); // 1 * TP = 1
		};
		TNL::Algorithms::ParallelFor2D<Device>::exec(0, 0, edges, Ny, reset_x_X);

		// Now m should be ready, solve the system
		auto step_solver = TNL::Solvers::getLinearSolver<Matrix>("sor");
		auto step_precond = TNL::Solvers::getPreconditioner<Matrix>("diagonal");
		step_precond->update(m);
		step_solver->setMatrix(m);
		step_solver->setPreconditioner(step_precond);
		step_solver->solve(right, TP);

		// Finally, get P
		auto P_view = P.getView();
		auto P_prev_view = P_prev.getView();
		auto set_P = [=] __cuda_callable__ (int ix, int iy, int u) mutable {
			const auto element = element_index(ix, iy, u, Nx);

			P_view[element] = lambda * P_prev_view[element] / beta;
			for (int e = 0; e < 3; ++e)
				P_view[element] += a * TP_view[edge_index(ix, iy, u, (Edge)e, Nx, Ny)] / beta / l;
		};
		TNL::Algorithms::ParallelFor3D<Device>::exec(0, 0, 0, Nx, Ny, 2, set_P);

		t += tau;
		return t < T;
	}

	bool save(const std::string& filename, const char& separator = ',') {
		std::filesystem::create_directories(saved_dir);
		std::ofstream output(saved_dir + "/" + filename);
		if (!output.is_open()) {
			std::cerr << "Couldn't open output file " << filename << std::endl;
			return false;
		}

		const Real dx = X / Nx;
		const Real dy = Y / Ny;

#ifdef HAVE_CUDA
		// Copy solution to host
		const auto elems = get_elems(Nx, Ny);
		TNL::Containers::Vector<Real, TNL::Devices::Host, Index> P(elems);
		P = this->P;
#endif

		auto get_x = [&] (const Index& element) {
			Index ix, iy, u;
			get_element_params(element, ix, iy, u, Nx, Ny);
			return (1 + u) * dx / 3 + dx * ix;
		};
		auto get_y = [&] (const Index& element) {
			Index ix, iy, u;
			get_element_params(element, ix, iy, u, Nx, Ny);
			return (2 - u) * dy / 3 + dy * iy;
		};

		for (Index element = 0; element < get_elems(Nx, Ny); ++element)
			output << get_x(element) << separator << get_y(element) << separator << P[element] << std::endl;

		output.close();
		return true;
	}

	bool save_midpoints(const std::string& filename, const char& separator = ',') {
		std::filesystem::create_directories(saved_dir);
		std::ofstream output(saved_dir + "/" + filename);
		if (!output.is_open()) {
			std::cerr << "Couldn't open output file " << filename << std::endl;
			return false;
		}

		const Real dx = X / Nx;
		const Real dy = Y / Ny;

#ifdef HAVE_CUDA
		// Copy solution to host
		const auto edges = get_edges(Nx, Ny);
		TNL::Containers::Vector<Real, TNL::Devices::Host, Index> TP(edges);
		TP = this->TP;
#endif

		for (Index edge = 0; edge < get_edges(Nx, Ny); ++edge) {
			auto p = edge_midpoint(edge, dx, dy, Nx, Ny);
			output << p[0] << separator << p[1] << separator << TP[edge] << std::endl;
		}

		output.close();
		return true;
	}
};
