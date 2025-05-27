#include <benchmark/benchmark.h>

import dxx.cstd.fixed;
import math;
import std;
import utils;

namespace {

namespace rng = utils::random::generators;

const auto a = std::views::take(rng::normal<f32>(-200.0, 200.0), 1024)
               | std::ranges::to<std::vector<f32>>();

const auto A_sparse = [] {
    math::CSR<f32> ret(1024, 1024);
    auto gen = rng::normal<f32>(-200.0, 200.0).begin();
    for (i64 i : range(0uz, ret.get_rows())) {
        for (i64 j : range(0uz, ret.get_cols())) {
            if (std::abs(i - j) <= 3) {
                ret[i, j] = *++gen;
            }
        }
    }
    return ret;
} (); // <-- A_sparse

const auto A_dense = [] {
    std::vector<f32> ret{};
    for (uz i : range(0uz, 1024uz)) {
        for (uz j : range(0uz, 1024uz)) {
            ret.push_back(A_sparse.at(i, j));
        }
    }
    return ret;
} (); // <-- A_dense

inline void matvec_dense(benchmark::State& state) {
    std::vector<f32> out(a.size());
    for (auto _ : state) {
        math::matvec(A_dense, a, out);
    }
} // <-- matvec_dense(state)

inline void matvec_sparse(benchmark::State& state) {
    std::vector<f32> out(a.size());
    for (auto _ : state) {
        math::matvec(A_sparse, a, out);
    }
} // <-- matvec_sparse(state)

BENCHMARK(matvec_dense);
BENCHMARK(matvec_sparse);

} // <-- namespace <anonymous>
