# d_iterator

## Description

d_iterator is a C++20 dependency-free header-only library that provides a set of iterator adapters and utilities.
dependency-free means, it compiles with "-nodefaultlibs -fno-builtin -nostdlib -fno-exceptions".

Iterators are a great way to write code, with an easy analysable complexity, constant memory usage and no side effects.
It also makes it easy to argue about the correctness of the code.

## Setup Tests

To run the unit tests, you need to install googletest first.
The following commands will install googletest and run the unit tests.

```bash
meson wrap install gtest
meson setup build
ninja -C build test
```

## Examples

Currently, there are no examples, but you can find the unit tests in the test folder.
The benchmarks are in the benchmark folder.

## Inner Workings

An iterator is an immutable view of a container.
The ability to modify the elements in-place is evaluated.
A container can provide its own iterator type.
By default, there is the default iterator type.
It can be initialized using a flat array.

```cpp
int *arr = new int[len];
auto it_1 = it::iterator(arr, len); // array and length
auto it_1 = it::iterator(arr, arr + len); // pointer and past-the-end pointer pair
```

There are 3 special iterator types:

- `it::c_string_iterator` for C-strings (use strings with predefined length, C-strings are slow as hell)
- `it::sequence_generator` for sequences like pythons `range` function
- `it::infinite_sqeuence_generator` for infinite sequences

Any iterator must comply with the following interface:

```cpp
*it; // gets the current element
++it; // moves the iterator to the next element
it.has_next(); // returns true if it is allowed to dereference the iterator
```

It is strictly forbidden to increment or dereference an iterator that has no next element.
There are no safety checks.

Any iterator may be copied and moved at any time and as often as you want.
Therefore `*it` and `it.has_next()` must be deterministic.

There is one extension to the interface, and 3 in work:

```cpp
// CountingIterator
it.count(); // returns the number of elements that are left.
// additional work in the future:
// ReverseIterator
it.reverse(); // returns an iterator that iterates in reverse order
// MutableIterator
*it = 42; // sets the current element to 42
// RandomAccessIterator
it += 42; // moves the iterator 42 elements forward
it.peek(42); // returns the element 42 elements forward
```

If you have a generic iterator there is no guarantee that it supports any of the extensions.
You need to consult the documentation of the iterator.
`it::counted_wrapper()` returns a `CountingIterator` even if the underlying iterator does not support it.
Use with caution.
The count is not cached and there is no lazy evaluation.
This makes it likely that an O(n) algorithm becomes O(n^2).
Do not call `it.count()` itself, instead use the algorithm `algo::count()`.

## Algorithms

There are 3 algorithms already implemented:
These algorithms do the actual work.

```cpp

// reduction, note you cant pass a function pointer as argument.
// it must be a template argument.
auto maximum = algo::reduce<[](auto a, auto b) { return max(a, b); }>(it, 0); // returns the maximum element

auto element_count = algo::count(it); // returns the number of elements

std::vector<int> vec = algo::to_array<std::vector<int>>(it); // returns a vector with all elements
```

## Functions

These functions exist to implement your own algorithms on top of the existing algorithms.

```cpp

auto new_it = it::map(it, [](auto a) { return *a; }); // returns an iterator that applies the function to each element

auto new_it = it::filter(it, [](auto a) { return a > 0; }); // returns an iterator that only iterates over elements that satisfy the predicate

auto new_it = it::take(it, 42); // returns an iterator that only iterates over the first 42 elements

auto new_it = it::zip(it, it2); // returns an iterator that iterates over both iterators at the same time

auto new_it = it::zip(it, it::infinite_sequence_generator); // returns an iterator that emumerates the elements of the first iterator

auto new_it = append(it, it2); // returns an iterator that iterates over the first iterator and then over the second iterator

// O(n^2) iterators
auto new_it = it::cross_product(it, it2); // returns an iterator that iterates over all pairs of elements of both iterators
auto new_it = it::unordered_pairs(it); // returns an iterator that iterates over all unordered pairs of elements of the iterator
```

You can't do everything with an iterator.
For example this won't work:

```cpp
while(cond()) {
    it = it::map(it, [](auto a) { return *a; });
}
```

The reason is, that the `it` in one iteration is a different type than the `it` in the next iteration.
Thanks to C++, the error message is not very helpful in that case.
The same holds for recursion.
This would require some kind of type erasure, which is not possible without dynamic memory allocation and not on my list
of priorities.

## Pipe Notation

The pipe notation is a way to write code in a more functional style.
Its purpose is to avoid the bracket hell that comes with nested function calls.
Every function that takes only one iterator as argument can be used with the pipe notation.

```cpp
auto new_it = it | it::map([](auto a) { return *a; }) | it::filter([](auto a) { return a > 0; }) | it::take(42);

// is equivalent to

auto new_it = it::take(42)(it::filter([](auto a) { return a > 0; })(it::map([](auto a) { return *a; })(it)));
```

## Using the library without the standard library

In this case the library only supports clang and gcc due to the use of the `__builtin` functions.
Please pass "-DNO_STD" to the compiler.
This will include not even one header from the standard library.
Since we can't use #include <cstdint> we need to define the types ourselves.
They may not match the compiler wide types.
But every 64-bit compiler should have the same types.


