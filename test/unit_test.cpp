//
// Created by af on 25/10/23.
//

// use Google test as unit test framework
#include <algorithm>
#include <gtest/gtest.h>
#include <random>

#define D_ITERATOR_UNIT_TEST
#include "array.h"
#include "iterator.h"


TEST(array_iterator, array_iterator_int) {
	const uint64 len = 1000;
	int          arr[len]{};

	for (uint64 i = 0; i < len; i++) { arr[i] = int(i); }

	auto it = it::iterator(arr, len);
	ASSERT_EQ(*it, 0);
	ASSERT_EQ(algo::count(it), len);
	ASSERT_EQ(algo::count(it::filter(it, [](int e) { return e % 2 == 0; })), len / 2);
	ASSERT_EQ(algo::count(it::map(it, [](int e) { return double(e); })), len);
}

TEST(array_iterator, array_iterator_double) {
	const uint64 len = 1000;
	double       arr[len]{};

	for (uint64 i = 0; i < len; i++) { arr[i] = double(i); }

	auto it = it::iterator(arr, len);
	ASSERT_EQ(*it, 0);
	ASSERT_EQ(algo::count(it), len);
	ASSERT_EQ(algo::count(it::filter(it, [](double e) { return int(e) % 2 == 0; })), len / 2);
	ASSERT_EQ(algo::count(it::map(it, [](double e) { return int(e); })), len);
}

struct non_trivially_copyable {
	int a = 0;

	non_trivially_copyable() = default;
	explicit non_trivially_copyable(int a) : a(a) {}

	non_trivially_copyable(const non_trivially_copyable &a)            = delete;
	non_trivially_copyable &operator=(const non_trivially_copyable &a) = delete;

	void assign(int a_) { a = a_; }

	~non_trivially_copyable() { a = 0; }
};

TEST(array_iterator, array_iterator_non_trivially_copyable) {
	const uint64           len = 1000;
	non_trivially_copyable arr[len]{};

	for (uint64 i = 0; i < len; i++) { arr[i].assign(int(i)); }

	auto it = it::iterator(arr, len);
	ASSERT_EQ((*it).a, 0);
	ASSERT_EQ(algo::count(it), len);
	ASSERT_EQ(algo::count(
					  it::filter(it, [](const non_trivially_copyable &e) { return e.a % 2 == 0; })),
			  len / 2);
	ASSERT_EQ(algo::count(it::map(it, [](const non_trivially_copyable &e) { return e.a; })), len);
}

TEST(reduction_algoritm, simple_sum) {
	const it::infinite_sequence_generator<int> seq;

	const uint64 N     = 100;
	const uint64 SUM_N = N * (N + 1) / 2;

	const int sum = seq | it::take(N + 1) | algo::reduce(0, [](int a, int b) { return a + b; });

	ASSERT_EQ(sum, SUM_N);
}

TEST(reduction_algoritm, sum_helper) {
	const it::infinite_sequence_generator<int> seq;

	const uint64 N     = 100;
	const uint64 SUM_N = N * (N + 1) / 2;

	const int sum  = seq | it::take(N + 1) | algo::sum<int>();
	const int sum2 = algo::sum(seq | it::take(N + 1));

	ASSERT_EQ(sum, SUM_N);
	ASSERT_EQ(sum2, SUM_N);
}

TEST(reduction_algoritm, stupit_count) {

	const it::infinite_sequence_generator<int> seq;

	const uint64 N = 100;

	uint64 count_1 = seq | it::take(N + 1) | it::map([](int) -> uint64 { return 1ULL; })
				   | algo::reduce(uint64(0), [](uint64 a, uint64 b) { return a + b; });

	uint64 count_2 = seq | it::take(N + 1) | algo::count();

	ASSERT_EQ(count_1, count_2);
}

constexpr int RANGE_MIN = 4900;
constexpr int RANGE_MAX = 4964;
constexpr int MAGIC_5   = 5;

constexpr uint64 count_pairs_naive(const int *arr, uint64 len) {
	uint64 count = 0;

	for (uint64 i = 0; i < len; i++) {
		for (uint64 j = 0; j <= i; j++) {
			const int larger  = it::max(arr[i], arr[j]);
			const int smaller = it::min(arr[i], arr[j]);
			const int val     = (larger + MAGIC_5) * smaller;
			if (val >= RANGE_MIN && val <= RANGE_MAX) { count++; }
		}
	}
	return count;
}

uint64 count_pairs(int *arr, uint64 len) {
	const auto it = it::iterator(arr, len);


	return algo::count(
			it::filter(it::filter(it::map(it::filter(it::cross_product(it, it),
													 [](auto p) { return p.first >= p.second; }),
										  [](auto p) {
											  struct pair_t {
												  int first;
												  int second;
											  };
											  return pair_t{p.first + MAGIC_5, p.second};
										  }),
								  [](auto p) { return p.first * p.second >= RANGE_MIN; }),
					   [](auto p) { return p.first * p.second <= RANGE_MAX; }));
}


constexpr uint64 count_pairs_better(int *arr, uint64 len) {
	const auto it = it::iterator(arr, len);
	struct pair_t {
		int first;
		int second;
	};

	return it | it::unordered_pairs() | it::map([](auto p) {
			   return pair_t{it::max(p.first, p.second) + MAGIC_5, it::min(p.first, p.second)};
		   })
		 | it::filter([](auto p) { return p.first * p.second >= RANGE_MIN; })
		 | it::filter([](auto p) { return p.first * p.second <= RANGE_MAX; }) | algo::count();
}

std::vector<int> shuffle(std::vector<int> v) {
	std::random_device rd;
	std::mt19937       g(rd());

	std::ranges::shuffle(v.begin(), v.end(), g);

	return v;
}


TEST(count_algorithm, fancy_count) {
	const uint64     len = 1000;
	std::vector<int> v;

	for (uint64 i = 0; i < len; i++) { v.push_back(int(i)); }

	v = shuffle(v);

	const uint64 count = count_pairs_naive(v.data(), v.size());

	ASSERT_EQ(count_pairs(v.data(), v.size()), count);
	ASSERT_EQ(count_pairs_better(v.data(), v.size()), count);
}

TEST(count_algorithm, unordered_pairs) {
	const int    N     = 10;
	const auto   sq    = it::sequence_generator<int>(0, N);
	const uint64 SUM_N = N * (N + 1) / 2;

	auto it = sq | it::unordered_pairs();

	ASSERT_EQ(algo::count(it), SUM_N);
	for (uint64 i = 0; i < SUM_N && it.has_next(); i++) {
		ASSERT_EQ(algo::count(it), SUM_N - i);
		++it;
	}
}

TEST(count_algorithm, cross_product) {
	const uint64 N1    = 10;
	const uint64 N2    = 13;
	const auto   sq_1  = it::sequence_generator<int>(0, N1);
	const auto   sq_2  = it::sequence_generator<int>(0, N2);
	const uint64 SUM_N = N1 * N2;

	auto it = it::cross_product(sq_1, sq_2);

	ASSERT_EQ(algo::count(it), SUM_N);
	for (uint64 i = 0; i < SUM_N && it.has_next(); i++) {
		ASSERT_EQ(algo::count(it), SUM_N - i);
		++it;
	}
}

TEST(zip, zero_sequence) {

	const uint64 N1   = 10;
	const uint64 N2   = 13;
	const auto   sq_1 = it::sequence_generator<int>(0, N1);
	const auto   sq_2 = it::sequence_generator<int>(0, N2);

	auto it = it::map(it::zip(sq_1, sq_2), [](auto p) { return p.first - p.second; });

	ASSERT_EQ(algo::count(it), N1);

	int c = 0;
	while (it.has_next()) {
		ASSERT_EQ(*it, 0);
		c++;
		++it;
	}
	ASSERT_EQ(c, N1);
}

TEST(to_array, vector) {
	int              j = 1;
	std::vector<int> v{++j, ++j, ++j, ++j, ++j};

	const auto it = it::iterator(v.data(), v.size());

	const algo::DynamicArray auto arr = algo::to_array<std::vector<int>>(it);

	ASSERT_EQ(arr.size(), v.size());
	for (uint64 i = 0; i < arr.size(); i++) { ASSERT_EQ(arr[i], v[i]); }
}


TEST(append, vector) {
	int              j = 1;
	std::vector<int> v_full{++j, ++j, ++j, ++j, ++j, ++j, ++j, ++j, ++j, ++j};
	j = 1;
	std::vector<int> v_1{++j, ++j, ++j, ++j, ++j};
	const int        element = ++j;
	std::vector<int> v_2{++j, ++j, ++j, ++j};

	auto it_1 = it::iterator(v_1.data(), v_1.size());
	auto it_2 = it::single_element_iterator(element);
	auto it_3 = it::iterator(v_2.data(), v_2.size());

	auto full = algo::to_array<std::vector<int>>(it::append(it::append(it_1, it_2), it_3));

	ASSERT_EQ(full.size(), v_full.size());
	for (uint64 i = 0; i < full.size(); i++) { ASSERT_EQ(full[i], v_full[i]); }
}

TEST(cached_iterator, cache_correct) {
	const uint64 len = 1000;
	int          arr[len]{};

	for (uint64 i = 0; i < len; i++) { arr[i] = int(i); }

	auto cached_it = it::iterator(arr, len) | it::caching_iterator();


	ASSERT_EQ(*cached_it, 0);
	ASSERT_EQ(*cached_it, 0);
	ASSERT_EQ(*cached_it, 0);

	uint64 number_to_skip = 10;

	auto skip_it = cached_it | it::skip(number_to_skip);


	ASSERT_EQ(*skip_it, number_to_skip);
	ASSERT_EQ(*skip_it, number_to_skip);
	ASSERT_EQ(*skip_it, number_to_skip);
}

TEST(cpp_iterator_adapter, simple_iteratr) {

	const uint64 len = 1000;
	int          arr[len]{};

	for (uint64 i = 0; i < len; i++) { arr[i] = int(i); }

	auto cpp_it = it::iterator(arr, len) | it::filter([](int e) { return e % 5 == 3; });

	for (int i: cpp_it) { ASSERT_EQ(i % 5, 3); }
}

TEST(reversing_iterators, sequence) {
	const uint64 len = 1000;
	int          arr[len]{};

	for (uint64 i = 0; i < len; i++) { arr[i] = int(i); }

	auto it = it::iterator(arr, len) | it::reverse();

	for (int i = len - 1; i >= 0; i--) {
		ASSERT_EQ(*it, i);
		++it;
	}
}

TEST(reversing_iterators, filter) {
	const int len = 1000;

	auto it = it::sequence_generator<int>(0, len)
			| it::filter([](uint64 element) { return element % 2 == 0; }) | it::reverse();

	for (int i = len - 2; i >= 0; i -= 2) {
		ASSERT_EQ(*it, i);
		++it;
	}
}


template<uint64 size>
constexpr auto successors(array<uint8, size> conf) {
	return it::sequence_generator<uint8>(0, 8)
		 | it::map([conf](uint8 i) { return i + conf; });
}

constexpr bool threats(int row1, int row2, int diag) {
	const int diff = row1 - row2;
	return diff == 0 || diff == diag || diff == -diag;
}

template<uint64 size>
constexpr bool legal(const array<uint8, size> conf) {
	if constexpr (conf.length == 0) { return true; }
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
constexpr auto operator|(CI it, flatten_) {
	return flatten(it);
}

template<uint64 size>
auto backtrack(array<uint8, size> conf) {
	if constexpr (conf.length == 8) {
		return std::vector{conf};
	} else {
		return successors(conf)             //
			 | it::filter(legal<size + 1>)  //
			 | it::map(backtrack<size + 1>) //
			 | flatten();
	}
}


TEST(backracking, queen) {
	auto solutions = backtrack(array<uint8, 0>{});
	ASSERT_EQ(solutions.size(), 92);
	const auto v_0  = array<uint8, 8>{3, 1, 6, 2, 5, 7, 4, 0};
	const auto v_1  = array<uint8, 8>{4, 1, 3, 6, 2, 7, 5, 0};
	const auto v_90 = array<uint8, 8>{3, 6, 4, 1, 5, 0, 2, 7};
	const auto v_91 = array<uint8, 8>{4, 6, 1, 5, 2, 0, 3, 7};
	ASSERT_EQ(solutions[0], v_0);
	ASSERT_EQ(solutions[1], v_1);
	ASSERT_EQ(solutions[90], v_90);
	ASSERT_EQ(solutions[91], v_91);
}
constexpr auto successors_2(array_f conf) {
	return it::sequence_generator<uint8>(0, 8) | it::map([conf](int8 i) { return i + conf; });
}

constexpr bool legal_2(const array_f conf) {
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

auto backtrack_2(array_f conf) {
	if (conf.size == 8) {
		return std::vector{conf};
	} else {
		return successors_2(conf)   //
			 | it::filter(legal_2)  //
			 | it::map(backtrack_2) //
			 | flatten_2();
	}
}


TEST(backracking, queen2) {
	auto solutions = backtrack_2(array_f{});
	ASSERT_EQ(solutions.size(), 92);
	const auto v_0  = array_f{3, 1, 6, 2, 5, 7, 4, 0};
	const auto v_1  = array_f{4, 1, 3, 6, 2, 7, 5, 0};
	const auto v_90 = array_f{3, 6, 4, 1, 5, 0, 2, 7};
	const auto v_91 = array_f{4, 6, 1, 5, 2, 0, 3, 7};
	ASSERT_EQ(solutions[0], v_0);
	ASSERT_EQ(solutions[1], v_1);
	ASSERT_EQ(solutions[90], v_90);
	ASSERT_EQ(solutions[91], v_91);
}

int main(int argc, char **argv) {


	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
