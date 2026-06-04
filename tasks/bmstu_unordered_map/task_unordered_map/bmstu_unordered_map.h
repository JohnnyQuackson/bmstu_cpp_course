#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iterator>
#include <list>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "fast_streebog.h"

namespace bmstu
{

namespace detail
{

template <typename Buffer, typename = void>
struct is_byte_buffer_impl : std::false_type
{
};

template <typename Buffer>
struct is_byte_buffer_impl<
	Buffer,
	std::void_t<decltype(std::declval<const Buffer&>().data()),
				decltype(std::declval<const Buffer&>().size())>>
	: std::bool_constant<
		  std::is_same_v<std::remove_cv_t<std::remove_pointer_t<
							 decltype(std::declval<const Buffer&>().data())>>,
						 uint8_t> &&
		  std::is_convertible_v<decltype(std::declval<const Buffer&>().size()),
								std::size_t>>
{
};

template <typename T, typename = void>
struct has_raw_bytes_impl : std::false_type
{
};

template <typename T>
struct has_raw_bytes_impl<
	T,
	std::void_t<decltype(std::declval<const T&>().rawBytes())>>
	: is_byte_buffer_impl<decltype(std::declval<const T&>().rawBytes())>
{
};

template <typename T, typename = void>
struct has_equality_operator_impl : std::false_type
{
};

template <typename T>
struct has_equality_operator_impl<
	T,
	std::void_t<decltype(std::declval<const T&>() == std::declval<const T&>())>>
	: std::is_convertible<decltype(std::declval<const T&>() ==
								   std::declval<const T&>()),
						  bool>
{
};

}  // namespace detail

template <typename T>
struct has_raw_bytes : detail::has_raw_bytes_impl<T>
{
};

template <typename T>
inline constexpr bool has_raw_bytes_v = has_raw_bytes<T>::value;

template <typename T>
struct has_equality_operator : detail::has_equality_operator_impl<T>
{
};

template <typename T>
inline constexpr bool has_equality_operator_v = has_equality_operator<T>::value;

template <typename K>
struct streebog_hash
{
	static_assert(has_raw_bytes_v<K> || std::is_trivially_copyable_v<K>,
				  "bmstu::streebog_hash: key type must be trivially-copyable "
				  "or provide `rawBytes() const` returning a byte buffer with "
				  "`data()`/`size()` (e.g. std::span<const uint8_t> or "
				  "std::vector<uint8_t>). ");

	std::size_t operator()(const K& key) const noexcept
	{
		uint8_t digest[32];
		if constexpr (has_raw_bytes_v<K>)
		{
			auto bytes = key.rawBytes();
			streebog_hash_256(bytes.data(), bytes.size(), digest);
		}
		else
		{
			streebog_hash_256(reinterpret_cast<const uint8_t*>(&key), sizeof(K),
							  digest);
		}
		std::size_t result = 0;
		std::memcpy(&result, digest, sizeof(std::size_t));
		return result;
	}
};

template <>
struct streebog_hash<std::string>
{
	std::size_t operator()(const std::string& key) const noexcept
	{
		uint8_t digest[32];
		streebog_hash_256(reinterpret_cast<const uint8_t*>(key.data()),
						  key.size(), digest);
		std::size_t result = 0;
		std::memcpy(&result, digest, sizeof(std::size_t));
		return result;
	}
};

template <typename K,
		  typename V,
		  typename Hash = streebog_hash<K>,
		  typename Equal = std::equal_to<K>>
class unordered_map
{
	static_assert(has_equality_operator_v<K>,
				  "bmstu::unordered_map: key type must provide `operator==`.");

   public:
	using key_type = K;
	using mapped_type = V;
	using value_type = std::pair<const K, V>;
	using size_type = std::size_t;

   private:
	static constexpr size_type DEFAULT_BUCKET_COUNT = 16;
	static constexpr double MAX_LOAD_FACTOR = 0.75;

	using bucket_type = std::list<value_type>;

	std::vector<bucket_type> buckets_;
	size_type size_ = 0;
	Hash hasher_;
	Equal equal_;

	size_type bucket_for(const K& key) const
	{
		return hasher_(key) % bucket_count();
	}

	void rehash(size_type new_count)
	{
		std::vector<bucket_type> new_buckets(new_count);
		for (auto& bucket : buckets_)
		{
			for (auto& kv : bucket)
			{
				std::size_t idx = hasher_(kv.first) % new_count;
				new_buckets[idx].push_back(std::move(kv));
			}
		}
		buckets_ = std::move(new_buckets);
	}

	size_type power_of_2(size_type n) const
	{
		size_type x = 1;
		while (x < n)
		{
			x <<= 1;
		}
		return x;
	}

   public:
	template <typename IteratorValueType,
			  typename VecPtrType,
			  typename ListIterType>
	struct base_iterator
	{
		using iterator_category = std::forward_iterator_tag;
		using value_type = IteratorValueType;
		using difference_type = std::ptrdiff_t;
		using pointer = value_type*;
		using reference = value_type&;

		VecPtrType buckets_ = nullptr;
		size_type bucket_idx_ = 0;
		ListIterType list_it_;

		base_iterator() = default;

		base_iterator(VecPtrType buckets, size_type idx, ListIterType it)
			: buckets_(buckets), bucket_idx_(idx), list_it_(it)
		{
		}

		reference operator*() const { return *list_it_; }
		pointer operator->() const { return &(*list_it_); }

		base_iterator& operator++()
		{
			++list_it_;
			if (list_it_ == (*buckets_)[bucket_idx_].end())
			{
				++bucket_idx_;
				while (bucket_idx_ < buckets_->size() &&
					   (*buckets_)[bucket_idx_].empty())
				{
					++bucket_idx_;
				}

				if (bucket_idx_ < buckets_->size())
				{
					list_it_ = (*buckets_)[bucket_idx_].begin();
				}
				else
				{
					list_it_ = {};
				}
			}
			return *this;
		}

		base_iterator operator++(int)
		{
			auto tmp = *this;
			++(*this);
			return tmp;
		}

		bool operator==(const base_iterator& o) const
		{
			return (buckets_ == o.buckets_ && bucket_idx_ == o.bucket_idx_ &&
					list_it_ == o.list_it_);
		}

		bool operator!=(const base_iterator& o) const { return !(*this == o); }
	};
	struct iterator : public base_iterator<unordered_map::value_type,
										   std::vector<bucket_type>*,
										   typename bucket_type::iterator>
	{
		using base_iterator<unordered_map::value_type,
							std::vector<bucket_type>*,
							typename bucket_type::iterator>::base_iterator;
	};

	struct const_iterator
		: public base_iterator<const unordered_map::value_type,
							   const std::vector<bucket_type>*,
							   typename bucket_type::const_iterator>
	{
		using base_iterator<
			const unordered_map::value_type,
			const std::vector<bucket_type>*,
			typename bucket_type::const_iterator>::base_iterator;

		const_iterator(const iterator& other)
			: base_iterator<const unordered_map::value_type,
							const std::vector<bucket_type>*,
							typename bucket_type::const_iterator>(
				  other.buckets_,
				  other.bucket_idx_,
				  other.list_it_)
		{
		}
	};

	explicit unordered_map(size_type bucket_count = DEFAULT_BUCKET_COUNT)
		: buckets_(bucket_count)
	{
	}

	unordered_map(const unordered_map&) = default;
	unordered_map(unordered_map&&) = default;
	unordered_map& operator=(const unordered_map&) = default;
	unordered_map& operator=(unordered_map&&) = default;
	~unordered_map() = default;

	iterator begin()
	{
		size_type idx = 0;
		while (idx < bucket_count() && buckets_[idx].empty())
		{
			++idx;
		}
		if (idx < bucket_count())
		{
			return iterator(&buckets_, idx, buckets_[idx].begin());
		}
		return end();
	}

	iterator end() { return iterator(&buckets_, buckets_.size(), {}); }

	const_iterator begin() const
	{
		size_type idx = 0;
		while (idx < bucket_count() && buckets_[idx].empty())
		{
			++idx;
		}
		if (idx < bucket_count())
		{
			return const_iterator(&buckets_, idx, buckets_[idx].begin());
		}
		return end();
	}

	const_iterator end() const
	{
		return const_iterator(&buckets_, buckets_.size(), {});
	}

	const_iterator cbegin() const { return begin(); }
	const_iterator cend() const { return end(); }

	size_type size() const noexcept { return size_; }

	bool empty() const noexcept { return size_ == 0; }

	iterator find(const K& key)
	{
		size_type idx = bucket_for(key);
		auto it = buckets_[idx].begin();
		while (it != buckets_[idx].end())
		{
			if (it->first == key)
			{
				return iterator(&buckets_, idx, it);
			}
			++it;
		}
		return end();
	}

	const_iterator find(const K& key) const
	{
		size_type idx = bucket_for(key);
		auto it = buckets_[idx].begin();
		while (it != buckets_[idx].end())
		{
			if (it->first == key)
			{
				return const_iterator(&buckets_, idx, it);
			}
			++it;
		}
		return end();
	}

	bool contains(const K& key) const { return find(key) != end(); }

	V& at(const K& key)
	{
		iterator it = find(key);
		if (it != end())
		{
			return it->second;
		}
		throw std::out_of_range("bmstu::unordered_map::at: key not found");
	}

	const V& at(const K& key) const
	{
		const_iterator it = find(key);
		if (it != end())
		{
			return it->second;
		}
		throw std::out_of_range("bmstu::unordered_map::at: key not found");
	}

	std::pair<iterator, bool> insert(const value_type& kv)
	{
		iterator it = find(kv.first);
		if (it != end())
		{
			return {it, false};
		}
		else
		{
			++size_;
			if (load_factor() > MAX_LOAD_FACTOR)
			{
				rehash(bucket_count() * 2);
			}

			size_type idx = bucket_for(kv.first);
			buckets_[idx].push_front(kv);
			return {iterator(&buckets_, idx, buckets_[idx].begin()), true};
		}
	}

	V& operator[](const K& key)
	{
		iterator it = find(key);
		if (it != end())
		{
			return it->second;
		}
		return insert({key, V{}}).first->second;
	}

	bool erase(const K& key)
	{
		iterator it = find(key);
		if (it != end())
		{
			buckets_[it.bucket_idx_].erase(it.list_it_);
			--size_;
			return true;
		}
		return false;
	}

	void clear()
	{
		for (size_type idx = 0; idx < bucket_count(); ++idx)
		{
			buckets_[idx].clear();
		}
		size_ = 0;
	}

	double load_factor() const
	{
		return static_cast<double>(size_) / bucket_count();
	}

	size_type bucket_count() const { return buckets_.size(); }

	void reserve(size_type count)
	{
		size_type min_buckets = (size_type)(count / MAX_LOAD_FACTOR) + 1;
		min_buckets = power_of_2(min_buckets);
		if (min_buckets > bucket_count())
		{
			rehash(min_buckets);
		}
	}
};

}  // namespace bmstu
