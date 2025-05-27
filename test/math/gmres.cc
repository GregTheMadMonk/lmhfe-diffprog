import test_utils;

namespace test::math::gmres {

const ::math::gmres::Options<f64> opt{};

const UnitTest test_2x2_diag{
    "2x2_diag", [] {
        const std::vector<f64> A{ 1.0, 0.0, 0.0, 2.0 };
        const std::vector<f64> b{ 1.0, 2.0 };
        const auto x = ::math::gmres::solve(A, b);
        const auto x0 = std::vector{1.0, 1.0};
        const auto atol = std::max(std::ranges::max(x), std::ranges::max(x0))
                          * opt.tol;
        test(all_close(x, x0, atol));
    }
}; // <-- 2x2_diag

const UnitTest test_2x2{
    "2x2", [] {
        const std::vector<f64> A{ 1.0, 8.0, 4.0, 2.0 };
        const std::vector<f64> b{ 13.0, 7.0 };
        const auto x = ::math::gmres::solve(A, b);
        const auto x0 = std::vector{ 1.0, 1.5 };
        const auto atol = std::max(std::ranges::max(x), std::ranges::max(x0))
                          * opt.tol;
        test(all_close(x, x0, atol));
    }
}; // <-- 2x2

const UnitTest test_2x2_2{
    "2x2_2", [] {
        const std::vector<f64> A{ 1.0, 3.0, -1.0, 2.0 };
        const std::vector<f64> b{ 1.0, 0.0 };
        const auto x = ::math::gmres::solve(A, b);
        const auto x0 = std::vector{ 0.4, 0.2 };
        const auto atol = std::max(std::ranges::max(x), std::ranges::max(x0))
                          * opt.tol;
        test(all_close(x, x0, atol));
    }
}; // <-- 2x2_2

const UnitTest test_20x20{
    "20x20", [] {
        namespace rng = utils::random::generators;

        static constexpr std::size_t n = 20;
        const auto A = std::views::take(rng::normal<f64>(-20.0, 20.0), n * n)
                     | std::ranges::to<std::vector<f64>>();
        const auto x0 = std::views::take(rng::normal<f64>(), n)
                      | std::ranges::to<std::vector<f64>>();
        const auto b0 = ::math::matvec(A, x0);

        const auto x = ::math::gmres::solve(A, b0);
        const auto atol = std::max(std::ranges::max(x), std::ranges::max(x0))
                          * opt.tol;
        test(all_close(x, x0, atol));
    }
}; // <-- 20x20

#ifdef NDEBUG
const ::math::gmres::Options<f64> big_opt{ .max_iters = 10000, .tol = 1e-7 };

const UnitTest test_2000x2000{
    "2000x2000", [] {
        namespace rng = utils::random::generators;

        static constexpr std::size_t n = 2000;
        std::println("    -- generating");
        auto A = std::views::take(rng::normal<f64>(-200.0, 200.0), n * n)
                     | std::ranges::to<std::vector<f64>>();
        for (i64 i : range(0uz, n)) { 
            for (i64 j : range(0uz, n)) {
                if (std::abs(i - j) > 3) A[i * n + j] = 0;
            }
        }
        const auto x0 = std::views::take(rng::normal<f64>(), n)
                      | std::ranges::to<std::vector<f64>>();
        std::println("    -- calculating RHS");
        const auto b0 = ::math::matvec(A, x0);

        std::println("    -- solving");
        namespace stdc = std::chrono;
        const auto start = stdc::system_clock::now();
        const auto x = ::math::gmres::solve(A, b0, { .max_iters = 10000 });
        const auto end   = stdc::system_clock::now();
        std::println(
            "    -- done in {}ms",
            stdc::duration_cast<stdc::milliseconds>(end - start).count()
        );
        std::println("    -- comparing");
        const auto b = ::math::matvec(A, x);
        const auto atol = std::max(std::ranges::max(b), std::ranges::max(b0))
                          * big_opt.tol;
        test(all_close(b, b0, atol));
    }
}; // <-- 2000x2000

const UnitTest test_sparse_2000x2000{
    "sparse_2000x2000", [] {
        namespace rng = utils::random::generators;

        static constexpr std::size_t n = 2000;
        std::println("    -- generating");
        const auto Ad = std::views::take(rng::normal<f64>(-200.0, 200.0), n * n)
                     | std::ranges::to<std::vector<f64>>();
        ::math::CSR<f64> A(n, n);
        for (i64 i : range(0uz, n)) { 
            for (i64 j : range(0uz, n)) {
                if (std::abs(i - j) <= 3) A.push(i, j, Ad[i * n + j]);
            }
        }
        const auto x0 = std::views::take(rng::normal<f64>(), n)
                      | std::ranges::to<std::vector<f64>>();
        std::println("    -- calculating RHS");
        const auto b0 = ::math::matvec(A, x0);

        std::println("    -- solving");
        namespace stdc = std::chrono;
        const auto start = stdc::system_clock::now();
        const auto x = ::math::gmres::solve(A, b0, { .max_iters = 10000 });
        const auto end   = stdc::system_clock::now();
        std::println(
            "    -- done in {}ms",
            stdc::duration_cast<stdc::milliseconds>(end - start).count()
        );
        std::println("    -- comparing");
        const auto b = ::math::matvec(A, x);
        const auto atol = std::max(std::ranges::max(b), std::ranges::max(b0))
                          * big_opt.tol;
        test(all_close(b, b0, atol));
    }
}; // <-- sparse_2000x2000
#endif

} // <-- namespace test::math::gmres
