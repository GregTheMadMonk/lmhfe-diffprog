import test_utils;

namespace test::math::norm {

const UnitTest norm = [] {
    test(::math::norm::euclidean(std::vector{ 3.0, 4.0 }) == 5.0);
}; // <-- norm

} // <-- namespace test::math::norm
