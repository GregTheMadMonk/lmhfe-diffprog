import test_utils;

namespace test::mesh {

using Mesh = ::mesh::Triangular<f32>;

const Mesh m{
    .points = {
        { 0.0f, 0.0f },
        { 0.0f, 1.0f },
        { 1.0f, 0.0f },
        { 1.0f, 1.0f },
    },
    .edges = {
        { .points = { 0, 1 }, .cells = { 0, ::mesh::no_cell } },
        { .points = { 0, 2 }, .cells = { 0, ::mesh::no_cell } },
        { .points = { 1, 2 }, .cells = { 1, 0               } },
        { .points = { 1, 3 }, .cells = { 1, ::mesh::no_cell } },
        { .points = { 2, 3 }, .cells = { 1, ::mesh::no_cell } },
    },
    .cells = {
        {
            .points = { 0, 1, 2 },
            .edges  = { 0, 1, 2 },
        },
        {
            .points = { 1, 3, 2 },
            .edges  = { 2, 3, 4 },
        },
    },
}; // <-- m

const UnitTest validate{
    "validate", [] {
        test(m.is_valid());
    }
}; // <-- validate

const UnitTest measure{
    "measure", [] {
        test(m.cell_measure(0) == 0.5f);
    }
}; // <-- measure

const UnitTest center{
    "center", [] {
        test(m.cell_center(0) == std::array{ 1.0f / 3.0f, 1.0f / 3.0f });
    }
}; // <-- center

const UnitTest saveload{
    "saveload", [] {
        struct FileDeleter {
            const std::filesystem::path tp{ "mesh.txt" };
            ~FileDeleter() {
                std::filesystem::remove(tp);
            }
        } tp {};

        dxx::assert::always(!std::filesystem::exists(tp.tp));

        m.dump(std::ofstream{ tp.tp });

        Mesh m1;
        m1.read(std::ifstream{ tp.tp });

        test(std::ranges::equal(m.points, m1.points));
        test(std::ranges::equal(m.edges, m1.edges));
        test(std::ranges::equal(m.cells, m1.cells));
    }
}; // <-- saveload

const UnitTest direct{
    "direct", [] {
        auto mc = m;
        mc.direct();
        test( mc.is_edge_clockwise(0, 0));
        test(!mc.is_edge_clockwise(1, 0));
        test( mc.is_edge_clockwise(2, 0));

        test(!mc.is_edge_clockwise(2, 1));
        test( mc.is_edge_clockwise(3, 1));
        test(!mc.is_edge_clockwise(4, 1));
    }
}; // <-- direct

} // <-- namespace test::mesh
