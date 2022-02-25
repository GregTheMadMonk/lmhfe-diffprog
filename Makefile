INCLUDE=-I./includes/tnl/src -I./includes/tnl/src/3rdparty/
FLAGS=-g -std=c++17
HOST_FLAGS=-fopenmp
CUDA_FLAGS=-x cu --expt-relaxed-constexpr --expt-extended-lambda -DHAVE_CUDA -Xcudafe --diag_suppress=esa_on_defaulted_function_ignored

# GMRES doesn't compile with nvcc
TARGETS=mhfe_host mhfe_omp#  mhfe_cuda

all: $(TARGETS)

%_host: %.cpp *.hpp
	c++ $(INCLUDE) -o $@ $< $(FLAGS) $(HOST_FLAGS)
%_omp: %.cpp *.hpp
	c++ $(INCLUDE) -o $@ $< $(FLAGS) $(HOST_FLAGS) -DHAVE_OPENMP
%_cuda: %.cpp *.hpp
	nvcc $(INCLUDE) -o $@ $< $(FLAGS) $(CUDA_FLAGS)

clean:
	-rm $(TARGETS)
