#include "../apply_function.h"

#include <benchmark/benchmark.h>

#include <cmath>
#include <vector>

namespace {

static void BM_SmallVectorLightTransform(benchmark::State& state) {
  const int threads = static_cast<int>(state.range(0));

  for (auto _ : state) {
    std::vector<int> data(32, 1);
    ApplyFunction<int>(data, [](int& value) { value += 1; }, threads);
    benchmark::DoNotOptimize(data.data());
  }
}

static void BM_LargeVectorHeavyTransform(benchmark::State& state) {
  const int threads = static_cast<int>(state.range(0));

  for (auto _ : state) {
    std::vector<double> data(200000, 1.5);
    ApplyFunction<double>(
        data,
        [](double& value) {
          double acc = value;
          for (int i = 0; i < 200; ++i) {
            acc = std::sin(acc) + std::cos(acc) + std::sqrt(acc + 2.0);
          }
          value = acc;
        },
        threads);
    benchmark::DoNotOptimize(data.data());
  }
}

}  // namespace

BENCHMARK(BM_SmallVectorLightTransform)->Arg(1)->Arg(8);
BENCHMARK(BM_LargeVectorHeavyTransform)->Arg(1)->Arg(8);

BENCHMARK_MAIN();
