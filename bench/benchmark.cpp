
#include "../include/iterator.h"

#include <benchmark/benchmark.h>

static void BM_count_if(benchmark::State &s) {
	int arr[1000];
	for (int i = 0; i < 1000; i++) { arr[i] = i; }

	for ([[maybe_unused]] auto _: s) {
		uint64 count = algo::count(it::filter(it::iterator(arr, 1000), [](int i) { return i % 2 == 0; }));

		benchmark::DoNotOptimize(std::move(count));
		benchmark::DoNotOptimize(std::move(arr));
	}
}

BENCHMARK(BM_count_if);

uint64 count_pairs(int *arr) {
	const auto it = it::iterator(arr, 1000);

	return algo::count(it::filter(
			it::filter(it::map(it::filter(it::cross_product(it, it), [](auto p) { return p.first >= p.second; }),
							   [](auto p) {
								   struct pair_t {
									   int first;
									   int second;
								   };
								   return pair_t{p.first + 5, p.second};
							   }),
					   [](auto p) { return p.first * p.second >= 4900; }),
			[](auto p) { return p.first * p.second <= 4964; }));
}
int max(int a, int b) { return a > b ? a : b; }
int min(int a, int b) { return a > b ? b : a; }

uint64 count_pairs_better(int *arr) {
	const auto it = it::iterator(arr, 1000);

	return algo::count(
			it::filter(it::filter(it::map(it::unordered_pairs(it),
										  [](auto p) {
											  struct pair_t {
												  int first;
												  int second;
											  };
											  return pair_t{max(p.first, p.second) + 5, min(p.first, p.second)};
										  }),
								  [](auto p) { return p.first * p.second >= 4900; }),
					   [](auto p) { return p.first * p.second <= 4964; }));
}

uint64 count_pairs_naive(int *arr) {
	const uint64 len = 1000;

	uint64 count = 0;

	for (uint64 i = 0; i < len; i++) {
		for (uint64 j = i; j < len; j++) {
			int larger  = max(arr[i], arr[j]);
			int smaller = min(arr[i], arr[j]);
			int val     = (larger + 5) * smaller;
			if (val >= 4900 && val <= 4964) { count++; }
		}
	}
	return count;
}
uint64 count_pairs_naive2(int *arr) {
	const uint64 len = 1000;

	uint64 count = 0;

	for (uint64 i = 0; i < len; i++) {
		for (uint64 j = 0; j <= i; j++) {
			int larger  = max(arr[i], arr[j]);
			int smaller = min(arr[i], arr[j]);
			int val     = (larger + 5) * smaller;
			if (val >= 4900 && val <= 4964) { count++; }
		}
	}
	return count;
}


template<auto VERSION>
static void BM_count_pairs(benchmark::State &s) {
	int arr[1000];
	for (int i = 0; i < 1000; i++) { arr[i] = i; }

	for ([[maybe_unused]] auto _: s) {

		uint64 count = VERSION(arr);

		benchmark::DoNotOptimize(std::move(count));
		benchmark::DoNotOptimize(std::move(arr));
	}
}

BENCHMARK(BM_count_pairs<count_pairs_naive>);
BENCHMARK(BM_count_pairs<count_pairs_naive2>);
BENCHMARK(BM_count_pairs<count_pairs_better>);
BENCHMARK(BM_count_pairs<count_pairs>);


BENCHMARK_MAIN();
