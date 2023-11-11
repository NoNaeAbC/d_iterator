//
// Created by af on 10/11/23.
//

#ifndef D_ITERATOR_ARRAY_H
#define D_ITERATOR_ARRAY_H


#include "iterator.h"

/*
 * This array is a fixed size array.
 * Mostly equivalent to std::array.
 * It behaves like a Haskell list, except for the fact that
 *  - it is not lazy
 *  - it is not immutable
 *  - it doesn't have a variable size
 *  - the head, tail is a copy of the original array
 *
 * Because of the copy, it is not recommended to use this array for large arrays.
 */
template<class T, uint64 size>
struct array {
	T arr[size];

	constexpr T &operator[](uint64 i) { return arr[i]; }
	constexpr T  operator[](uint64 i) const { return arr[i]; }

	static constexpr uint64 length = size;

	[[nodiscard]] constexpr auto head_tail() const
		requires(size > 0)
	{

		struct head_tail_pair {
			T                  head;
			array<T, size - 1> tail;
		};

		array<T, size - 1> tail;
		if constexpr (size > 1) {
			for (uint64 i = 0; i < size - 1; i++) { tail.arr[i] = arr[i + 1]; }
		}

		return head_tail_pair{arr[0], tail};
	}

	explicit array(T e)
		requires(size == 1)
	{
		arr[0] = e;
	}

	array(std::initializer_list<T> l) {
		uint64 i = 0;
		for (const auto &e: l) {
			arr[i] = e;
			i++;
		}
	}

	bool operator==(const array<T, size> &other) const {
		for (uint64 i = 0; i < size; i++) {
			if (arr[i] != other.arr[i]) { return false; }
		}
		return true;
	}

	array() = default;

	// implement copy and copy assignment
	constexpr array(const array<T, size> &other) {
		for (uint64 i = 0; i < size; i++) { arr[i] = other.arr[i]; }
	}
	constexpr array<T, size> &operator=(const array<T, size> &other) {
		for (uint64 i = 0; i < size; i++) { arr[i] = other.arr[i]; }
		return *this;
	}

	struct iterator {
		array<T, size> arr;
		int            index = 0;
		using value_type     = T;

		constexpr iterator(array<T, size> arr, int index) : arr(arr), index(index) {}

		[[nodiscard]] constexpr bool has_next() const { return index != size; }
		constexpr T                  operator*() const { return arr[index]; }
		constexpr void               operator++() { index++; }

		[[nodiscard]] uint64 count() const { return size - index; }
	};

	[[nodiscard]] constexpr auto to_iterator() const { return iterator(*this, 0); }

	~array() = default;
};


template<class T>
struct array<T, 0> {
	static constexpr uint64 length = 0;

	constexpr T &operator[](uint64) { __builtin_unreachable(); }

	struct empty_iterator {
		using value_type = T;
		[[nodiscard]] constexpr bool has_next() const { return false; }
		constexpr T                  operator*() const { return T(); }
		constexpr void               operator++() { /* */
		}

		[[nodiscard]] uint64 count() const { return 0; }
	};

	constexpr auto to_iterator() { return empty_iterator{}; }
};


template<class T, uint64 size>
array<T, size + 1> operator+(T e, array<T, size> arr) {
	auto result = it::undefined<array<T, size + 1>>();

	result.arr[0] = e;
	for (uint64 i = 0; i < size; i++) { result.arr[i + 1] = arr.arr[i]; }
	return result;
}
template<class T>
array<T, 1> operator+(T e, array<T, 0>) {
	array<T, 1> result = array<T, 1>{e};
	return result;
}

/*
 * Only exposed in unit testing.
 */
#ifdef D_ITERATOR_UNIT_TEST

struct array_f {
	uint64 arr  = ~0ULL;
	int    size = 0;

	constexpr int8 operator[](uint64 i) const { return int8((arr >> (i * 8)) & 0xFF); }

	[[nodiscard]] constexpr auto head_tail() const {

		struct head_tail_pair {
			int8    head;
			array_f tail;
		};

		array_f tail;
		tail.arr  = arr >> 8;
		tail.size = size - 1;

		return head_tail_pair{int8(arr & 0xFF), tail};
	}

	explicit array_f(int8 e) {
		arr  = uint8(e);
		size = 1;
	}

	array_f(std::initializer_list<int8> l) {
		int i = 0;
		arr   = 0;
		for (const auto &e: l) {
			arr |= uint64(uint8(e)) << (i * 8);
			i++;
		}
		size = i;
	}

	bool operator==(const array_f &other) const {
		if (size != other.size) { return false; }
		uint64 mask = ~0ULL >> (64 - size * 8);
		return (arr & mask) == (other.arr & mask);
	}

	array_f() = default;

	// implement copy and copy assignment
	constexpr array_f(const array_f &other) {
		arr  = other.arr;
		size = other.size;
	}
	constexpr array_f &operator=(const array_f &other) {
		arr  = other.arr;
		size = other.size;
		return *this;
	}

	[[nodiscard]] constexpr auto to_iterator() const;

	~array_f() = default;
};


struct iterator {
	array_f arr;
	int     index    = 0;
	using value_type = int8;

	constexpr iterator(array_f arr, int index) : arr(arr), index(index) {}

	[[nodiscard]] constexpr bool has_next() const { return index != arr.size; }
	constexpr int8               operator*() const { return arr[index]; }
	constexpr void               operator++() { index++; }

	[[nodiscard]] uint64 count() const { return arr.size - index; }
};

[[nodiscard]] constexpr auto array_f::to_iterator() const { return iterator(*this, 0); }
array_f                      operator+(int8 e, array_f arr) {
    //	if (arr.size > 7) { __builtin_trap(); }
    array_f result;

    result.arr = uint8(e);
    result.arr |= arr.arr << 8;
    result.size = arr.size + 1;
    return result;
}

#endif

#endif //D_ITERATOR_ARRAY_H
