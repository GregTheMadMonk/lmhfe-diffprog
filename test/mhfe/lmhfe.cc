import test_utils;

namespace test::mhfe::lmhfe {

using Real = f32;

const auto prob = [] {
    ::mhfe::Problem<Real> prob{};
    prob.tau = 0.1;
    prob.mesh = mesh::gen_rect<Real>(40, 20, 20, 10).value().direct();
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

    return prob;
} (); // <-- prob

const UnitTest lmhfe{
    "lmhfe", [] {
        test(prob.is_valid());

        ::mhfe::LMHFE solver(prob, 1e-6f);

        namespace stdc = std::chrono;
        const auto start = stdc::system_clock::now();
        for (uz _ : range(0uz, 10uz)) {
        //while (solver.get_time() < 10.0) {
            solver.step();
        }
        const auto end = stdc::system_clock::now();
        std::println(
            "    - simulated {} steps in {}ms",
            static_cast<uz>(solver.get_time() / prob.tau),
            stdc::duration_cast<stdc::milliseconds>(end - start).count()
        );

        using namespace std::literals;
        if (const auto* ds = std::getenv("SOL_DUMP"); ds && "1"sv == ds) {
            std::println("solution={}", solver.get_solution());
            std::println("points={}", prob.mesh.points);
            std::println(
                "cells={}",
                std::views::transform(
                    prob.mesh.cells,
                    &decltype(prob.mesh)::Cell::points
                )
            );
        }
    }
}; // <-- lmhfe

const UnitTest fin_diff{
    "fin_diff", [] {
        test(prob.is_valid());

        ::mhfe::FinDiff<Real> solver(prob, 1e-6f, 0.01);

        namespace stdc = std::chrono;
        const auto start = stdc::system_clock::now();
        for (uz _ : range(0uz, 10uz)) {
            solver.step();
        }
        const auto sens = solver.get_sensitivity(
            [] (const auto& v) { return std::reduce(v.cbegin(), v.cend()) / v.size(); }
        );
        const auto end = stdc::system_clock::now();
        std::println(
            "    - simulated 10 steps in {}ms",
            stdc::duration_cast<stdc::milliseconds>(end - start).count()
        );

        using namespace std::literals;
        if (const auto* ds = std::getenv("SOL_DUMP"); ds && "1"sv == ds) {
            std::println("sensitivity={}", sens);
        }
    }
}; // <-- fin_diff

const UnitTest fwd_diff{
    "fwd_diff", [] {
        test(prob.is_valid());

        const auto g_wrt_P = [] (const auto& P, auto& out) {
            std::ranges::fill(out, 1.0f / P.size());
        }; // <-- g_wrt_P

        const auto g_wrt_a = [] (auto& out) { std::ranges::fill(out, 0); };

        ::mhfe::FwdDiff<Real, decltype(g_wrt_P), decltype(g_wrt_a)> solver(
            prob, 1e-6f, g_wrt_P, g_wrt_a
        );

        namespace stdc = std::chrono;
        const auto start = stdc::system_clock::now();
        for (uz _ : range(0uz, 10uz)) {
            solver.step();
        }
        const auto sens = solver.get_sensitivity();
        const auto end = stdc::system_clock::now();
        std::println(
            "    - simulated 10 steps in {}ms",
            stdc::duration_cast<stdc::milliseconds>(end - start).count()
        );

        using namespace std::literals;
        if (const auto* ds = std::getenv("SOL_DUMP"); ds && "1"sv == ds) {
            std::println("sensitivity={}", sens);
        }
    }
}; // <-- fwd_diff

} // <-- namespace test::mhfe::lmhfe
