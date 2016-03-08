#pragma once

#include "tags.h"
#include "stream.h"
#include "optional.h"

namespace goldfish
{
	template <class... types>
	class document_on_variant
	{
	public:
		using tag = tags::document;
		template <class... Args> document_on_variant(Args&&... args)
			: m_data(std::forward<Args>(args)...)
		{}

		template <class Lambda> decltype(auto) visit(Lambda&& l) &
		{
			return m_data.visit([&](auto& x) -> decltype(auto)
			{
				return l(x, tags::get_tag(x));
			});
		}
		template <class Lambda> decltype(auto) visit(Lambda&& l) &&
		{
			return std::move(m_data).visit([&](auto&& x) -> decltype(auto)
			{
				return l(std::forward<decltype(x)>(x), tags::get_tag(x));
			});
		}
		template <class tag> auto& as() & noexcept
		{
			return m_data.as<tags::type_with_tag_t<tag, types...>>();
		}
		template <class tag> auto&& as() && noexcept
		{
			return std::move(m_data).as<tags::type_with_tag_t<tag, types...>>();
		}
		template <class tag> bool is() const noexcept
		{
			static_assert(tags::is_tag<tag>::value, "document::is must be called with a tag (see tags.h)");
			return m_data.visit([&](auto&& x)
			{
				return std::is_same<tag, decltype(tags::get_tag(x))>::value;
			});
		}
		template <class T> bool is_exactly() const noexcept
		{
			return m_data.is<T>();
		}

		using invalid_state = typename variant<types...>::invalid_state;
	private:
		variant<types...> m_data;
	};

	template <class Document> std::enable_if_t<tags::has_tag<std::decay_t<Document>, tags::document>::value, void> skip(Document&& d)
	{
		d.visit([&](auto&& x, auto) { skip(std::forward<decltype(x)>(x), tags::get_tag(x)); });
	}
	template <class type> void skip(type&&, tags::undefined) {}
	template <class type> void skip(type&&, tags::floating_point) {}
	template <class type> void skip(type&&, tags::unsigned_int) {}
	template <class type> void skip(type&&, tags::signed_int) {}
	template <class type> void skip(type&&, tags::boolean) {}
	template <class type> void skip(type&&, tags::null) {}
	template <class type> void skip(type&& x, tags::byte_string)
	{
		stream::skip(x, std::numeric_limits<uint64_t>::max());
	}
	template <class type> void skip(type&& x, tags::text_string)
	{
		stream::skip(x, std::numeric_limits<uint64_t>::max());
	}
	template <class type> void skip(type&& x, tags::array)
	{
		while (auto d = x.read())
			skip(*d);
	}

	template <class type> void skip(type&& x, tags::map)
	{
		while (auto d = x.read_key())
		{
			skip(*d);
			skip(x.read_value());
		}
	}

	template <class Map> class filtered_map
	{
	public:
		filtered_map(Map&& map, array_ref<const uint64_t> key_names)
			: m_map(std::move(map))
			, m_key_names(key_names)
		{}
		optional<decltype(std::declval<Map>().read_value())> read_value_by_index(size_t index)
		{
			assert(m_index < m_key_names.size());
			if (m_index > index)
				return nullopt;

			if (m_on_value)
			{
				m_on_value = false;
				if (m_index == index)
					return m_map.read_value();
				else
					skip(m_map.read_value());
			}
			assert(!m_on_value);

			while (auto key = m_map.read_key())
			{
				// We currently only support unsigned int key types
				if (!key->is<tags::unsigned_int>())
				{
					skip(*key);
					skip(m_map.read_value());
					continue;
				}

				// do any of the keys match?
				auto it = std::find(m_key_names.begin() + m_index, m_key_names.end(), key->as<tags::unsigned_int>());
				if (it == m_key_names.end())
				{
					// This was a new key that we didn't know about, skip it
					skip(m_map.read_value());
					continue;
				}

				// We found the key, compute its index
				m_index = std::distance(m_key_names.begin(), it);
				if (m_index == index)
				{
					// That's the key we were looking for, return its value
					// at that point, we assume not being on value any more because the caller will process the value
					assert(!m_on_value);
					return m_map.read_value();
				}
				else if (m_index > index)
				{
					// Our key was not found (we found a key later in the list of keys)
					// We are on the value of that later key
					m_on_value = true;
					return nullopt;
				}
				else
				{
					// We found a key that is still before us, skip the value and keep searching
					skip(m_map.read_value());
				}
			}

			return nullopt;
		}
		friend void skip(filtered_map& m)
		{
			if (m.m_on_value)
			{
				skip(m.m_map.read_value());
				m.m_on_value = false;
			}

			goldfish::skip(m.m_map, tags::map{});
			m.m_index = m.m_key_names.size();
		}
	private:
		Map m_map;
		array_ref<const uint64_t> m_key_names;
		size_t m_index = 0;
		bool m_on_value = false;
	};
	template <class Map> filtered_map<std::decay_t<Map>> filter_map(Map&& map, array_ref<const uint64_t> key_names)
	{
		return{ std::forward<Map>(map), key_names };
	}
}