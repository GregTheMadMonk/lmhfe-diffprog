# Output directory
OUTPUT_DIR=./build

# Include directories
INCLUDE_DIRS=-I./includes/tnl/src -I./includes/tnl/src/3rdparty/ -I./src

# Flags for both g++ and nvcc
FLAGS=-g -std=c++17 -lstdc++fs -DHAVE_TINYXML2 -ltinyxml2
# g++-specific flags
GCPP_FLAGS=-fopenmp -Winline
# nvcc-specific flags
NVCC_FLAGS=-x cu --expt-relaxed-constexpr --expt-extended-lambda -DHAVE_CUDA -Xcudafe --diag_suppress=esa_on_defaulted_function_ignored

# GMRES doesn't compile with nvcc
TESTS=$(patsubst tests/%.cpp,%,$(wildcard tests/*.cpp))
TARGETS_CUDA=$(patsubst %,%_cuda,$(TESTS))
TARGETS_HOST=$(patsubst %,%_host,$(TESTS))
TARGETS_OMP=$(patsubst %,%_omp,$(TESTS))

all: all_host all_omp all_cuda

all_host: $(TARGETS_HOST)
all_omp: $(TARGETS_OMP)
all_cuda: $(TARGETS_CUDA)

output_dir:
	mkdir -p $(OUTPUT_DIR)

%_host: tests/%.cpp tests/*.hpp src/*.hpp output_dir
	c++ $(INCLUDE_DIRS) -o $(OUTPUT_DIR)/$@ $< $(FLAGS) $(GCPP_FLAGS)
%_omp: tests/%.cpp tests/*.hpp src/*.hpp output_dir
	c++ $(INCLUDE_DIRS) -o $(OUTPUT_DIR)/$@ $< $(FLAGS) $(GCPP_FLAGS) -DHAVE_OPENMP
%_cuda: tests/%.cpp tests/*.hpp src/*.hpp output_dir
	nvcc $(INCLUDE_DIRS) -o $(OUTPUT_DIR)/$@ $< $(FLAGS) $(NVCC_FLAGS)

clean:
	rm -r $(OUTPUT_DIR)
