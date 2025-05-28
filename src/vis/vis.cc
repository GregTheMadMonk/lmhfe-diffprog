import dxx.cstd.compat;
import dxx.cstd.fixed;
import mesh;
import mhfe;
import std;
import utils;

int main(int argc, char** argv) {
    if (argc != 3 && argc != 5) {
        std::println(std::cerr, "Usage: {} T dt [x_mul y_mul]", argv[0]);
        return EXIT_FAILURE;
    }

    using Real = f64;

    using Mesh = mesh::Triangular<Real>;
    using Cell = Mesh::Cell;

    const Real T  = std::stod(argv[1]);
    const Real dt = std::stod(argv[2]);

    const uz x_mul = (argc == 3) ? 1 : std::stoll(argv[3]);
    const uz y_mul = (argc == 3) ? 1 : std::stoll(argv[4]);

    const auto mo = mesh::gen_rect<Real>(40 * x_mul, 20 * y_mul, 20, 10);
    if (!mo.has_value()) {
        std::println(std::cerr, "Could not generate the mesh for some reason");
        return EXIT_FAILURE;
    }

    ::mhfe::Problem<Real> prob{};
    prob.tau = dt;
    prob.mesh = *mo;
    prob.a.resize(prob.mesh.cells.size(), 1);
    prob.c.resize(prob.mesh.cells.size(), 1);

    prob.dirichlet_mask.resize(prob.mesh.edges.size(), 0);
    prob.dirichlet.resize(prob.mesh.edges.size(), 0);
    prob.neumann_mask.resize(prob.mesh.edges.size(), 0);
    prob.neumann.resize(prob.mesh.edges.size(), 0);

    for (auto [ e_idx, edge ] : enumerate(prob.mesh.edges)) {
        if (!edge.is_boundary()) {
            continue;
        }

        const auto p1 = prob.mesh.points[edge.points[0]];

        const auto d = prob.mesh.get_edge_dir(e_idx);
        if (d[0] == 0) { // x = const
            const bool mask = p1[0] == 0
                              && (p1[1] + d[1] / 2 > 1)
                              && (p1[1] + d[1] / 2 < 9);
            prob.dirichlet_mask[e_idx] = 1;
            prob.dirichlet[e_idx] = mask ? 1.0 : 0.0;
        } else if (d[1] == 0) { // y = const
            prob.neumann_mask[e_idx] = 1;
            prob.neumann[e_idx] = 0;
        }
    }

    prob.points = prob.mesh.points.size();
    prob.edges  = prob.mesh.edges.size();
    prob.cells  = prob.mesh.cells.size();

    if (!prob.is_valid()) {
        std::println(std::cerr, "Problem initialization error");
        return EXIT_FAILURE;
    }

    std::println(std::cerr, "Running vanilla LMHFE");

    std::println("import numpy as np");
    std::println("import pyvista as pv");

    std::println("points = {}", mo->points);
    std::println("cells = {}", std::views::transform(mo->cells, &Cell::points));

    std::println("surf = pv.PolyData(");
    std::println("    [ [ p[0], p[1], 0 ] for p in points ],");
    std::println("    [ [ 3, *c ] for c in cells ],");
    std::println(")");

    const auto s_ms = utils::timeit([&] {
        mhfe::LMHFE<Real> solver{ prob, 1e-7 };
        std::println("solution = np.array([");
        while (solver.get_time() < T) {
            std::print(std::cerr, "\r{:20}/{:<20}", solver.get_time(), T);
            solver.step();
            std::println("    {},", solver.get_solution());
        }
        std::println("])");
        std::println(std::cerr, "\nDone");
    }).count();

    std::println(
R"py(
pl = pv.Plotter(off_screen=True)
for i, sol in enumerate(solution):
    pl.clear()
    pl.add_mesh(surf, scalars=sol)
    pl.save_graphic(f'solution/{}.svg')
)py", "{i}"
    );

    const auto fw_ms = utils::timeit([&] {
        const auto g_wrt_s = [] (const auto& p, auto& o) {
            std::ranges::fill(o, 1.0 / p.size());
        };
        const auto g_wrt_a = [] (auto& o) { std::ranges::fill(o, 0); };
        mhfe::FwdDiff<Real, decltype(g_wrt_s), decltype(g_wrt_a)> solver{
            prob, 1e-7, g_wrt_s, g_wrt_a
        };
        std::println("fwd_diff = np.array([");
        while (solver.get_time() < T) {
            std::print(std::cerr, "\r{:20}/{:<20}", solver.get_time(), T);
            solver.step();
            std::println("    {},", solver.get_sensitivity());
        }
        std::println("])");
        std::println(std::cerr, "\nDone");
    }).count();

    std::println(
R"py(
for i, sol in enumerate(fwd_diff):
    pl.clear()
    pl.add_mesh(surf, scalars=sol)
    pl.save_graphic(f'fwddiff/{}.svg')
)py", "{i}"
    );

    const auto fd_ms = utils::timeit([&] {
        const auto g_wrt_s = [] (const auto& p, auto& o) {
            std::ranges::fill(o, 1.0 / p.size());
        };
        const auto g_wrt_a = [] (auto& o) { std::ranges::fill(o, 0); };
        mhfe::FinDiff<Real> solver{ prob, 1e-7, 0.01 };
        while (solver.get_time() < T) {
            std::print(std::cerr, "\r{:20}/{:<20}", solver.get_time(), T);
            solver.step();
        }
        std::println(
            "fd_diff = np.array({})",
            solver.get_sensitivity(
                [] (const auto& p) {
                    return std::reduce(p.cbegin(), p.cend()) / p.size();
                }
            )
        );
        std::println(std::cerr, "\nDone");
    }).count();

    std::println(
R"py(
pl.clear()
pl.add_mesh(surf, scalars=fd_diff)
pl.save_graphic(f'fd_diff.svg')
)py", "{i}"
    );

    std::println(
        std::cerr,
        "Took: {}ms | {}ms | {}ms", s_ms, fw_ms, fd_ms
    );
}
