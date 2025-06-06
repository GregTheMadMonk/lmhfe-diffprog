#include <benchmark/benchmark.h>

import dxx.cstd.fixed;
import math;
import std;
import utils;

namespace {

namespace rng = utils::random::generators;

const auto a = std::views::take(rng::normal<f32>(-200.0, 200.0), 1024)
               | std::ranges::to<std::vector<f32>>();
const auto b = std::views::take(rng::normal<f32>(-200.0, 200.0), 1024)
               | std::ranges::to<std::vector<f32>>();

const auto a_al = std::views::take(rng::normal<f32>(-200.0, 200.0), 1024)
               | std::ranges::to<std::vector<f32, utils::aligned::Allocator<f32, 256>>>();
const auto b_al = std::views::take(rng::normal<f32>(-200.0, 200.0), 1024)
               | std::ranges::to<std::vector<f32, utils::aligned::Allocator<f32, 256>>>();

inline
void dot_product(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(math::dot(a, b));
    }
}

inline
void dot_product_aligned(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(math::dot(a_al, b_al));
    }
}

BENCHMARK(dot_product);
BENCHMARK(dot_product_aligned);

} // <-- namespace <anonymous>
