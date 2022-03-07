#include <iostream>
#include <filesystem>

#include "mhfe.hpp"

using namespace std;

#ifdef HAVE_CUDA
using Device = TNL::Devices::Cuda;
#else
using Device = TNL::Devices::Host;
#endif

int main(int argc, char** argv) {
	mhfe<float, Device> solver;
	cin >> solver.Nx >> solver.Ny >> solver.tau;
	/*
	for (int i = 0; i < mhfe<float, Device>::get_edges(solver.Nx, solver.Ny); ++i) {
		auto p = mhfe<float, Device>::edge_midpoint(i, solver.X / solver.Nx, solver.Y / solver.Ny, solver.Nx, solver.Ny);
		cout << i << '\t' << p[0] << '\t' << p[1] << endl;
	}
	return 0;
	*/
	filesystem::remove_all(solver.saved_dir);
	solver.init();
	do {
		//cout << *solver.m << endl;
		//cout << solver.right << endl;
		//cout << solver.P << endl;
		cout << "\r[" << setw(10) << left << solver.t << "/" << right << solver.T << "]";
		cout.flush();
		if (!solver.save_midpoints(to_string(solver.t) + ".dat", ' ')) return EXIT_FAILURE;
	} while(solver.step());
	cout << endl;
	return EXIT_SUCCESS;
}
