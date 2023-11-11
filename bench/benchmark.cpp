#define D_ITERATOR_UNIT_TEST
#include "../include/array.h"
#include "../include/iterator.h"

#include <benchmark/benchmark.h>

static void BM_count_if(benchmark::State &s) {
	int arr[1000];
	for (int i = 0; i < 1000; i++) { arr[i] = i; }

	for ([[maybe_unused]] auto _: s) {
		uint64 count = algo::count(
				it::filter(it::iterator(arr, 1000), [](int i) { return i % 2 == 0; }));

		benchmark::DoNotOptimize(std::move(count));
		benchmark::DoNotOptimize(std::move(arr));
	}
}

BENCHMARK(BM_count_if);

uint64 count_pairs(int *arr) {
	const auto it = it::iterator(arr, 1000);

	return algo::count(
			it::filter(it::filter(it::map(it::filter(it::cross_product(it, it),
													 [](auto p) { return p.first >= p.second; }),
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

	return algo::count(it::filter(
			it::filter(
					it::map(it::unordered_pairs(it),
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

static void BM_skip(benchmark::State &s) {
	uint64 size = 1000;

	int *arr = new int[size];
	for (uint64 i = 0; i < size; i++) { arr[i] = i; }

	for ([[maybe_unused]] auto _: s) {

		const auto it = it::iterator(arr, size) | it::skip(500);

		benchmark::DoNotOptimize(std::move(*it));
		benchmark::DoNotOptimize(std::move(arr));
		benchmark::DoNotOptimize(std::move(size));
	}
	delete[] arr;
}

template<bool use_cache>
static void BM_caching_iterator(benchmark::State &s) {
	uint64 size = 1000;

	int *arr = new int[size];
	for (uint64 i = 0; i < size; i++) { arr[i] = i; }
	// use some map function to generate some work
	for ([[maybe_unused]] auto _: s) {
		auto it = it::iterator(arr, size) | it::map([](int i) { return i * i / 2 + i * 4; });
		if constexpr (use_cache) {
			auto c_it = it::caching_iterator(it);
			while (c_it.has_next()) {
				benchmark::DoNotOptimize(*c_it);
				benchmark::DoNotOptimize(*c_it);
				++c_it;
			}
		} else {
			while (it.has_next()) {
				benchmark::DoNotOptimize(*it);
				benchmark::DoNotOptimize(*it);
				++it;
			}
		}
		benchmark::DoNotOptimize(std::move(arr));
		benchmark::DoNotOptimize(std::move(size));
	}
	delete[] arr;
}

static void BM_stupid_count(benchmark::State &s) {

	uint64 size = 1000;

	int *arr = new int[size];
	for (uint64 i = 0; i < size; i++) { arr[i] = i; }

	for ([[maybe_unused]] auto _: s) {
		uint64 count = it::iterator(arr, size) | it::map([](int) -> uint64 { return 1ULL; })
					 | algo::reduce(uint64(0), [](uint64 a, uint64 b) { return a + b; });


		benchmark::DoNotOptimize(std::move(count));
		benchmark::DoNotOptimize(std::move(size));
	}
	delete[] arr;
}


template<uint64 size>
constexpr auto successors(array<uint8, size> conf) {
	return it::sequence_generator<uint8>(0, 8)
		 | it::map([conf](uint8 i) -> array<uint8, size + 1> { return i + conf; });
}

constexpr bool threats(int row1, int row2, int diag) {
	const int diff = row1 - row2;
	return diff == 0 || diff == diag || diff == -diag;
}

template<uint64 size>
constexpr bool legal(const array<uint8, size> conf) {
	if constexpr (size == 0) { return true; }
	auto [head, tail] = conf.head_tail();
	return it::infinite_sequence_generator(1U)                                          //
		 | it::zip(tail.to_iterator())                                                  //
		 | it::map([head](auto p) -> bool { return threats(head, p.second, p.first); }) //
		 | algo::any()                                                                  //
		 | it::negate();
}

template<it::CustomIterator CI>
std::vector<typename CI::value_type::value_type> flatten(CI it) {
	using T = typename CI::value_type::value_type;
	return it | algo::reduce(std::vector<T>{}, [](std::vector<T> a, std::vector<T> b) {
			   a.insert(a.end(), b.begin(), b.end());
			   return a;
		   });
}
struct flatten_ {};
constexpr flatten_ flatten() { return {}; }
template<it::CustomIterator CI>
auto operator|(CI it, flatten_) {
	return flatten(it);
}

template<uint64 size>
auto backtrack(array<uint8, size> conf) {
	if constexpr (size == 8) {
		return std::vector{conf};
	} else {
		return successors(conf)             //
			 | it::filter(legal<size + 1>)  //
			 | it::map(backtrack<size + 1>) //
			 | flatten();
	}
}

static void BM_n_queens(benchmark::State &s) {
	for ([[maybe_unused]] auto _: s) {
		auto solutions = backtrack(array<uint8, 0>{});
		benchmark::DoNotOptimize(std::move(solutions));
	}
}



constexpr auto successors_2(array_f<int8> conf) {
	return it::sequence_generator<uint8>(0, 8) | it::map([conf](int8 i) { return i + conf; });
}

constexpr bool legal_2(const array_f<int8> conf) {
	if (conf.size == 0) { return true; }
	auto [head, tail] = conf.head_tail();
	return it::infinite_sequence_generator(1U)                                          //
		 | it::zip(tail.to_iterator())                                                  //
		 | it::map([head](auto p) -> bool { return threats(head, p.second, p.first); }) //
		 | algo::any()                                                                  //
		 | it::negate();
}

template<it::CustomIterator CI>
std::vector<typename CI::value_type::value_type> flatten_2(CI it) {
	using T = typename CI::value_type::value_type;
	return it | algo::reduce(std::vector<T>{}, [](std::vector<T> a, std::vector<T> b) {
			   a.insert(a.end(), b.begin(), b.end());
			   return a;
		   });
}
struct flatten_2_ {};
constexpr flatten_2_ flatten_2() { return {}; }
template<it::CustomIterator CI>
constexpr auto operator|(CI it, flatten_2_) {
	return flatten_2(it);
}

auto backtrack_2(array_f<int8> conf) {
	// if (conf.size != depth) { __builtin_trap(); }
	// if (conf.size > 8) { __builtin_trap(); }
	if (conf.size == 8) {
		return std::vector{conf};
	} else {
		return successors_2(conf)                                                            //
			 | it::filter(legal_2)                                                           //
			 | it::map(backtrack_2) //
			 | flatten_2();
	}
}


static void BM_n_queens2(benchmark::State &s) {
	for ([[maybe_unused]] auto _: s) {
		auto solutions = backtrack_2(array_f<int8>{});
		benchmark::DoNotOptimize(std::move(solutions));
	}
}


BENCHMARK(BM_count_pairs<count_pairs_naive>);
BENCHMARK(BM_count_pairs<count_pairs_naive2>);
BENCHMARK(BM_count_pairs<count_pairs_better>);
BENCHMARK(BM_count_pairs<count_pairs>);
BENCHMARK(BM_caching_iterator<true>);
BENCHMARK(BM_caching_iterator<false>);
BENCHMARK(BM_skip);
BENCHMARK(BM_stupid_count);
BENCHMARK(BM_n_queens);
BENCHMARK(BM_n_queens2);

BENCHMARK_MAIN();
