#pragma once

#include <array>
#include <assert.h>
#include <iterator>
#include <vector>

namespace goldfish
{
	template <class T>
	class array_ref
	{
	public:
		constexpr array_ref()
			: m_begin(nullptr)
			, m_end(nullptr)
		{}
		constexpr array_ref(T* begin, T* end)
			: m_begin(begin)
			, m_end(end)
		{}
		constexpr array_ref(T* begin, size_t size)
			: m_begin(begin)
			, m_end(m_begin + size)
		{}
		template <class U, size_t N> constexpr array_ref(std::array<U, N>& rhs)
			: m_begin(rhs.data())
			, m_end(rhs.data() + N)
		{}
		template <class U, size_t N> constexpr array_ref(const std::array<U, N>& rhs)
			: m_begin(rhs.data())
			, m_end(rhs.data() + N)
		{}
		template <class U, size_t N> constexpr array_ref(U(&rhs)[N])
			: m_begin(rhs)
			, m_end(rhs + N)
		{}
		template <class U> array_ref(std::vector<U>& rhs)
			: m_begin(rhs.data())
			, m_end(rhs.data() + rhs.size())
		{}
		template <class U> array_ref(const std::vector<U>& rhs)
			: m_begin(rhs.data())
			, m_end(rhs.data() + rhs.size())
		{}
		template <class U, size_t N> constexpr array_ref(array_ref<U> rhs)
			: m_begin(rhs.begin())
			, m_end(rhs.end())
		{}

		constexpr T* begin() const { return m_begin; }
		constexpr T* end() const { return m_end; }
		constexpr T* data() const { return m_begin; }
		size_t size() const { return std::distance(m_begin, m_end); }
		constexpr bool empty() const { return m_begin == m_end; }
		constexpr T& front() const { return *m_begin; }
		constexpr T& back() const { return *(m_end - 1); }
		T& pop_front() { assert(!empty()); return *(m_begin++); }
		T& operator[](size_t i) { assert(i < size()); return m_begin[i]; }
		std::vector<std::decay_t<T>> vec() const { return{ begin(), end() }; }

		void clear()
		{
			m_begin = m_end;
		}
		array_ref<T> remove_front(size_t n)
		{
			assert(n <= size());
			auto b = m_begin;
			m_begin += n;
			return{ b, m_begin };
		}
		constexpr array_ref<T> slice_from_front(size_t c) const
		{
			assert(c <= size());
			return{ m_begin, m_begin + c };
		}
		constexpr array_ref<T> without_front(size_t c) const
		{
			assert(s <= size());
			return{ m_begin + c, m_end };
		}
		constexpr array_ref<T> without_end(size_t c) const
		{
			assert(c <= size());
			return{ m_begin, m_end - c };
		}
		constexpr array_ref<T> slice(size_t from, size_t to) const
		{
			assert(to <= size());
			assert(from <= to);
			return{ m_begin + from, m_begin + to };
		}

	private:
		T* m_begin;
		T* m_end;
	};

	template <class T, class U>
	size_t copy(array_ref<T> from, array_ref<U> to)
	{
		assert(from.size() == to.size());
		std::copy(from.begin(), from.end(), stdext::make_unchecked_array_iterator(to.begin()));
		return from.size();
	}

	template <class T, class U>
	size_t copy_and_pop(array_ref<T>& from, array_ref<U>& to)
	{
		auto to_copy = std::min(from.size(), to.size());
		return copy(from.remove_front(to_copy), to.remove_front(to_copy));
	}

	template <class T, size_t N> array_ref<T> make_array_ref(std::array<T, N>& rhs) { return{ rhs }; }
	template <class T, size_t N> array_ref<const T> make_array_ref(const std::array<T, N>& rhs) { return{ rhs }; }
	template <class T, size_t N> array_ref<T> make_array_ref(T(&rhs)[N]) { return{ rhs }; }
	template <class T> array_ref<T> make_array_ref(std::vector<T>& rhs) { return{ rhs }; }
	template <class T> array_ref<const T> make_array_ref(const std::vector<T>& rhs) { return{ rhs }; }
	template <class T> array_ref<T> make_array_ref(const array_ref<T>& rhs) { return rhs; }

	template <class T, class U> constexpr array_ref<std::enable_if_t<!std::is_const<U>::value, T>> reinterpret_helper(array_ref<U> from)
	{
		return{ reinterpret_cast<T*>(from.data()), from.size() * sizeof(U) / sizeof(T) };
	}
	template <class T, class U> constexpr array_ref<std::enable_if_t<std::is_const<U>::value && std::is_const<T>::value, T>> reinterpret_helper(array_ref<U> from)
	{
		return{ reinterpret_cast<T*>(from.data()), from.size() * sizeof(U) / sizeof(T) };
	}
	template <class T, class U> constexpr auto reinterpret(U&& u) { return reinterpret_helper<T>(make_array_ref(std::forward<U>(u))); }

	using const_buffer_ref = array_ref<const uint8_t>;
	using buffer_ref = array_ref<uint8_t>;

	template <class T> const_buffer_ref constexpr to_buffer(const T& t) { return{ reinterpret_cast<const uint8_t*>(&t), reinterpret_cast<const uint8_t*>(&t + 1) }; }
	template <class T> buffer_ref constexpr to_buffer(T& t) { return{ reinterpret_cast<uint8_t*>(&t), reinterpret_cast<uint8_t*>(&t + 1) }; }

	template <size_t N> constexpr array_ref<const char> string_literal_to_array_ref_without_null_terminator(const char(&s)[N])
	{
		return{ s, N - 1 };
	}
}