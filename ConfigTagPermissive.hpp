#pragma once

template <typename CellTopology> struct ConfigTagPermissive {};

namespace TNL::Meshes::BuildConfigTags {
	template <typename CellTopology>
	struct MeshCellTopologyTag<ConfigTagPermissive<CellTopology>, CellTopology> { enum { enabled = true }; };
};
