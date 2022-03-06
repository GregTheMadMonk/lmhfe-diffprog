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
	// Data vectors
	using RVector		= TNL::Containers::Vector<Real, Device, Index>;
	using IVector		= TNL::Containers::Vector<Index, Device, Index>;
	// Mesh
	// NOTE: resolveAndLoad mesh returns a mesh with 'long int' for GlobalIndex for some reason
	// instead of default 'int', so we have to account for that here
	using MeshConfig	= TNL::Meshes::DefaultConfig<CellTopology, CellTopology::dimension, Real, long int>;
	using MeshType		= TNL::Meshes::Mesh<MeshConfig>; // Meshes are only implemented for Host (?)

	// Domain data layers (TNL meshes don't contain any data by themselves)
	struct {
		struct {
			std::vector<RVector> real;
			std::vector<IVector> index;
		} cells, edges;
	} layers;

	// Domain mesh
	MeshType mesh;

	public:
	enum Layer { Cell, Edge };

	static constexpr int dimensions() { return MeshType::getMeshDimension(); }

	// Constructors
	Domain()		= default;
	Domain(Domain&& domain)	= default;

	// Mesh iterating
	// These repeat 'Mesh' mathods (basically call them)
	// Pretend this is 'readable'
	template <int dimension = dimensions(), typename Device2 = Device, typename Func>
	void forAll(Func f)	{ mesh.template forAll<dimension, Device2>(f);		}
	template <int dimension = dimensions(), typename Device2 = Device, typename Func>
	void forBoundary(Func f){ mesh.template forBoundary<dimension, Device2>(f);	}
	template <int dimension = dimensions(), typename Device2 = Device, typename Func>
	void forGhost(Func f)	{ mesh.template forGhost<dimension, Device2>(f);	}
	template <int dimension = dimensions(), typename Device2 = Device, typename Func>
	void forInterior(Func f){ mesh.template forInterior<dimension, Device2>(f);	}
	template <int dimension = dimensions(), typename Device2 = Device, typename Func>
	void forLocal(Func f)	{ mesh.template forLocal<dimension, Device2>(f);	}

	// Layer management
	RVector& getRealLayer(const Layer& layer, const std::size_t& index);
	IVector& getIndexLayer(const Layer& layer, const std::size_t& index);
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
