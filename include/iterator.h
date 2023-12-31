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
	struct function_traits_ret : function_traits_ret<decltype(&T::operator())> {};
	template<typename ClassType, typename ReturnType, typename... Args>
	struct function_traits_ret<ReturnType (ClassType::*)(Args...) const> {
		using return_type = ReturnType;
	};
	template<typename T>
	using return_value_t = typename function_traits_ret<T>::return_type;

	template<typename T>
	struct function_traits_1 : function_traits_1<decltype(&T::operator())> {};
	template<typename ClassType, typename ReturnType, typename... Args>
	struct function_traits_1<ReturnType (ClassType::*)(Args...) const> {
		static_assert(sizeof...(Args) >= 1, "The function must have at least two parameters.");

		using first_argument_type = typename template_element<0, Args...>::type;
	};

	template<typename T>
	struct function_traits_2 : function_traits_2<decltype(&T::operator())> {};
	template<typename ClassType, typename ReturnType, typename... Args>
	struct function_traits_2<ReturnType (ClassType::*)(Args...) const> {
		static_assert(sizeof...(Args) >= 2, "The function must have at least two parameters.");

		using first_argument_type = typename template_element<0, Args...>::type;

		using second_argument_type = typename template_element<1, Args...>::type;
	};
	template<typename T>
	using second_argument_t = typename function_traits_2<T>::second_argument_type;
	template<typename T>
	using first_argument_t = typename function_traits_1<T>::first_argument_type;

} // namespace algo
namespace it {

	template<class T>
	constexpr T max(T a, T b) {
		return a > b ? a : b;
	}

	template<class T>
	constexpr T min(T a, T b) {
		return a > b ? b : a;
	}

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

	// Helper functions to mimic std::__declval and __is_same
	template<typename T>
	T &&_declare_val(); // No definition needed, it's never called.


	template<typename From, typename To>
	struct is_convertible_helper {
	private:
		// test() tries to call the helper function and checks if the return type is To
		static void test(...);
		static To   test(To);

	public:
		// The value will be true if and only if From can be implicitly converted to To
		static const bool value = !is_same_v<decltype(test(_declare_val<From>())), void>;
	};

	template<typename From, typename To>
	concept ConvertibleTo = is_convertible_helper<From, To>::value;

	// Remove Reference
	template<typename T>
	struct remove_reference {
		using type = T;
	};

	template<typename T>
	struct remove_reference<T &> {
		using type = T;
	};

	template<typename T>
	struct remove_reference<T &&> {
		using type = T;
	};

	// Helper alias template for remove_reference
	template<typename T>
	using remove_reference_t = typename remove_reference<T>::type;

	// Add Pointer
	template<typename T>
	struct add_pointer {
		using type = T *;
	};

	// Helper alias template for add_pointer
	template<typename T>
	using add_pointer_t = typename add_pointer<T>::type;

	// Combine them to mimic std::add_pointer_t<std::remove_reference_t<T>>
	template<typename T>
	using add_pointer_to_removed_reference_t = add_pointer_t<remove_reference_t<T>>;

	struct negate {};

	bool operator|(bool b, negate) { return !b; }

	// type_if_t<condition, type_if_true, type_if_false>
	template<bool condition, typename T, typename F>
	struct type_if {
		using type = T;
	};

	template<typename T, typename F>
	struct type_if<false, T, F> {
		using type = F;
	};

	template<bool condition, typename T, typename F>
	using type_if_t = typename type_if<condition, T, F>::type;


	template<typename T>
	concept CustomIterator = requires(const T it, T other, bool b, T::value_type t) {
		b = it.has_next();
		{ *it } -> ConvertibleTo<typename T::value_type>;
		++other;
	};

	template<typename T>
	concept CopyableIterator
			= CustomIterator<T> && requires(const T it, T other) { other = T(it); };

	template<typename T>
	concept CountingIterator
			= CustomIterator<T> && requires(const T it, uint64 count) { count = it.count(); };

	template<typename T>
	concept ReverseIterator = CustomIterator<T> && requires(const T it) {
		{ it.reverse() } -> same_as<typename T::reverse_t>;
	};

	template<class T>
	struct cpp_iterator_adapter {

		struct sentinel {};

		constexpr cpp_iterator_adapter() = default;

		// Assuming T is the derived type
		T                     &derived() { return *static_cast<T *>(*this); }
		[[nodiscard]] const T &derived() const { return static_cast<const T &>(*this); }

		using iterator_type = T;
		using sentinel_type = sentinel;

		// begin simply returns a reference to the derived iterator
		[[nodiscard]] iterator_type begin() const { return derived(); }

		// end returns a default-constructed sentinel object
		[[nodiscard]] sentinel_type end() const { return {}; }

		// Compare the derived iterator with sentinel to check for the end
		friend bool operator==(const cpp_iterator_adapter &a, const sentinel &) {
			return !a.derived().has_next();
		}
		// Provide increment, dereference, and other iterator functionality
		// as needed by forwarding to the derived iterator.
	};

	enum class IteratorType {
		Forward,
		Reverse,
	};

	constexpr IteratorType operator!(IteratorType type) {
		using enum it::IteratorType;
		if (type == Forward) { return Reverse; }
		return Forward;
	}

	template<class T, IteratorType direction = IteratorType::Forward>
	struct iterator : public cpp_iterator_adapter<iterator<T, direction>> {
		using value_type [[maybe_unused]] = TypeMapper<T>::Type;

		T *_begin = nullptr;
		T *_end   = nullptr;

		using pointer   = add_pointer_to_removed_reference_t<value_type>;
		using reference = value_type &;

		constexpr iterator() = default;

		constexpr iterator(T *begin, T *end) : _begin(begin), _end(end) {}

		constexpr iterator(T *begin, uint64 size) : _begin(begin), _end(begin + size) {}

		constexpr void operator++() {
			if constexpr (direction == IteratorType::Forward) { _begin++; }
			if constexpr (direction == IteratorType::Reverse) { _end--; }
		}
		constexpr void operator--() {
			if constexpr (direction == IteratorType::Forward) { _begin--; }
			if constexpr (direction == IteratorType::Reverse) { _end++; }
		}

		constexpr reference operator*() const {
			if constexpr (direction == IteratorType::Forward) { return *_begin; }
			if constexpr (direction == IteratorType::Reverse) { return *_end; }
		}

		[[nodiscard]] constexpr bool has_next() const { return _begin != _end; }

		[[nodiscard]] constexpr uint64 count() const { return _end - _begin; }


		// Equality comparison (needed for Regular concept)
		friend bool operator==(const iterator &a, const iterator &b) {

			if constexpr (direction == IteratorType::Forward) { return a._begin == b._begin; }
			if constexpr (direction == IteratorType::Reverse) { return a._end == b._end; }
		}

		using reverse_t = iterator<T, !direction>;
		[[nodiscard]] constexpr iterator<T, !direction> reverse() const {
			if constexpr (direction == IteratorType::Forward) {
				return iterator<T, !direction>(_begin - 1, _end - 1);
			}
			if constexpr (direction == IteratorType::Reverse) {
				return iterator<T, !direction>(_begin + 1, _end + 1);
			}
		}
	};

	template<class T>
	struct single_element_iterator : cpp_iterator_adapter<single_element_iterator<T>> {
		using value_type [[maybe_unused]] = T;

		T    element;
		bool iterated = false;


		explicit constexpr single_element_iterator(T element) : element(element) {}


		constexpr void operator++() { iterated = true; }

		constexpr T &operator*() { return element; }

		constexpr value_type operator*() const { return element; }

		[[nodiscard]] constexpr bool has_next() const { return !iterated; }

		[[nodiscard]] constexpr uint64 count() const { return iterated ? 0 : 1; }

		using reverse_t = single_element_iterator<T>;
		[[nodiscard]] constexpr single_element_iterator<T> reverse() const { return *this; }
	};

	struct c_string_iterator : cpp_iterator_adapter<c_string_iterator> {
		using value_type [[maybe_unused]] = char;

		const char *sting;

		explicit constexpr c_string_iterator(const char *str) : sting(str){};

		constexpr void operator++() { sting++; }

		constexpr char operator*() const { return *sting; }

		[[nodiscard]] constexpr bool has_next() const { return *sting != '\0'; }
	};

	template<class T, IteratorType direction = IteratorType::Forward>
	struct sequence_generator : cpp_iterator_adapter<sequence_generator<T, direction>> {
		using value_type [[maybe_unused]] = T;

		T _begin;
		T _end;

		constexpr sequence_generator(T begin, T end) : _begin(begin), _end(end) {}

		constexpr void operator++() {
			if constexpr (direction == IteratorType::Forward) { _begin++; }
			if constexpr (direction == IteratorType::Reverse) { _end--; }
		}

		constexpr T operator*() const {
			if constexpr (direction == IteratorType::Forward) { return _begin; }
			if constexpr (direction == IteratorType::Reverse) { return _end; }
		}

		[[nodiscard]] constexpr bool has_next() const { return _begin != _end; }

		[[nodiscard]] constexpr uint64 count() const { return _end - _begin; }

		using reverse_t = sequence_generator<T, !direction>;
		[[nodiscard]] constexpr sequence_generator<T, !direction> reverse() const {
			if constexpr (direction == IteratorType::Forward) {
				return sequence_generator<T, !direction>(_begin - 1, _end - 1);
			}
			if constexpr (direction == IteratorType::Reverse) {
				return sequence_generator<T, !direction>(_begin + 1, _end + 1);
			}
		}
	};

	template<class T>
	struct infinite_sequence_generator : cpp_iterator_adapter<infinite_sequence_generator<T>> {

		using value_type [[maybe_unused]] = T;

		T _begin = 0;

		explicit constexpr infinite_sequence_generator(T begin) : _begin(begin) {}
		explicit constexpr infinite_sequence_generator() = default;

		constexpr void operator++() { _begin++; }

		constexpr T operator*() const { return _begin; }

		[[nodiscard]] constexpr bool has_next() const { return true; }

		[[nodiscard]] constexpr uint64 count() const { return ~0UL; }
	};

	template<typename T, typename ARG>
	concept MapFunction = requires(T t, T other, ARG arg) { t(arg); };

	template<CustomIterator CI, MapFunction<typename CI::value_type> FN, class T>
	struct _i_MapIterator : cpp_iterator_adapter<_i_MapIterator<CI, FN, T>> {
		using value_type [[maybe_unused]] = T;

		CI _it;
		FN _lambda;

		explicit constexpr _i_MapIterator(CI it, FN lambda) : _it(it), _lambda(lambda) {}

		constexpr void operator++() { ++_it; }

		constexpr value_type operator*() const { return _lambda(*_it); }


		[[nodiscard]] constexpr bool has_next() const { return _it.has_next(); }

		[[nodiscard]] constexpr uint64 count() const
			requires CountingIterator<CI>
		{
			return _it.count();
		}

		template<CustomIterator _i_CI>
		struct reverse_t_s;

		template<CustomIterator _i_CI>
			requires ReverseIterator<_i_CI>
		struct reverse_t_s<_i_CI> {
			using type = _i_MapIterator<typename _i_CI::reverse_t, FN, T>;
		};

		template<CustomIterator _i_CI>
			requires(!ReverseIterator<_i_CI>)
		struct reverse_t_s<_i_CI> {
			using type = void;
		};

		using reverse_t = typename reverse_t_s<CI>::type;

		[[nodiscard]] constexpr auto reverse() const
			requires ReverseIterator<CI>
		{
			return map<typename CI::reverse_t, FN>(_it.reverse(), _lambda);
		}
	};

	template<CustomIterator CI, MapFunction<typename CI::value_type> FN>
	constexpr auto map(CI it, FN lambda) {
		static_assert(CustomIterator<_i_MapIterator<CI, FN, decltype(lambda(*it))>>, "help");
		return _i_MapIterator<CI, FN, decltype(lambda(*it))>(it, lambda);
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
	struct _i_FilterIterator : cpp_iterator_adapter<_i_FilterIterator<CI, FN>> {
		using value_type [[maybe_unused]] = CI::value_type;

		CI _it;
		FN _lambda;

		constexpr _i_FilterIterator(CI it, FN lambda) : _it(it), _lambda(lambda) {
			while (_it.has_next() && !_lambda(*_it)) { ++_it; }
		}

		constexpr void operator++() {
			++_it;
			while (_it.has_next() && !_lambda(*_it)) { ++_it; }
		}

		constexpr value_type operator*() const { return *_it; }

		[[nodiscard]] constexpr bool has_next() const { return _it.has_next(); }

		template<CustomIterator _i_CI>
		struct reverse_t_s;

		template<CustomIterator _i_CI>
			requires ReverseIterator<_i_CI>
		struct reverse_t_s<_i_CI> {
			using type = _i_FilterIterator<typename _i_CI::reverse_t, FN>;
		};

		template<CustomIterator _i_CI>
			requires(!ReverseIterator<_i_CI>)
		struct reverse_t_s<_i_CI> {
			using type = void;
		};

		using reverse_t = typename reverse_t_s<CI>::type;

		[[nodiscard]] constexpr auto reverse() const
			requires ReverseIterator<CI>
		{
			return filter<typename CI::reverse_t, FN>(_it.reverse(), _lambda);
		}
	};

	template<CustomIterator CI, PredicateFunction<typename CI::value_type> FN>
	constexpr auto filter(CI it, FN lambda) {

		return _i_FilterIterator(it, lambda);
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
	constexpr auto operator|(CI it, filter_<FN> lambda) {
		return filter(it, lambda._lambda);
	}

	template<CustomIterator CI>
	constexpr auto take(CI it, uint64 n) {
		using T = CI::value_type;

		struct _ : cpp_iterator_adapter<_> {
			using value_type [[maybe_unused]] = TypeMapper<T>::Type;

			CI     _it;
			uint64 _n;

			constexpr _(CI it, uint64 n) : _it(it), _n(n) {}

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

		return _(it, n);
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

		struct _ : cpp_iterator_adapter<_> {
			using value_type [[maybe_unused]] = pair_t;

			CI_1 _it_1;
			CI_2 _it_2;

			constexpr _(CI_1 it_1, CI_2 it_2) : _it_1(it_1), _it_2(it_2) {}

			constexpr void operator++() {
				++_it_1;
				++_it_2;
			}

			constexpr pair_t operator*() const { return {*_it_1, *_it_2}; }

			[[nodiscard]] constexpr bool has_next() const {
				return _it_1.has_next() && _it_2.has_next();
			}

			[[nodiscard]] constexpr uint64 count() const
				requires CountingIterator<CI_1> && CountingIterator<CI_2>
			{
				return min(_it_1.count(), _it_2.count());
			}
		};

		return _(it_1, it_2);
	}

	template<CopyableIterator CI_2>
	struct zip_ {
		CI_2 _it_2;
		constexpr explicit zip_(CI_2 it_2) : _it_2(it_2) {}
	};
	template<CopyableIterator CI_2>
	constexpr auto zip(CI_2 it_2) {
		return zip_<CI_2>(it_2);
	}
	template<CustomIterator CI_1, CopyableIterator CI_2>
	constexpr auto operator|(CI_1 it_1, zip_<CI_2> it_2) {
		return zip(it_1, it_2._it_2);
	}

	template<CustomIterator CI_1, CustomIterator CI_2>
	constexpr auto append(CI_1 it_1, CI_2 it_2) {
		using T_1 = CI_1::value_type;
		using T_2 = CI_2::value_type;
		static_assert(is_same_v<T_1, T_2>);

		struct _ : cpp_iterator_adapter<_> {
			using value_type [[maybe_unused]] = TypeMapper<T_1>::Type;

			CI_1 _it_1;
			CI_2 _it_2;
			bool iterator_in_use = false; /* false => 1, true => 2 */

			constexpr _(CI_1 it_1, CI_2 it_2) : _it_1(it_1), _it_2(it_2) {}

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

			[[nodiscard]] constexpr bool has_next() const {
				return !iterator_in_use || _it_2.has_next();
			}

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

	template<CopyableIterator CI_1, CopyableIterator CI_2>
	constexpr auto cross_product(CI_1 it_1, CI_2 it_2) {
		using T_1 = CI_1::value_type;
		using T_2 = CI_2::value_type;
		struct pair_t {
			TypeMapper<T_1>::Type first;
			TypeMapper<T_2>::Type second;
		};

		struct _ : cpp_iterator_adapter<_> {
			using value_type [[maybe_unused]] = pair_t;

			CI_1 _it_1;
			CI_1 current_it_1;
			CI_2 _it_2;
			T_2  it_value_cache; // it may be expensive to call *_it_2;

			constexpr _(CI_1 it_1, CI_2 it_2) : _it_1(it_1), current_it_1(it_1), _it_2(it_2) {
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

	template<CopyableIterator CI>
	constexpr auto unordered_pairs(CI it) {
		using T = CI::value_type;
		struct pair_t {
			TypeMapper<T>::Type first;
			TypeMapper<T>::Type second;
		};

		struct _ : cpp_iterator_adapter<_> {
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

	template<ReverseIterator CI>
	constexpr auto reverse(CI it) {
		return it.reverse();
	}
	struct reverse_ {};
	constexpr auto reverse() { return reverse_{}; }
	template<ReverseIterator CI>
	constexpr auto operator|(CI it, reverse_) {
		return reverse(it);
	}

} // namespace it

namespace algo {
	template<class T, class OUT>
	concept FoldFunction_I = requires(T t, second_argument_t<T> in, OUT out) { out = t(out, in); };

	template<it::CustomIterator CI, class OUT, FoldFunction_I<OUT> L>
	constexpr OUT reduce(CI it, L func, OUT initial) {
		static_assert(it::is_same_v<typename CI::value_type, first_argument_t<L>>,
					  "The iterator value type must be the same as the first "
					  "argument of the function.");
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
					  "The iterator value type must be the same as the first "
					  "argument of the function.");
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
	constexpr bool any(CI it)
		requires it::is_same_v<typename CI::value_type, bool>
	{
		while (it.has_next()) {
			if (*it) { return true; }
			++it;
		}
		return false;
	}
	struct any_ {};
	constexpr auto any() { return any_{}; }
	template<it::CustomIterator CI>
	constexpr auto operator|(CI it, any_) {
		return any(it);
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
		struct _ : cpp_iterator_adapter<_> {
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
		struct _ : cpp_iterator_adapter<_> {
			using value_type [[maybe_unused]] = TypeMapper<T>::Type;

			CI _it;
			T  cache;

			explicit constexpr _(CI it) : _it(it) {
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
		return _(it);
	}
	struct caching_iterator_ {};
	constexpr auto caching_iterator() { return caching_iterator_{}; }
	template<CustomIterator CI>
	constexpr auto operator|(CI it, caching_iterator_) {
		return caching_iterator(it);
	}
} // namespace it


#endif //TINY_CPP_ITERATOR_H
