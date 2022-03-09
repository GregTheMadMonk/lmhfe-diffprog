#pragma once

// STL headers
#include <string>
#include <vector>

// TNL headers
#include <TNL/Containers/Vector.h>

template <class Index, typename Writer>
class LayerBase {
	public:
	virtual void setSize(const Index& size) = 0;
	virtual Index getSize() const = 0;
	virtual void writeCellData(Writer& writer, const std::string& name) = 0;
};

template <typename Data, typename Device, typename Index, typename Writer>
struct Layer : public LayerBase<Index, Writer> {
	using Vector = TNL::Containers::Vector<Data, Device, Index>;
	Vector data;
	Layer(const Index& size) {
		setSize(size);
	}
	inline void setSize(const Index& size) {
		data.setSize(size);
	}
	inline Data& operator[](const Index& index) {
		return data[index];
	}
	inline Index getSize() const { return data.getSize(); }

	inline void writeCellData(Writer& writer, const std::string& name) {
		writer.writeCellData(data, name);
	}
};

template <typename Index, typename Device, typename Writer>
class LayerManager {
	std::vector<LayerBase<Index, Writer>*> layers;
	Index size;

	public:
	inline void setSize(const Index& newSize) {
		size = newSize;
		for (auto& layer : layers) layer->setSize(size);
	}

	inline std::size_t count() { return layers.size(); }

	inline void clear() {
		while (!layers.empty()) {
			delete layers.at(0);
			layers.erase(layers.begin());
		}
	}

	template <typename Data>
	inline std::size_t add() {
		Layer<Data, Device, Index, Writer>* layer = new Layer<Data, Device, Index, Writer>(size);
		layers.push_back(layer);
		return layers.size() - 1;
	}
	template <typename Data>
	inline Layer<Data, Device, Index, Writer>& get(const std::size_t& index) {
		const auto ptr = dynamic_cast<Layer<Data, Device, Index, Writer>*>(layers.at(index));
		// dynamic_cast will return nullptr on child template type mismatch
		if (ptr == nullptr) throw std::runtime_error("Couldn't return layer, probably wrong data type!");
		return *ptr;
	}
	inline LayerBase<Index, Writer>* getBasePtr(const std::size_t& index) {
		return layers.at(index);
	}
};
