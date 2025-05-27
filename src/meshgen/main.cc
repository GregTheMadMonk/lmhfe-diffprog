import dxx.cstd.compat;
import dxx.cstd.fixed;
import mesh;
import std;
import utils;

using Real = f64;

int main(int argc, char** argv) {
    if (argc != 5) {
        std::println(std::cerr, "Usage: {} Nx Ny X Y", argv[0]);
        return EXIT_FAILURE;
    }

    const uz   N_x = std::stol(argv[1]);
    const uz   N_y = std::stol(argv[2]);
    const Real X   = std::stod(argv[3]);
    const Real Y   = std::stod(argv[4]);

    if (const auto mo = mesh::gen_rect(N_x, N_y, X, Y); mo.has_value()) {
        mo.value().dump(std::cout);
    } else {
        return EXIT_FAILURE;
    }
}
