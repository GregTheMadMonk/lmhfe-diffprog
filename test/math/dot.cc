import test_utils;

namespace test::math::dot {

const UnitTest test1 = [] {
    std::vector u{ 1.0f, 2.0f, 3.0f };
    std::vector v{ 3.0f, 2.0f, 1.0f };

    test(10.0f == ::math::dot(u, v));
    test(10.0f == ::math::dot(v, u));
}; // <-- test1

} // <-- namespace test::math::dot
