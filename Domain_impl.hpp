#pragma once

// STL headers
#include <iostream>

// TNL headers
#include <TNL/Algorithms/ParallelFor.h>
#include <TNL/Meshes/MeshBuilder.h>
#include <TNL/Meshes/Writers/VTKWriter.h>

using namespace std;

// Layer getters
DOMAIN_TEMPLATE
typename Domain DOMAIN_TARGS::RVector& Domain DOMAIN_TARGS::getRealLayer(const Layer& layer, const size_t& index) {
	switch (layer) {
		case Cell:
			return layers.cells.real.at(index);
		case Edge:
			return layers.edges.real.at(index);
	}
}

DOMAIN_TEMPLATE
typename Domain DOMAIN_TARGS::IVector& Domain DOMAIN_TARGS::getIndexLayer(const Layer& layer, const size_t& index) {
	switch (layer) {
		case Cell:
			return layers.cells.index.at(index);
		case Edge:
			return layers.edges.index.at(index);
	}
}

DOMAIN_TEMPLATE
size_t Domain DOMAIN_TARGS::addRealLayer(const Layer& layer) {
	switch (layer) {
		case Cell:
			{
			const auto cells = mesh.template getEntitiesCount<MeshType::getMeshDimension()>();
			layers.cells.real.push_back(RVector(cells));
			return layers.cells.real.size() - 1;
			}
		case Edge:
			{
			const auto edges = mesh.template getEntitiesCount<MeshType::getMeshDimension() - 1>();
			layers.edges.real.push_back(RVector(edges));
			return layers.edges.real.size() - 1;
			}
		default:
			return -1;
	}
}

// Generators
DOMAIN_TEMPLATE
inline bool Domain DOMAIN_TARGS::generateRectangularDomain(Domain& domain, const Index& Nx, const Index& Ny, const Real& dx, const Real& dy) {
	if (!is_same<CellTopology, TNL::Meshes::Topologies::Triangle>()) {
		cerr << "generateRectangularDomain(...) is only implemented for Triangle (2D) topology!" << endl;
		return false;
	}

	using Builder = TNL::Meshes::MeshBuilder<MeshType>;
	Builder builder;
	const auto elems = Nx * Ny * 2;
	const auto points = (Nx + 1) * (Ny + 1);
	builder.setEntitiesCount(points, elems);

	using Point = typename MeshType::MeshTraitsType::PointType;
	const auto point_id = [&] (const Index& ix, const Index& iy) -> Index {
		return ix + (Nx + 1) * iy;
	};
	const auto fill_points = [&] (const Index& ix, const Index& iy) {
		builder.setPoint(point_id(ix, iy), Point(ix * dx, iy * dy));
	};
	TNL::Algorithms::ParallelFor2D<TNL::Devices::Host>::exec(0, 0, Nx + 1, Ny + 1, fill_points);

	const auto fill_elems = [&] (const Index& ix, const Index& iy, const Index& u) {
		const auto cell = 2 * (ix + Nx * iy) + u;
		auto seed = builder.getCellSeed(cell);

		switch (u) {
			case 1:
				seed.setCornerId(0, point_id(ix, iy));
				seed.setCornerId(1, point_id(ix, iy + 1));
				seed.setCornerId(2, point_id(ix + 1, iy));
				break;
			case 0:
				seed.setCornerId(0, point_id(ix + 1, iy + 1));
				seed.setCornerId(1, point_id(ix + 1, iy));
				seed.setCornerId(2, point_id(ix, iy + 1));
				break;
			default:
				cerr << "Unrealistic 'u' value encountered: " << u << endl;
		}
	};
	TNL::Algorithms::ParallelFor3D<TNL::Devices::Host>::exec(0, 0, 0, Nx, Ny, 2, fill_elems);

	builder.build(domain.mesh);

	domain.mesh.print(cout);
	//cout << "Mesh built!" << endl << domain.mesh << endl;

	return true;
}

DOMAIN_TEMPLATE
inline bool Domain DOMAIN_TARGS::generateCuboidDomain(Domain& domain) {
	if (!is_same<CellTopology, TNL::Meshes::Topologies::Tetrahedron>()) {
		cerr << "generateCuboidDomain(...) is only implemented for Tetrahedron (3D) topology!" << endl;
		return false;
	}

	cerr << "TODO: implement" << endl; // TODO
	return false;

	return true;
}

DOMAIN_TEMPLATE
inline bool Domain DOMAIN_TARGS::write(const std::string& filename) {
	ofstream file(filename);
	if (!file.is_open()) {
		cerr << "Could not open file for writing: " << filename << endl;
		return false;
	}

	using Writer = TNL::Meshes::Writers::VTKWriter<MeshType>;
	Writer writer(file);
	writer.template writeEntities<MeshType::getMeshDimension()>(mesh);

	// Write cell index layer
	IVector layer(mesh.template getEntitiesCount<MeshType::getMeshDimension()>());
	layer.forAllElements([] (int i, Index& v) { v = i; });
	writer.writeCellData(layer, "cell_index");

	// Write layers
	for (int i = 0; i < layers.cells.real.size(); ++i)
		writer.writeCellData(layers.cells.real.at(i), "cell_real_layer_" + to_string(i));
	for (int i = 0; i < layers.cells.index.size(); ++i)
		writer.writeCellData(layers.cells.index.at(i), "cell_index_layer_" + to_string(i));
	for (int i = 0; i < layers.edges.real.size(); ++i)
		writer.writeCellData(layers.edges.real.at(i), "edge_real_layer_" + to_string(i));
	for (int i = 0; i < layers.edges.index.size(); ++i)
		writer.writeCellData(layers.edges.index.at(i), "edge_index_layer_" + to_string(i));

	return true;
}
