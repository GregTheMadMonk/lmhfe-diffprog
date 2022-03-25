#pragma once

// STL headers
#include <cassert>
#include <memory>
#include <variant>

// TNL headers
#include <TNL/Containers/Vector.h>

#define LVFOR(VARIANT, WHAT) switch (VARIANT.index()) {\
		case Layer::VType::I8:\
			WHAT(std::get<Vector<std::int8_t>>(VARIANT));\
			break;\
		case Layer::VType::UI8:\
			WHAT(std::get<Vector<std::uint8_t>>(VARIANT));\
			break;\
		case Layer::VType::I16:\
			WHAT(std::get<Vector<std::int16_t>>(VARIANT));\
			break;\
		case Layer::VType::UI16:\
			WHAT(std::get<Vector<std::uint16_t>>(VARIANT));\
			break;\
		case Layer::VType::I32:\
			WHAT(std::get<Vector<std::int32_t>>(VARIANT));\
			break;\
		case Layer::VType::UI32:\
			WHAT(std::get<Vector<std::uint32_t>>(VARIANT));\
			break;\
		case Layer::VType::I64:\
			WHAT(std::get<Vector<std::int64_t>>(VARIANT));\
			break;\
		case Layer::VType::UI64:\
			WHAT(std::get<Vector<std::uint64_t>>(VARIANT));\
			break;\
		case Layer::VType::FLOAT:\
			WHAT(std::get<Vector<float>>(VARIANT));\
			break;\
		case Layer::VType::DOUBLE:\
			WHAT(std::get<Vector<double>>(VARIANT));\
			break;\
	}

template <typename Device = TNL::Devices::Host, typename Index = int>
struct Layer {
	// TNL::Meshes::Readers::MeshReader has std::vector as mesh data container.
	// We want to be able to store data on different devices, hence we also
	// have to hard-code our own variant of TNL::Containers::Vector's
	public:
	template <typename DataType> using Vector = TNL::Containers::Vector<DataType, Device, Index>;
	using VariantVector = std::variant< Vector< std::int8_t >,
						Vector< std::uint8_t >,
						Vector< std::int16_t >,
						Vector< std::uint16_t >,
						Vector< std::int32_t >,
						Vector< std::uint32_t >,
						Vector< std::int64_t >,
						Vector< std::uint64_t >,
						Vector< float >,
						Vector< double > >;

	protected:
	VariantVector data;
	Index size;

	public:
	enum VType : std::size_t {
		// Corresponds to TNL::Meshes::Readers::MeshReader::VariantVector too
		I8	= 0,
		UI8	= 1,
		I16	= 2,
		UI16	= 3,
		I32	= 4,
		UI32	= 5,
		I64	= 6,
		UI64	= 7,
		FLOAT	= 8,
		DOUBLE	= 9
	};

	template <typename DataType>
	void init(const Index& newSize, const DataType& value = DataType()) {
		size = newSize;
		data = Vector<DataType>(size, value);
	}
	void setSize(const Index& newSize) {
		size = newSize;
		const auto setSize_f = [&] (auto& v) {
			v.setSize(size);
		};
		LVFOR(data, setSize_f);
	}
	const Index& getSize() const { return size; }

	template <typename DataType>
	void setFrom(const std::vector<DataType>& from) {
		assert(("Source size must be equal to layer size", size == from.size()));

		data = Vector<DataType>(size);
		std::get<Vector<DataType>>(data).forAllElements([&] (Index i, DataType& v) {
			v = from.at(i);
		});
	}

	template <typename DataType>
	void setFrom(const Vector<DataType>& from) {
		assert(("Source size must be equal to layer size", size == from.size()));

		data = Vector<DataType>(size);
		std::get<Vector<DataType>>(data).forAllElements([&] (Index i, DataType& v) {
			v = from[i];
		});
	}

	template <typename DataType>
	DataType& operator[](const Index& index) {
		return std::get<Vector<DataType>>(data)[index];
	}

	template <typename DataType>
	Vector<DataType>& get() {
		return std::get<Vector<DataType>>(data);
	}

	template <typename DataType>
	static Layer makeLayer(const Index& size, const DataType& value = DataType()) {
		Layer l;
		l.template init(size, value);
		return l;
	}

	template <typename Writer>
	void writeCellData(Writer& writer, const std::string& name) {
		const auto writeCellData_f = [&] (auto& v) {
			writer.writeCellData(v, name);
		};
		LVFOR(data, writeCellData_f);
	}
	template <typename Writer>
	void writeDataArray(Writer& writer, const std::string& name) {
		const auto writeDataArray_f = [&] (auto& v) {
			writer.writeDataArray(v, name);
		};
		LVFOR(data, writeDataArray_f);
	}
};

template <typename Device = TNL::Devices::Host, typename Index = int>
class LayerManager {
	public:
	using LayerType = Layer<Device, Index>;
	template <typename DataType> using Vector = TNL::Containers::Vector<DataType, Device, Index>;
	std::vector<Layer<Device, Index>> layers;
	protected:
	Index size;
	public:
	void setSize(const Index& newSize) {
		size = newSize;
		for (auto& layer : layers) layer.setSize(size);
	}
	std::size_t count() const { return layers.size(); }

	void clear() {
		while (!layers.empty()) layers.erase(layers.begin());
	}

	template <typename DataType>
	std::size_t add(const DataType& value = DataType()) {
		layers.push_back(LayerType::template makeLayer<DataType>(size, value));
		return layers.size() - 1;
	}

	template <typename DataType>
	Vector<DataType>& get(const std::size_t& index) {
		return layers.at(index).template get<DataType>();
	}

	LayerType& getLayer(const std::size_t& index) {
		return layers.at(index);
	}
};
