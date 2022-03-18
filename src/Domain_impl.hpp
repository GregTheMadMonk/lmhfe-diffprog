#pragma once

// STL headers
#include <fstream>

// TNL headers
#include <TNL/Algorithms/ParallelFor.h>
#include <TNL/Meshes/MeshBuilder.h>
#include <TNL/Meshes/TypeResolver/resolveMeshType.h>

// Local headers
#include "ConfigTagPermissive.hpp"

// Update layer sizes
DOMAIN_TEMPLATE
inline void Domain DOMAIN_TARGS::updateLayerSizes() {
	layers.cell.setSize(getEntitiesCount());
	layers.edge.setSize(getEntitiesCount<dimensions() - 1>());
}

// Mesh clearer
DOMAIN_TEMPLATE
inline void Domain DOMAIN_TARGS::clear() {
	if (isClean()) return; // Nothing to clear

	// Reset the mesh
	mesh.reset();

	layers.cell.clear();
	layers.edge.clear();
}

// Generators
DOMAIN_TEMPLATE
bool Domain DOMAIN_TARGS::generateRectangularDomain(const Index& Nx, const Index& Ny, const Real& dx, const Real& dy) {
	if (mesh != std::nullopt) {
		std::cerr << "Mesh data is not empty, was clear() called?" << std::endl;
		return false;
	}
	if (!std::is_same<CellTopology, TNL::Meshes::Topologies::Triangle>()) {
		std::cerr << "generateRectangularDomain(...) is only implemented for Triangle (2D) topology!" << std::endl;
		return false;
	}

	mesh = MeshType();

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

	builder.build(mesh.value());

	updateLayerSizes();

	return true;
}

DOMAIN_TEMPLATE
bool Domain DOMAIN_TARGS::generateCuboidDomain(const Index& Nx, const Index& Ny, const Index& Nz, const Real& dx, const Real& dy) {
	if (mesh != std::nullopt) {
		std::cerr << "Mesh data is not empty, was clear() called?" << std::endl;
		return false;
	}
	if (!std::is_same<CellTopology, TNL::Meshes::Topologies::Tetrahedron>()) {
		std::cerr << "generateCuboidDomain(...) is only implemented for Tetrahedron (3D) topology!" << std::endl;
		return false;
	}

	mesh = MeshType();

	std::cerr << "TODO: implement" << std::endl; // TODO
	return false;

	updateLayerSizes();

	return true;
}

DOMAIN_TEMPLATE
bool Domain DOMAIN_TARGS::loadFromMesh(const std::filesystem::path& filename) {
	if (mesh != std::nullopt) {
		std::cerr << "Mesh data is not empty, was clear() called?" << std::endl;
		return false;
	}

	if (!std::filesystem::exists(filename)) {
		std::cerr << "Mesh file doesn't exist:" << filename << std::endl;
		return false;
	}

	auto loader = [&] (auto& reader, auto&& loadedMesh) {
		if (typeid(loadedMesh) != typeid(MeshType)) {
			std::cerr << "Read mesh type mismatch ("
				<< typeid(loadedMesh).name() << " != " << typeid(MeshType).name()
				<< ")!" << std::endl;
			return false;
		}
		mesh = *(MeshType*)&loadedMesh;// This is evil and hacky but it gets the work done
						// And since we check for type mismatch it should
						// not crash
		updateLayerSizes();

		// TODO: find a way to get data layers too

		return true;
	};

	using ConfigTag = ConfigTagPermissive<CellTopology>;
	return TNL::Meshes::resolveAndLoadMesh<ConfigTag, TNL::Devices::Host>(loader, filename, "auto");
}

DOMAIN_TEMPLATE
inline bool Domain DOMAIN_TARGS::write(const std::filesystem::path& filename) {
	if (mesh == std::nullopt) {
		std::cerr << "Mesh is empty! Could not save." << std::endl;
		return false;
	}

	std::ofstream file(filename);
	if (!file.is_open()) {
		std::cerr << "Could not open file for writing: " << filename << std::endl;
		return false;
	}

	MeshWriter writer(file);
	writer.template writeEntities<MeshType::getMeshDimension()>(mesh.value());

	// Write cell index layer
	TNL::Containers::Vector<Index, Device> layer(mesh.value().template getEntitiesCount<MeshType::getMeshDimension()>());
	layer.forAllElements([] (int i, Index& v) { v = i; });
	writer.writeCellData(layer, "cell_index");

	// Write layers
	for (int i = 0; i < layers.cell.count(); ++i)
		layers.cell.getBasePtr(i)->writeCellData(writer, "cell_layer_" + std::to_string(i));
	for (int i = 0; i < layers.edge.count(); ++i)
		layers.edge.getBasePtr(i)->writeDataArray(writer, "edge_layer_" + std::to_string(i));

	return true;
}
