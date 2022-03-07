#pragma once

// STL headers
#include <string>
#include <vector>

// TNL headers
#include <TNL/Meshes/DefaultConfig.h>
#include <TNL/Meshes/Mesh.h>
#include <TNL/Meshes/TypeResolver/BuildConfigTags.h>

// Me lazy no want write many word
#define DOMAIN_TEMPLATE	template <typename CellTopology, typename Device, typename Real, typename Index>
#define DOMAIN_TARGS	<CellTopology, Device, Real, Index>

// Domain class declaration
// Domain contains the mesh and data layers for it
template <typename CellTopology, typename Device = TNL::Devices::Host, typename Real = float, typename Index = int>
class Domain {
	// Alias types
	// Data vectors
	using RVector		= TNL::Containers::Vector<Real, Device, Index>;
	using IVector		= TNL::Containers::Vector<Index, Device, Index>;
	// Mesh
	// NOTE: resolveAndLoad mesh returns a mesh with 'long int' for GlobalIndex for some reason
	// instead of default 'int', so we have to account for that here
	using MeshConfig	= TNL::Meshes::DefaultConfig<CellTopology, CellTopology::dimension, Real, long int>;
	using MeshType		= TNL::Meshes::Mesh<MeshConfig>;// Meshes are only implemented for Host (?)
	using Mesh_p		= std::unique_ptr<MeshType>;	// Mesh is only accessible from inside a class,
						// and isn't passed by pointer, so unique_ptr should be sufficient

	// Domain data layers (TNL meshes don't contain any data by themselves)
	struct {
		struct {
			std::vector<RVector> real;
			std::vector<IVector> index;
		} cells, edges;
	} layers;

	// Domain mesh
	Mesh_p mesh = nullptr;

	public:
	enum Layer { Cell, Edge };

	static constexpr int dimensions() { return MeshType::getMeshDimension(); }

	// Constructors
	Domain()		= default;
	Domain(const Domain& d2)= default;
	Domain(Domain&& d2)	= default;
	Domain& operator=(const Domain& d2)	= default;
	Domain& operator=(Domain&& d2)		= default;

	// Clear mesh data
	void clear();
	void clearLayers();

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

	// Layer management
	RVector& getRealLayer(const Layer& layer, const std::size_t& index);
	const RVector& getRealLayer(const Layer& layer, const std::size_t& index) const {
		return static_cast<Domain*>(this)->getRealLayer(layer, index);
	}
	IVector& getIndexLayer(const Layer& layer, const std::size_t& index);
	const IVector& getIndexLayer(const Layer& layer, const std::size_t& index) const {
		return static_cast<Domain*>(this)->getIndexLayer(layer, index);
	}
	std::size_t addRealLayer(const Layer& layer);
	std::size_t addIndexLayer(const Layer& layer);

	// Some generator functions
	bool generateRectangularDomain(const Index& Nx, const Index& Ny, const Real& dx, const Real& dy);
	bool generateCuboidDomain(const Index& Nx, const Index& Ny, const Index& Nz, const Real& dx, const Real& dy);
	bool loadFromMesh(const std::string& filename);

	// Write mesh data to a file
	bool write(const std::string& filename);
}; // --> class Domain

#include "Domain_impl.hpp"
