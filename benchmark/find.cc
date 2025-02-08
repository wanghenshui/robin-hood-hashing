#include <benchmark/benchmark.h>
#include <cstdint>
// taskset -c 0  ./hamming_weight
#define BM(x) BENCHMARK(BM_##x)

#include <algorithm>
#include <random>
#include <vector>
#include <robin_hood.h>
// 带有预分配的优化版本
//
using map_t = robin_hood::unordered_map<int, int>;
void insertSequentialKeysOptimized(map_t& map,
                                 size_t count,
				 size_t rate = 100,
                                 const int& defaultValue = 0) {
    // 预分配空间（如果容器支持）
    if constexpr (requires(map_t m) { m.reserve(size_t{}); }) {
        map.reserve(map.size() + count);
    }
    
    for (size_t i = 0; i < count; ++i) {
	if (count % 100 < rate)
        map[i] = defaultValue;
    }
}


void BM_robin_map_find(benchmark::State& state) {
  robin_hood::unordered_map<int,int> m; 
  size_t count = state.range(0);
  size_t rate = state.range(1);
  insertSequentialKeysOptimized(m, count, rate);
  for (auto _ : state) {
    for (size_t i = 0; i < count; ++i) {
      benchmark::DoNotOptimize(m.find(i));
    }
  }
}


BM(robin_map_find)->Args({1000000, 80})
                        ->Args({1000000, 95})
                        ->Args({4000000,80})
                        ->Args({4000000,95})
                        ->Args({10000000,80})
	                ->Args({10000000,95})
	;


BENCHMARK_MAIN();
