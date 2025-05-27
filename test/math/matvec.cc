import test_utils;

namespace test::math::matvec {

namespace dense {

const UnitTest test_2x2{
    "2x2", [] {
        const std::vector A{ 1.0, 2.0, 4.0, 3.0 };
        const std::vector x{ 1.0, 2.0 };
        const auto b = ::math::matvec(A, x);
        test(std::ranges::equal(b, std::vector{ 5.0, 10.0 }));
    }
}; // <-- 2x2

const UnitTest test_2x2_addition{
    "2x2_addition", [] {
        const std::vector A{ 1.0, 2.0, 4.0, 3.0 };
        const std::vector x{ 1.0, 2.0 };
        std::vector out{ 1.0, 2.0 };
        ::math::matvec(A, x, out);
        test(std::ranges::equal(out, std::vector{ 6.0, 12.0 }));
    }
}; // <-- 2x2_addition

const UnitTest test_2x2_add_alpha{
    "2x2_add_alpha", [] {
        const std::vector A{ 1.0, 2.0, 4.0, 3.0 };
        const std::vector x{ 1.0, 2.0 };
        std::vector out{ 1.0, 2.0 };
        ::math::matvec(A, x, out, -2);
        test(std::ranges::equal(out, std::vector{ -9.0, -18.0 }));
    }
}; // <-- 2x2_add_alpha

const UnitTest test_2x2_diag{
    "2x2_diag", [] {
        const std::vector A{ 1.0, 0.0, 0.0, 3.0 };
        const std::vector x{ 1.0, 2.0 };
        const auto b = ::math::matvec(A, x);
        test(std::ranges::equal(b, std::vector{ 1.0, 6.0 }));
    }
}; // <-- 2x2_diag

} // <-- namespace dense

namespace csr {

const UnitTest test_2x2{
    "2x2", [] {
        ::math::CSR<f64> A(2, 2);
        A.push(0, 0, 1.0);
        A.push(0, 1, 2.0);
        A.push(1, 0, 4.0);
        A.push(1, 1, 3.0);
        const std::vector x{ 1.0, 2.0 };
        const auto b = ::math::matvec(A, x);
        test(std::ranges::equal(b, std::vector{ 5.0, 10.0 }));
    }
}; // <-- 2x2

const UnitTest test_2x2_addition{
    "2x2_addition", [] {
        ::math::CSR<f64> A(2, 2);
        A.push(0, 0, 1.0);
        A.push(0, 1, 2.0);
        A.push(1, 0, 4.0);
        A.push(1, 1, 3.0);
        const std::vector x{ 1.0, 2.0 };
        std::vector out{ 1.0, 2.0 };
        ::math::matvec(A, x, out);
        test(std::ranges::equal(out, std::vector{ 6.0, 12.0 }));
    }
}; // <-- 2x2_addition

const UnitTest test_2x2_add_alpha{
    "2x2_add_alpha", [] {
        ::math::CSR<f64> A(2, 2);
        A.push(0, 0, 1.0);
        A.push(0, 1, 2.0);
        A.push(1, 0, 4.0);
        A.push(1, 1, 3.0);
        const std::vector x{ 1.0, 2.0 };
        std::vector out{ 1.0, 2.0 };
        ::math::matvec(A, x, out, -2);
        test(std::ranges::equal(out, std::vector{ -9.0, -18.0 }));
    }
}; // <-- 2x2_add_alpha

const UnitTest test_2x2_diag{
    "2x2_diag", [] {
        ::math::CSR<f64> A(2, 2);
        A.push(0, 0, 1.0);
        A.push(1, 1, 3.0);
        const std::vector x{ 1.0, 2.0 };
        const auto b = ::math::matvec(A, x);
        test(std::ranges::equal(b, std::vector{ 1.0, 6.0 }));
    }
}; // <-- 2x2_diag

} // <-- namespace csr

} // <-- namespace test::math::matvec
