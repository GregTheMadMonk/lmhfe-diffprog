import test_utils;

namespace test::math::csr {

const UnitTest create{
    "create", [] {
        ::math::CSR<f32> csr(2, 2);
        test(csr[0, 0]    == 0);
        test(csr.at(0, 1) == 0);
        test(csr[1, 0]    == 0);
        test(csr.at(1, 1) == 0);

        test(csr.find(0, 0) != nullptr);
        test(csr.find(0, 1) == nullptr);
        test(csr.find(1, 0) != nullptr);
        test(csr.find(1, 1) == nullptr);

        test(set_equal(csr.get_row(0), std::set{0}));
        test(set_equal(csr.get_row(1), std::set{0}));

        csr[0, 0] = 1;
        test(csr.at(0, 0) == 1);
        test(csr.at(0, 1) == 0);
        test(csr.at(1, 0) == 0);
        test(csr.at(1, 1) == 0);
        
        csr[1, 0] += 2;
        test(csr.at(0, 0) == 1);
        test(csr.at(0, 1) == 0);
        test(csr.at(1, 0) == 2);
        test(csr.at(1, 1) == 0);

        csr.push(0, 1, 10);
        test(csr.at(0, 0) == 1);
        test(csr.at(0, 1) == 10);
        test(csr.at(1, 0) == 2);
        test(csr.at(1, 1) == 0);

        test(set_equal(csr.get_row(0), std::set{0, 1}));
        test(set_equal(csr.get_row(1), std::set{0}));

        csr.reset();

        test(csr.at(0, 0) == 0);
        test(csr.at(0, 1) == 0);
        test(csr.at(1, 0) == 0);
        test(csr.at(1, 1) == 0);

        test(set_equal(csr.get_row(0), std::set<uz>{}));
        test(set_equal(csr.get_row(1), std::set<uz>{}));
    }
}; // <-- create

} // <-- namespace test::math::csr
