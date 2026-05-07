#include <ostream>
#include <stdexcept>
#include <utility>
#include "array_ptr.h"

namespace bmstu
{
template <typename T>
class simple_vector
{
   public:
	class iterator
	{
	   public:
		using iterator_category = std::contiguous_iterator_tag;
		using value_type = T;
		using pointer = T*;
		using reference = T&;
		using difference_type = std::ptrdiff_t;

		iterator() = default;

		iterator(const iterator& other) = default;

		iterator(std::nullptr_t) noexcept : ptr_(nullptr) {}

		iterator(iterator&& other) = default;

		explicit iterator(pointer ptr) : ptr_(ptr) {}

		reference operator*() const { return *ptr_; }

		pointer operator->() const { return ptr_; }

		friend pointer to_address(const iterator& it) noexcept
		{
			return it.ptr_;
		}

		iterator& operator=(const iterator& other) = default;

		iterator& operator=(iterator&& other) = default;

#pragma region Operators
		iterator& operator++()
		{
			++ptr_;
			return *this;
		}

		iterator& operator--()
		{
			--ptr_;
			return *this;
		}

		iterator operator++(int)
		{
			iterator tmp = *this;
			ptr_++;
			return tmp;
		}

		iterator operator--(int)
		{
			iterator tmp = *this;
			ptr_--;
			return tmp;
		}

		explicit operator bool() const { return ptr_ != nullptr; }

		friend bool operator==(const iterator& lhs, const iterator& rhs)
		{
			return lhs.ptr_ == rhs.ptr_;
		}

		friend bool operator==(const iterator& lhs, std::nullptr_t)
		{
			return lhs.ptr_ == nullptr;
		}

		friend auto operator<=>(const iterator& lhs, const iterator& rhs)
		{
			return lhs.ptr_ <=> rhs.ptr_;
		}

		reference operator[](difference_type n) const { return ptr_[n]; }

		iterator& operator=(std::nullptr_t) noexcept
		{
			ptr_ = nullptr;
			return *this;
		}

		friend bool operator==(std::nullptr_t, const iterator& rhs)
		{
			return rhs.ptr_ == nullptr;
		}

		friend bool operator!=(const iterator& lhs, const iterator& rhs)
		{
			return lhs.ptr_ != rhs.ptr_;
		}

		iterator operator+(const difference_type& n) const noexcept
		{
			return iterator(ptr_ + n);
		}

		iterator operator+=(const difference_type& n) noexcept
		{
			ptr_ += n;
			return *this;
		}

		iterator operator-(const difference_type& n) const noexcept
		{
			return iterator(ptr_ - n);
		}

		iterator operator-=(const difference_type& n) noexcept
		{
			ptr_ -= n;
			return *this;
		}

		friend difference_type operator-(const iterator& end,
										 const iterator& begin) noexcept
		{
			return end.ptr_ - begin.ptr_;
		}

#pragma endregion
	   private:
		pointer ptr_ = nullptr;
	};

	simple_vector() noexcept = default;

	~simple_vector() = default;

	simple_vector(std::initializer_list<T> init)
	{
		size_t size = init.size(), i = 0;
		array_ptr<T> new_data(size);

		if (size > 0)
		{
			for (T c : init)
			{
				new_data[i++] = c;
			}
			data_ = std::move(new_data);
			size_ = size;
			capacity_ = size;
		}
	}

	simple_vector(const simple_vector& other)
	{
		size_ = other.size();
		capacity_ = other.capacity();
		array_ptr<T> new_data(capacity_);
		for (size_t i = 0; i < size_; i++)
		{
			new_data[i] = other[i];
		}
		data_ = std::move(new_data);
	}

	simple_vector(simple_vector&& other) noexcept { swap(other); }

	simple_vector& operator=(const simple_vector& other)
	{
		if (this == &other)
			return *this;
		size_ = other.size();
		capacity_ = other.capacity();
		array_ptr<T> new_data(capacity_);
		for (size_t i = 0; i < size_; i++)
		{
			new_data[i] = other[i];
		}
		data_ = std::move(new_data);
		return *this;
	}

	simple_vector(size_t size, const T& value = T{})
	{
		size_ = size;
		capacity_ = size;
		array_ptr<T> new_data(capacity_);
		for (size_t i = 0; i < size_; i++)
		{
			new_data[i] = value;
		}
		data_ = std::move(new_data);
	}

	iterator begin() noexcept { return iterator(data_.get()); }

	iterator end() noexcept { return iterator(data_.get() + size_); }

	using const_iterator = iterator;

	const_iterator begin() const noexcept
	{
		return const_iterator(data_.get());
	}

	const_iterator end() const noexcept
	{
		return const_iterator(data_.get() + size_);
	}

	typename iterator::reference operator[](size_t index) noexcept
	{
		return data_[index];
	}

	typename const_iterator::reference operator[](size_t index) const noexcept
	{
		return data_.get()[index];
	}

	typename iterator::reference at(size_t index) { return data_.get()[index]; }

	typename const_iterator::reference at(size_t index) const
	{
		return data_.get()[index];
	}

	size_t size() const noexcept { return size_; }

	size_t capacity() const noexcept { return capacity_; }

	void swap(simple_vector& other) noexcept
	{
		my_swap(size_, other.size_);
		my_swap(capacity_, other.capacity_);
		data_.swap(other.data_);
	}

	friend void swap(simple_vector& lhs, simple_vector& rhs) noexcept
	{
		my_swap(lhs.size_, rhs.size_);
		my_swap(lhs.capacity_, rhs.capacity_);
		lhs.data_.swap(rhs.data_);
	}

	void reserve(size_t new_cap)
	{
		if (new_cap <= capacity_)
			return;
		array_ptr<T> new_data(new_cap);
		for (size_t i = 0; i < size_; i++)
		{
			new_data[i] = std::move(data_[i]);
		}
		data_ = std::move(new_data);
		capacity_ = new_cap;
	}

	void resize(size_t new_size)
	{
		if (new_size <= size_)
		{
			size_ = new_size;
		}
		else
		{
			reserve(std::max(new_size, capacity_ * 2));
			for (size_t i = size_; i < new_size; i++)
			{
				data_[i] = T{};
			}
			size_ = new_size;
		}
	}

	iterator insert(const_iterator where, T&& value)
	{
		size_t index = where - begin();
		if (size_ == capacity_)
		{
			reserve(std::max(size_ + 1, capacity_ * 2));
		}
		for (size_t i = size_; i > index; --i)
		{
			data_[i] = std::move(data_[i - 1]);
		}
		data_[index] = std::move(value);
		size_++;
		return begin() + index;
	}

	iterator insert(const_iterator where, const T& value)
	{
		insert(std::move(value));
	}

	void push_back(T&& value) { insert(end(), std::move(value)); }

	void clear() noexcept { size_ = 0; }

	void push_back(const T& value)
	{
		T copy = value;
		insert(end(), std::move(copy));
	}

	bool empty() const noexcept { return size_ == 0; }

	void pop_back()
	{
		if (size_ > 0)
		{
			size_--;
		}
	}

	friend bool operator==(const simple_vector& lhs, const simple_vector& rhs)
	{
		if (lhs.size_ == rhs.size_)
		{
			for (size_t i = 0; i < lhs.size_; i++)
			{
				if (lhs[i] != rhs[i])
				{
					return false;
				}
			}
			return true;
		}
		return false;
	}

	friend bool operator!=(const simple_vector& lhs, const simple_vector& rhs)
	{
		return !(lhs == rhs);
	}

	friend auto operator<=>(const simple_vector& lhs, const simple_vector& rhs)
	{
		return alphabet_compare(lhs, rhs);
	}

	friend std::ostream& operator<<(std::ostream& os, const simple_vector& vec)
	{
		return os;
	}
	iterator erase(iterator where)
	{
		size_t index = where - begin();
		for (size_t i = index; i < (size_ - 1); i++)
		{
			data_[i] = std::move(data_[i + 1]);
		}
		size_--;
		return begin() + index;
	}

   private:
	static auto alphabet_compare(const simple_vector<T>& lhs,
								 const simple_vector<T>& rhs)
	{
		size_t min_size = std::min(lhs.size_, rhs.size_);
		for (size_t i = 0; i < min_size; i++)
		{
			if (lhs[i] != rhs[i])
			{
				return (lhs[i] <=> rhs[i]);
			}
		}

		return lhs.size_ <=> rhs.size_;
	}
	array_ptr<T> data_;
	size_t size_ = 0;
	size_t capacity_ = 0;
};
}  // namespace bmstu
