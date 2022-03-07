#pragma once

// STL headers
#include <iostream>
#include <fstream>

// TNL headers
#include <TNL/Algorithms/ParallelFor.h>
#include <TNL/Meshes/MeshBuilder.h>
#include <TNL/Meshes/TypeResolver/resolveMeshType.h>
#include <TNL/Meshes/Writers/VTKWriter.h>

// Local headers
#include "ConfigTagPermissive.hpp"

// Mesh clearer
DOMAIN_TEMPLATE
inline void Domain DOMAIN_TARGS::clear() {
	if (isClean()) return; // Nothing to clear

	// Reset the mesh
	mesh.reset();
	mesh = nullptr;

	clearLayers();
}

DOMAIN_TEMPLATE
inline void Domain DOMAIN_TARGS::clearLayers() {
	// Reset the layers
	layers.cells.real.clear();
	layers.cells.index.clear();
	layers.edges.real.clear();
	layers.edges.index.clear();
}

// Layer getters
DOMAIN_TEMPLATE
inline typename Domain DOMAIN_TARGS::RVector& Domain DOMAIN_TARGS::getRealLayer(const Layer& layer, const size_t& index) {
	switch (layer) {
		case Cell:
			return layers.cells.real.at(index);
		case Edge:
			return layers.edges.real.at(index);
	}
	return *(RVector*)nullptr;	// Is never reached, WILL CRASH if somehow reached
					// and is only here to remove that annoyting warning
}

DOMAIN_TEMPLATE
inline typename Domain DOMAIN_TARGS::IVector& Domain DOMAIN_TARGS::getIndexLayer(const Layer& layer, const size_t& index) {
	switch (layer) {
		case Cell:
			return layers.cells.index.at(index);
		case Edge:
			return layers.edges.index.at(index);
	}
	return *(IVector*)nullptr;	// Is never reached, WILL CRASH if somehow reached
					// and is only here to remove that annoyting warning
}

DOMAIN_TEMPLATE
inline size_t Domain DOMAIN_TARGS::addRealLayer(const Layer& layer) {
	switch (layer) {
		case Cell: {
			const auto cells = mesh->template getEntitiesCount<MeshType::getMeshDimension()>();
			layers.cells.real.push_back(RVector(cells));
			return layers.cells.real.size() - 1;
			}
		case Edge: {
			const auto edges = mesh->template getEntitiesCount<MeshType::getMeshDimension() - 1>();
			layers.edges.real.push_back(RVector(edges));
			return layers.edges.real.size() - 1;
			}
	}
	return -1;
}

DOMAIN_TEMPLATE
inline size_t Domain DOMAIN_TARGS::addIndexLayer(const Layer& layer) {
	switch (layer) {
		case Cell: {
			const auto cells = mesh->template getEntitiesCount<MeshType::getMeshDimension()>();
			layers.cells.index.push_back(IVector(cells));
			return layers.cells.index.size() - 1;
			}
		case Edge: {
			const auto edges = mesh->template getEntitiesCount<MeshType::getMeshDimension() - 1>();
			layers.edges.index.push_back(IVector(edges));
			return layers.edges.index.size() - 1;
			}
	}
	return -1;
}

// Generators
DOMAIN_TEMPLATE
bool Domain DOMAIN_TARGS::generateRectangularDomain(const Index& Nx, const Index& Ny, const Real& dx, const Real& dy) {
	if (mesh != nullptr) {
		std::cerr << "Mesh data is not empty, was clear() called?" << std::endl;
		return false;
	}
	if (!std::is_same<CellTopology, TNL::Meshes::Topologies::Triangle>()) {
		std::cerr << "generateRectangularDomain(...) is only implemented for Triangle (2D) topology!" << std::endl;
		return false;
	}

	mesh = std::make_unique<MeshType>();

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
				std::cerr << "Unrealistic 'u' value encountered: " << u << std::endl;
		}
	};
	TNL::Algorithms::ParallelFor3D<TNL::Devices::Host>::exec(0, 0, 0, Nx, Ny, 2, fill_elems);

	builder.build(*mesh);

	return true;
}

DOMAIN_TEMPLATE
bool Domain DOMAIN_TARGS::generateCuboidDomain(const Index& Nx, const Index& Ny, const Index& Nz, const Real& dx, const Real& dy) {
	if (mesh != nullptr) {
		std::cerr << "Mesh data is not empty, was clear() called?" << std::endl;
		return false;
	}
	if (!std::is_same<CellTopology, TNL::Meshes::Topologies::Tetrahedron>()) {
		std::cerr << "generateCuboidDomain(...) is only implemented for Tetrahedron (3D) topology!" << std::endl;
		return false;
	}

	mesh = std::make_unique<MeshType>();

	std::cerr << "TODO: implement" << std::endl; // TODO
	return false;

	return true;
}

DOMAIN_TEMPLATE
bool Domain DOMAIN_TARGS::loadFromMesh(const std::string& filename) {
	if (mesh != nullptr) {
		std::cerr << "Mesh data is not empty, was clear() called?" << std::endl;
		return false;
	}

	mesh = std::make_unique<MeshType>();

	auto loader = [&] (auto& reader, auto&& loadedMesh) {
		if (typeid(loadedMesh) != typeid(MeshType)) {
			std::cerr << "Read mesh type mismatch ("
				<< typeid(loadedMesh).name() << " != " << typeid(MeshType).name()
				<< ")!" << std::endl;
			return false;
		}
		*mesh = *(MeshType*)&loadedMesh;// This is evil and hacky but it gets the work done
						// And since we check for type mismatch it should
						// not crash
		// TODO: find a way to get data layers too
		return true;
	};

	using ConfigTag = ConfigTagPermissive<CellTopology>;
	return TNL::Meshes::resolveAndLoadMesh<ConfigTag, TNL::Devices::Host>(loader, filename, "auto");
}

DOMAIN_TEMPLATE
inline bool Domain DOMAIN_TARGS::write(const std::string& filename) {
	std::ofstream file(filename);
	if (!file.is_open()) {
		std::cerr << "Could not open file for writing: " << filename << std::endl;
		return false;
	}

	using Writer = TNL::Meshes::Writers::VTKWriter<MeshType>;
	Writer writer(file);
	writer.template writeEntities<MeshType::getMeshDimension()>(*mesh);

	// Write cell index layer
	IVector layer(mesh->template getEntitiesCount<MeshType::getMeshDimension()>());
	layer.forAllElements([] (int i, Index& v) { v = i; });
	writer.writeCellData(layer, "cell_index");

	// Write layers
	for (int i = 0; i < layers.cells.real.size(); ++i)
		writer.writeCellData(layers.cells.real.at(i), "cell_real_layer_" + std::to_string(i));
	for (int i = 0; i < layers.cells.index.size(); ++i)
		writer.writeCellData(layers.cells.index.at(i), "cell_index_layer_" + std::to_string(i));
	for (int i = 0; i < layers.edges.real.size(); ++i)
		writer.writeCellData(layers.edges.real.at(i), "edge_real_layer_" + std::to_string(i));
	for (int i = 0; i < layers.edges.index.size(); ++i)
		writer.writeCellData(layers.edges.index.at(i), "edge_index_layer_" + std::to_string(i));

	return true;
}
