//
// Created by af on 12/10/23.
//

#ifndef TINY_CPP_ITERATOR_H
#define TINY_CPP_ITERATOR_H

#include "c_int_types.h"

#if !defined(NO_STD)
#include <tuple>
#include <type_traits>
#endif

namespace it {

	constexpr int max(int a, int b) { return a > b ? a : b; }

	constexpr int min(int a, int b) { return a > b ? b : a; }

	template<typename T, typename U>
	struct is_same {
		static constexpr bool value = false;
	};

	template<typename T>
	struct is_same<T, T> {
		static constexpr bool value = true;
	};

	template<typename T_1, typename T_2>
	concept same_as = is_same<T_1, T_2>::value && is_same<T_2, T_1>::value;


	template<typename T, typename U>
	inline constexpr bool is_same_v = is_same<T, U>::value;

#if defined(NO_STD)
	template<typename T>
	concept TriviallyCopyable = __is_trivially_copyable(T);
#else
	template<typename T>
	concept TriviallyCopyable = std::is_trivially_copyable_v<T>;
#endif


	template<typename T, bool = TriviallyCopyable<T>>
	struct TypeMapper;

	template<typename T>
	struct TypeMapper<T, true> {
		using Type = T;
	};

	template<typename T>
	struct TypeMapper<T, false> {
		using Type = const T &;
	};

	template<typename T>
	concept CustomIterator = requires(const T it, T other, bool b, T::value_type t) {
		b = it.has_next();
		{ *it } -> same_as<typename T::value_type>;
		++other;
		other = T(it);
	};

	template<typename T>
	concept CountingIterator = CustomIterator<T> && requires(const T it, uint64 count) { count = it.count(); };

	template<typename T>
	concept ReverseIterator = CustomIterator<T> && requires(const T it, T r) { r = it.reverse(); };

	template<class T>
	struct iterator {
		using value_type [[maybe_unused]] = TypeMapper<T>::Type;

		T *begin;
		T *end;

		constexpr iterator() : begin(nullptr), end(nullptr) {}

		constexpr iterator(T *begin, T *end) : begin(begin), end(end) {}

		constexpr iterator(T *begin, uint64 size) : begin(begin), end(begin + size) {}

		constexpr void operator++() { begin++; }

		constexpr T &operator*() { return *begin; }

		constexpr value_type operator*() const { return *begin; }

		[[nodiscard]] constexpr bool has_next() const { return begin != end; }

		[[nodiscard]] constexpr uint64 count() const { return end - begin; }
	};

	template<class T>
	struct single_element_iterator {
		using value_type [[maybe_unused]] = T;

		T    element;
		bool iterated = false;


		explicit constexpr single_element_iterator(T element) : element(element) {}


		constexpr void operator++() { iterated = true; }

		constexpr T &operator*() { return element; }

		constexpr value_type operator*() const { return element; }

		[[nodiscard]] constexpr bool has_next() const { return !iterated; }

		[[nodiscard]] constexpr uint64 count() const { return iterated ? 0 : 1; }
	};

	struct c_string_iterator {
		using value_type [[maybe_unused]] = char;

		const char *sting;

		explicit constexpr c_string_iterator(const char *str) : sting(str){};

		constexpr void operator++() { sting++; }

		constexpr char operator*() const { return *sting; }

		[[nodiscard]] constexpr bool has_next() const { return *sting != '\0'; }
	};

	template<class T>
	struct sequence_generator {
		using value_type [[maybe_unused]] = T;

		T begin;
		T end;

		constexpr void operator++() { begin++; }

		constexpr T operator*() const { return begin; }

		[[nodiscard]] constexpr bool has_next() const { return begin != end; }

		[[nodiscard]] constexpr uint64 count() const { return end - begin; }
	};

	template<class T>
	struct infinite_sequence_generator {

		using value_type [[maybe_unused]] = T;

		T begin = 0;

		constexpr void operator++() { begin++; }

		constexpr T operator*() const { return begin; }

		[[nodiscard]] constexpr bool has_next() const { return true; }

		[[nodiscard]] constexpr uint64 count() const { return ~0UL; }
	};

	template<typename T, typename ARG>
	concept MapFunction = requires(T t, ARG arg) { t(arg); };

	template<CustomIterator CI, MapFunction<typename CI::value_type> FN>
	constexpr auto map(CI it, FN lambda) {

		using T [[maybe_unused]] = decltype(lambda(*it));

		struct _ {
			using value_type [[maybe_unused]] = TypeMapper<T>::Type;

			CI _it;
			FN _lambda;

			constexpr void operator++() { ++_it; }

			constexpr value_type operator*() const { return _lambda(*_it); }


			[[nodiscard]] constexpr bool has_next() const { return _it.has_next(); }

			//#if !defined(__clang__) || defined(__GNUC__)
			[[nodiscard]] constexpr uint64 count() const
				requires CountingIterator<CI>
			{
				if constexpr (CountingIterator<CI>) { return _it.count(); }
				return 0;
			}
			//#endif
		};

		return _{it, lambda};
	}
	template<typename FN>
	struct map_ {
		FN _lambda;
		constexpr explicit map_(FN lambda) : _lambda(lambda) {}
	};
	template<typename FN>
	constexpr auto map(FN lambda) {
		return map_<FN>(lambda);
	}
	template<CustomIterator CI, MapFunction<typename CI::value_type> FN>
	constexpr auto operator|(CI it, map_<FN> _lambda) {
		return map(it, _lambda._lambda);
	}


	template<typename T, typename ARG>
	concept PredicateFunction = requires(T it, ARG arg) {
		{ it(arg) } -> same_as<bool>;
	};

	template<CustomIterator CI, PredicateFunction<typename CI::value_type> FN>
	constexpr auto filter(CI it, FN lambda) {
		using T = CI::value_type;

		struct _ {
			using value_type [[maybe_unused]] = TypeMapper<T>::Type;

			CI _it;
			FN _lambda;

			constexpr _(decltype(it) it, decltype(lambda) lambda) : _it(it), _lambda(lambda) {
				while (_it.has_next() && !_lambda(*_it)) { ++_it; }
			}

			constexpr void operator++() {
				++_it;
				while (_it.has_next() && !_lambda(*_it)) { ++_it; }
			}

			constexpr value_type operator*() const { return *_it; }

			[[nodiscard]] constexpr bool has_next() const { return _it.has_next(); }
		};

		return _{it, lambda};
	}
	template<typename FN>
	struct filter_ {
		FN _lambda;
		constexpr explicit filter_(FN lambda) : _lambda(lambda) {}
	};
	template<typename FN>
	constexpr auto filter(FN lambda) {
		return filter_<FN>(lambda);
	}
	template<CustomIterator CI, PredicateFunction<typename CI::value_type> FN>
	constexpr auto operator|(CI it, filter_<FN> _lambda) {
		return filter(it, _lambda._lambda);
	}

	template<CustomIterator CI>
	constexpr auto take(CI it, uint64 n) {
		using T = CI::value_type;

		struct _ {
			using value_type [[maybe_unused]] = TypeMapper<T>::Type;

			CI     _it;
			uint64 _n;

			constexpr void operator++() {
				++_it;
				--_n;
			}

			constexpr value_type operator*() const { return *_it; }

			[[nodiscard]] constexpr bool has_next() const { return _it.has_next() && _n > 0; }

			[[nodiscard]] constexpr uint64 count() const
				requires CountingIterator<CI>
			{
				return _n;
			}
		};

		return _{it, n};
	}
	struct take_ {
		uint64 _n;
	};
	constexpr auto take(uint64 n) { return take_{n}; }
	template<CustomIterator CI>
	constexpr auto operator|(CI it, take_ n) {
		return take(it, n._n);
	}


	/*
	 * Maybe contrary to what's expected this library doesn't do full lazy evaluation.
	 * If the code will never be executed, it's the compilers job to dead code eliminate it.
	 * Or the programmers job to not write it.
	 *
	 * This function is a prime example. It's not lazy.
	 */
	template<CustomIterator CI>
	constexpr auto skip(CI it, uint64 n) {
		for (uint64 i = 0; i < n; i++) { ++it; }

		return it;
	}
	struct skip_ {
		uint64 _n;
	};
	constexpr auto skip(uint64 n) { return skip_{n}; }
	template<CustomIterator CI>
	constexpr auto operator|(CI it, skip_ n) {
		return skip(it, n._n);
	}


	template<CustomIterator CI_1, CustomIterator CI_2>
	constexpr auto zip(CI_1 it_1, CI_2 it_2) {
		using T_1 = CI_1::value_type;
		using T_2 = CI_2::value_type;
		struct pair_t {
			TypeMapper<T_1>::Type first;
			TypeMapper<T_2>::Type second;
		};

		struct _ {
			using value_type [[maybe_unused]] = pair_t;

			CI_1 _it_1;
			CI_2 _it_2;

			constexpr void operator++() {
				++_it_1;
				++_it_2;
			}

			constexpr pair_t operator*() const { return {*_it_1, *_it_2}; }

			[[nodiscard]] constexpr bool has_next() const { return _it_1.has_next() && _it_2.has_next(); }

			[[nodiscard]] constexpr uint64 count() const
				requires CountingIterator<CI_1> && CountingIterator<CI_2>
			{
				return min(_it_1.count(), _it_2.count());
			}
		};

		return _{it_1, it_2};
	}

	template<CustomIterator CI_1, CustomIterator CI_2>
	constexpr auto append(CI_1 it_1, CI_2 it_2) {
		using T_1 = CI_1::value_type;
		using T_2 = CI_2::value_type;
		static_assert(is_same_v<T_1, T_2>);

		struct _ {
			using value_type [[maybe_unused]] = TypeMapper<T_1>::Type;

			CI_1 _it_1;
			CI_2 _it_2;
			bool iterator_in_use = false; /* false => 1, true => 2 */

			_(CI_1 it_1, CI_2 it_2) : _it_1(it_1), _it_2(it_2) {}

			constexpr void operator++() {
				if (iterator_in_use) {
					++_it_2;
				} else {
					++_it_1;
					if (!_it_1.has_next()) { iterator_in_use = true; }
				}
			}

			constexpr value_type operator*() const {
				if (iterator_in_use) { return *_it_2; }
				return *_it_1;
			}

			[[nodiscard]] constexpr bool has_next() const { return !iterator_in_use || _it_2.has_next(); }

			[[nodiscard]] constexpr uint64 count() const
				requires CountingIterator<CI_1> && CountingIterator<CI_2>
			{
				return _it_1.count() + _it_2.count();
			}
		};

		return _(it_1, it_2);
	}

	// silence all warnings for this function
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wuninitialized"
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
#endif
	template<typename T>
	T undefined() {
		T _ = _; // NOLINT
		return _;
	}
#ifdef __clang__
#pragma clang diagnostic pop
#else
#pragma GCC diagnostic pop
#endif

	template<CustomIterator CI_1, CustomIterator CI_2>
	constexpr auto cross_product(CI_1 it_1, CI_2 it_2) {
		using T_1 = CI_1::value_type;
		using T_2 = CI_2::value_type;
		struct pair_t {
			TypeMapper<T_1>::Type first;
			TypeMapper<T_2>::Type second;
		};

		struct _ {
			using value_type [[maybe_unused]] = pair_t;

			CI_1 _it_1;
			CI_1 current_it_1;
			CI_2 _it_2;
			T_2  it_value_cache; // it may be expensive to call *_it_2;

			explicit constexpr _(CI_1 it_1, CI_2 it_2) : _it_1(it_1), current_it_1(it_1), _it_2(it_2) {
				if (_it_2.has_next()) {
					it_value_cache = *_it_2;
				} else {
					it_value_cache = undefined<T_2>();
				}
			}

			constexpr void operator++() {
				++current_it_1;
				while (!current_it_1.has_next()) {
					current_it_1 = _it_1;
					++_it_2;
					if (_it_2.has_next()) {
						it_value_cache = *_it_2;
					} else {
						return;
					}
				}
			}

			constexpr pair_t operator*() const { return {*current_it_1, it_value_cache}; }

			[[nodiscard]] constexpr bool has_next() const { return _it_2.has_next(); }

			[[nodiscard]] constexpr uint64 count() const
				requires CountingIterator<CI_1> && CountingIterator<CI_2>
			{
				return _it_1.count() * (_it_2.count() - 1) + current_it_1.count();
			}
		};

		return _(it_1, it_2);
	}

	template<CustomIterator CI>
	constexpr auto unordered_pairs(CI it) {
		using T = CI::value_type;
		struct pair_t {
			TypeMapper<T>::Type first;
			TypeMapper<T>::Type second;
		};

		struct _ {
			using value_type [[maybe_unused]] = pair_t;

			CI _it;
			CI current_it;
			T  it_value_cache; // it may be expensive to call *_it;

			explicit constexpr _(CI it) : _it(it), current_it(it) {
				if (_it.has_next()) {
					it_value_cache = *_it;
				} else {
					it_value_cache = undefined<T>();
				}
			}

			constexpr void operator++() {
				++current_it;
				if (!current_it.has_next()) {
					++_it;
					if (_it.has_next()) { it_value_cache = *_it; }
					current_it = _it;
				}
			}

			constexpr pair_t operator*() const { return {*current_it, it_value_cache}; }

			[[nodiscard]] constexpr bool has_next() const { return _it.has_next(); }

			[[nodiscard]] constexpr uint64 count() const
				requires CountingIterator<CI>
			{
				const uint64 it_count             = _it.count();
				const uint64 iteration_in_current = current_it.count();

				return it_count * (it_count - 1) / 2 + iteration_in_current;
			}
		};

		return _(it);
	}
	struct unordered_pairs_ {};
	constexpr auto unordered_pairs() { return unordered_pairs_{}; }
	template<CustomIterator CI>
	constexpr auto operator|(CI it, unordered_pairs_) {
		return unordered_pairs(it);
	}

} // namespace it


namespace algo {
	template<std::size_t idx, typename... Ts>
	struct template_element;

	template<typename T, typename... Ts>
	struct template_element<0, T, Ts...> {
		using type = T;
	};

	template<std::size_t idx, typename T, typename... Ts>
	struct template_element<idx, T, Ts...> {
		static_assert(idx < sizeof...(Ts) + 1, "Index out of bounds.");
		using type = typename template_element<idx - 1, Ts...>::type;
	};
	template<uint64 idx, typename... Ts>
	using template_element_t = typename template_element<idx, Ts...>::type;

	template<typename T>
	struct function_traits : function_traits<decltype(&T::operator())> {};

	template<typename ClassType, typename ReturnType, typename... Args>
	struct function_traits<ReturnType (ClassType::*)(Args...) const> {
		static_assert(sizeof...(Args) >= 2, "The function must have at least two parameters.");

		using first_argument_type = typename template_element<0, Args...>::type;

		using second_argument_type = typename template_element<1, Args...>::type;
	};
	template<typename T>
	using second_argument_t = typename function_traits<T>::second_argument_type;
	template<typename T>
	using first_argument_t = typename function_traits<T>::first_argument_type;


	template<class T, class OUT>
	concept FoldFunction_I = requires(T t, second_argument_t<T> in, OUT out) { out = t(out, in); };

	template<it::CustomIterator CI, class OUT, FoldFunction_I<OUT> L>
	constexpr OUT reduce(CI it, L func, OUT initial) {
		static_assert(it::is_same_v<typename CI::value_type, first_argument_t<L>>,
					  "The iterator value type must be the same as the first argument of the function.");
		OUT acc = initial;
		while (it.has_next()) {
			acc = func(acc, *it);
			++it;
		}
		return acc;
	}
	template<typename OUT, FoldFunction_I<OUT> L>
	struct reduce_ {
		const OUT _initial;
		const L   _func;
	};
	template<typename OUT, FoldFunction_I<OUT> L>
	constexpr auto reduce(OUT initial, L func) {
		return reduce_<OUT, L>{
				._initial = initial,
				._func    = func,
		};
	}
	template<it::CustomIterator CI, class OUT, FoldFunction_I<OUT> L>
	constexpr auto operator|(CI it, reduce_<OUT, L> initial) {
		static_assert(it::is_same_v<typename CI::value_type, first_argument_t<L>>,
					  "The iterator value type must be the same as the first argument of the function.");
		return reduce(it, initial._func, initial._initial);
	}

	template<typename E>
	constexpr auto sum() {
		return reduce(E(0), [](E a, E b) { return a + b; });
	}

	template<it::CustomIterator CI>
	constexpr auto sum(CI it) {
		using E = typename CI::value_type;
		return reduce(it, [](E a, E b) { return a + b; }, E(0));
	}

	template<it::CustomIterator CI>
	constexpr uint64 count(CI it) {
#if !defined(__clang__) || defined(__GNUC__)
		if constexpr (it::CountingIterator<CI>) { return it.count(); }
#endif
		uint64 acc = 0;
		while (it.has_next()) {
			acc++;
			++it;
		}
		return acc;
	}
	struct count_ {};
	constexpr auto count() { return count_{}; }
	template<it::CustomIterator CI>
	constexpr auto operator|(CI it, count_) {
		return count(it);
	}

	template<typename T>
	concept DynamicArray = requires(T arr, uint64 len, T::value_type val) {
		val = arr[len];
		arr.push_back(val);
	};

	template<DynamicArray T, it::CustomIterator CI>
	constexpr T to_array(CI it) {
		static_assert(it::is_same_v<typename T::value_type, typename CI::value_type>);
		T arr;
		while (it.has_next()) {
			arr.push_back(*it);
			++it;
		}
		return arr;
	}

} // namespace algo

namespace it {
	template<CustomIterator CI>
	constexpr auto counted_wrapper(CI it) {
		using T = CI::value_type;
		struct _ {
			using value_type [[maybe_unused]] = TypeMapper<T>::Type;

			CI _it;

			constexpr void operator++() { ++_it; }

			constexpr value_type operator*() const { return *_it; }

			[[nodiscard]] constexpr bool has_next() const { return _it.has_next(); }

			[[nodiscard]] constexpr uint64 count() const
				requires(!CountingIterator<CI>)
			{
				CI copy = _it;
				return algo::count(copy);
			}

			[[nodiscard]] constexpr uint64 count() const
				requires(CountingIterator<CI>)
			{
				return _it.count();
			}
		};
		return _{it};
	}
	struct counted_wrapper_ {};
	constexpr auto counted_wrapper() { return counted_wrapper_{}; }
	template<CustomIterator CI>
	constexpr auto operator|(CI it, counted_wrapper_) {
		return counted_wrapper(it);
	}

	/*
	 * If *it is expensive but called multiple times, the performance can be improved by caching the value.
	 * This should be valid since *it should be pure.
	 * This iterator caches the value everytime it's incremented.
	 * This may waste resources if only it++ is called without *it,
	 * but *it stays const and tread safe.
	 */
	template<CustomIterator CI>
	constexpr auto caching_iterator(CI it) {
		using T = CI::value_type;
		struct _ {
			using value_type [[maybe_unused]] = TypeMapper<T>::Type;

			CI _it;
			T  cache = undefined<T>();

			explicit constexpr _(CI it) : _it(it) {
				if (_it.has_next()) { cache = *_it; }
			}

			_() {
				if (_it.has_next()) { cache = *_it; }
			}

			constexpr void operator++() {
				++_it;
				if (_it.has_next()) { cache = *_it; }
			}

			constexpr value_type operator*() const { return cache; }

			[[nodiscard]] constexpr bool has_next() const { return _it.has_next(); }


			[[nodiscard]] constexpr uint64 count() const
				requires CountingIterator<CI>
			{
				if constexpr (CountingIterator<CI>) { return _it.count(); }
				return 0;
			}
		};
		return _{it};
	}
	struct caching_iterator_ {};
	constexpr auto caching_iterator() { return caching_iterator_{}; }
	template<CustomIterator CI>
	constexpr auto operator|(CI it, caching_iterator_) {
		return caching_iterator(it);
	}
} // namespace it


#endif //TINY_CPP_ITERATOR_H
