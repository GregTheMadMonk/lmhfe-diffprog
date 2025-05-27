import test_utils;

namespace test::mhfe::problem {

const UnitTest is_valid{
    "is_valid", [] {
        ::mhfe::Problem<f32> p;
        test(!p.is_valid());
    }
}; // <-- is_valid

} // <-- namespace test::mhfe::problem
