#pragma once

// STL headers
#include <iostream>
#include <string>
#include <vector>

// TNL headers
#include <TNL/Meshes/DefaultConfig.h>
#include <TNL/Meshes/Mesh.h>
#include <TNL/Meshes/TypeResolver/BuildConfigTags.h>
#include <TNL/Meshes/Writers/VTKWriter.h>

// Local headers
#include "LayerManager.hpp"

// Me lazy no want write many word
#define DOMAIN_TEMPLATE	template <typename CellTopology, typename Device, typename Real, typename Index>
#define DOMAIN_TARGS	<CellTopology, Device, Real, Index>

// Domain class declaration
// Domain contains the mesh and data layers for it
template <typename CellTopology, typename Device = TNL::Devices::Host, typename Real = float, typename Index = int>
class Domain {
	// Alias types (declare public)
	public:
	// Mesh
	// NOTE: resolveAndLoad mesh returns a mesh with 'long int' for GlobalIndex for some reason
	// instead of default 'int', so we have to account for that here
	using MeshConfig	= TNL::Meshes::DefaultConfig<CellTopology, CellTopology::dimension, Real, long int>;
	using MeshType		= TNL::Meshes::Mesh<MeshConfig>;// Meshes are only implemented for Host (?)
	using Mesh_p		= std::unique_ptr<MeshType>;	// Mesh is only accessible from inside a class,
						// and isn't passed by pointer, so unique_ptr should be sufficient
	using MeshWriter = TNL::Meshes::Writers::VTKWriter<MeshType>;

	protected:
	// Domain mesh
	Mesh_p mesh = nullptr;

	void updateLayerSizes();

	public:
	struct {
		LayerManager<Index, Device, MeshWriter> cell;
		LayerManager<Index, Device, MeshWriter> edge;
	} layers;

	static constexpr int dimensions() { return MeshType::getMeshDimension(); }

	// Constructors
	Domain()		= default;
	Domain(const Domain& d2)= default;
	Domain(Domain&& d2)	= default;
	Domain& operator=(const Domain& d2) {
		mesh = std::make_unique<MeshType>(*d2.mesh);
		layers = d2.layers;
		return *this;
	}
	Domain& operator=(Domain&& d2) {
		mesh = std::move(d2.mesh);
		layers = std::move(d2.layers);
		return *this;
	}

	// Clear mesh data
	void clear();

	bool isClean() const { return mesh == nullptr; }

	// Mesh iterating
	// These repeat 'Mesh' methods (basically call them)
	// Pretend this is 'readable'
	template <int dimension = dimensions(), typename Device2 = Device, typename Func>
	void forAll(Func f)	{ mesh->template forAll<dimension, Device2>(f);		}
	template <int dimension = dimensions(), typename Device2 = Device, typename Func>
	void forBoundary(Func f){ mesh->template forBoundary<dimension, Device2>(f);	}
	template <int dimension = dimensions(), typename Device2 = Device, typename Func>
	void forGhost(Func f)	{ mesh->template forGhost<dimension, Device2>(f);	}
	template <int dimension = dimensions(), typename Device2 = Device, typename Func>
	void forInterior(Func f){ mesh->template forInterior<dimension, Device2>(f);	}
	template <int dimension = dimensions(), typename Device2 = Device, typename Func>
	void forLocal(Func f)	{ mesh->template forLocal<dimension, Device2>(f);	}

	// Mesh parameter getters
	template <int dimension = dimensions()>
	auto getEntitiesCount() const { return mesh->template getEntitiesCount<dimension>(); }
	template <int eDimension = dimensions(), int sDimension = eDimension - 1>
	auto getSubentitiesCount(typename MeshType::GlobalIndexType index) const {
		return mesh->template getSubentitiesCount<eDimension, sDimension>(index);
	}
	template <int eDimension = dimensions(), int sDimension = eDimension - 1>
	auto getSubentityIndex(typename MeshType::GlobalIndexType eIndex,
				typename MeshType::GlobalIndexType sIndex) {
		return mesh->template getSubentityIndex<eDimension, sDimension>(eIndex, sIndex);
	}
	template <int eDimension = dimensions() - 1, int sDimension = eDimension + 1>
	auto getSuperentitiesCount(typename MeshType::GlobalIndexType index) {
		return mesh->template getSuperentitiesCount<eDimension, sDimension>(index);
	}
	template <int eDimension = dimensions() - 1, int sDimension = eDimension + 1>
	auto getSuperentityIndex(typename MeshType::GlobalIndexType eIndex,
				typename MeshType::LocalIndexType sIndex) {
		return mesh->template getSuperentityIndex<eDimension, sDimension>(eIndex, sIndex);
	}

	// Some generator functions
	bool generateRectangularDomain(const Index& Nx, const Index& Ny, const Real& dx, const Real& dy);
	bool generateCuboidDomain(const Index& Nx, const Index& Ny, const Index& Nz, const Real& dx, const Real& dy);
	bool loadFromMesh(const std::string& filename);

	// Write mesh data to a file
	bool write(const std::string& filename);

	// Output domain
	friend std::ostream& operator<<(std::ostream& stream, const Domain& d) {
		stream << "Domain: " << std::endl;
		stream << "\tMesh cells: " << d.getEntitiesCount() << std::endl;
		stream << "\tCell layers: " << d.layers.cell.count() << std::endl;
		stream << "\tEdge layers: " << d.layers.edge.count() << std::endl;
		return stream;
	}
}; // --> class Domain

#include "Domain_impl.hpp"
