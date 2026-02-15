#pragma once

#include <cstring>	// для std::memcpy
#include <exception>
#include <iostream>

namespace bmstu
{
template <typename T>
class basic_string;

using string = basic_string<char>;
using wstring = basic_string<wchar_t>;
using u16string = basic_string<char16_t>;
using u32string = basic_string<char32_t>;

template <typename T>
class basic_string
{
   private:
	static constexpr size_t SSO_CAPACITY =
		(sizeof(T*) + sizeof(size_t) + sizeof(size_t)) / sizeof(T) - 1;

	struct LongString
	{
		T* ptr;
		size_t size;
		size_t capacity;
	};

	struct ShortString
	{
		T buffer[SSO_CAPACITY + 1];
		unsigned char size;
	};

	union Data
	{
		LongString long_str;
		ShortString short_str;
	};

	Data data_;
	bool is_long_;

	bool is_long() const { return is_long_; }

	T* get_ptr()
	{
		return is_long() ? data_.long_str.ptr : data_.short_str.buffer;
	}

	const T* get_ptr() const
	{
		return is_long() ? data_.long_str.ptr : data_.short_str.buffer;
	}

	size_t get_size() const
	{
		return is_long() ? data_.long_str.size : data_.short_str.size;
	}

	size_t get_capacity() const
	{
		return is_long() ? data_.long_str.capacity : SSO_CAPACITY;
	}

   public:
	basic_string()
	{
		is_long_ = false;
		data_.short_str.buffer[0] = '\0';
		data_.short_str.size = 0;
	}

	basic_string(size_t size)
	{
		T filler = static_cast<T>(' ');

		if (size <= SSO_CAPACITY)
		{
			is_long_ = false;
			data_.short_str.size = static_cast<unsigned char>(size);
			for (size_t i = 0; i < size; ++i)
			{
				data_.short_str.buffer[i] = filler;
			}
			data_.short_str.buffer[size] = '\0';
		}
		else
		{
			is_long_ = true;
			data_.long_str.size = size;
			data_.long_str.capacity = size + 1;
			data_.long_str.ptr = new T[get_capacity()];
			for (size_t i = 0; i < size; ++i)
			{
				data_.long_str.ptr[i] = filler;
			}
			data_.long_str.ptr[size] = '\0';
		}
	}

	basic_string(std::initializer_list<T> il)
	{
		size_t size = il.size();
		if (size <= SSO_CAPACITY)
		{
			is_long_ = false;
			data_.short_str.size = static_cast<unsigned char>(size);
			size_t i = 0;
			for (T c : il)
			{
				data_.short_str.buffer[i++] = c;
			}
			data_.short_str.buffer[size] = '\0';
		}
		else
		{
			is_long_ = true;
			data_.long_str.size = size;
			data_.long_str.capacity = size + 1;
			data_.long_str.ptr = new T[get_capacity()];

			size_t i = 0;
			for (T c : il)
			{
				data_.long_str.ptr[i++] = c;
			}
			data_.long_str.ptr[size] = '\0';
		}
	}

	basic_string(const T* c_str)
	{
		size_t len = strlen_(c_str);

		if (len <= SSO_CAPACITY)
		{
			is_long_ = false;
			std::memcpy(data_.short_str.buffer, c_str, len * sizeof(T));
			data_.short_str.buffer[len] = '\0';
			data_.short_str.size = static_cast<unsigned char>(len);
		}
		else
		{
			is_long_ = true;
			data_.long_str.capacity = len + 1;
			data_.long_str.size = len;
			data_.long_str.ptr = new T[get_capacity()];
			std::memcpy(data_.long_str.ptr, c_str, (len + 1) * sizeof(T));
		}
	}

	basic_string(const basic_string& other)
	{
		is_long_ = other.is_long_;
		if (is_long())
		{
			size_t cap = other.data_.long_str.capacity;
			data_.long_str.ptr = new T[cap];
			data_.long_str.size = other.data_.long_str.size;
			data_.long_str.capacity = cap;
			std::memcpy(data_.long_str.ptr, other.data_.long_str.ptr,
						(data_.long_str.size + 1) * sizeof(T));
		}
		else
		{
			data_.short_str = other.data_.short_str;
		}
	}

	basic_string(basic_string&& dying) noexcept
	{
		is_long_ = dying.is_long_;
		if (is_long())
		{
			data_.long_str = dying.data_.long_str;
			dying.data_.long_str.ptr = nullptr;
			dying.data_.long_str.size = 0;
		}
		else
		{
			data_.short_str = dying.data_.short_str;
		}
		dying.is_long_ = false;
		dying.data_.short_str.size = 0;
	}

	~basic_string() { clean_(); }

	const T* c_str() const { return get_ptr(); }

	size_t size() const
	{
		return is_long() ? data_.long_str.size
						 : static_cast<size_t>(data_.short_str.size);
	}

	bool is_using_sso() const { return !is_long(); }

	size_t capacity() const
	{
		return is_long() ? data_.long_str.capacity : SSO_CAPACITY;
	}

	basic_string& operator=(const basic_string& other)
	{
		if (this == &other)
			return *this;

		if (other.is_long())
		{
			size_t cap = other.get_capacity();
			size_t size = other.get_size();
			T* new_ptr = new T[cap];
			std::memcpy(new_ptr, other.get_ptr(), (size + 1) * sizeof(T));
			clean_();
			data_.long_str.size = size;
			data_.long_str.capacity = cap;
			data_.long_str.ptr = new_ptr;
			is_long_ = true;
		}
		else
		{
			clean_();
			data_.short_str = other.data_.short_str;
			is_long_ = false;
		}
		return *this;
	}

	basic_string& operator=(const T* c_str)
	{
		size_t len = strlen_(c_str);

		if (len <= SSO_CAPACITY)
		{
			clean_();
			is_long_ = false;
			std::memcpy(data_.short_str.buffer, c_str, len * sizeof(T));
			data_.short_str.buffer[len] = '\0';
			data_.short_str.size = static_cast<unsigned char>(len);
		}
		else
		{
			size_t new_cap = len + 1;
			T* new_ptr = new T[new_cap];
			std::memcpy(new_ptr, c_str, (len + 1) * sizeof(T));
			clean_();
			data_.long_str.capacity = new_cap;
			data_.long_str.ptr = new_ptr;
			data_.long_str.size = len;
			is_long_ = true;
		}
		return *this;
	}

	basic_string& operator=(basic_string&& other)
	{
		if (this == &other)
			return *this;
		clean_();
		is_long_ = other.is_long_;
		data_ = other.data_;

		if (other.is_long_)
		{
			other.data_.long_str.ptr = nullptr;
			other.data_.long_str.size = 0;
		}
		else
		{
			other.data_.short_str.size = 0;
			other.data_.short_str.buffer[0] = '\0';
		}
		other.is_long_ = false;

		return *this;
	}

	friend basic_string<T> operator+(const basic_string<T>& left,
									 const basic_string<T>& right)
	{
		basic_string<T> result(left);
		result += right;
		return result;
	}

	template <typename S>
	friend S& operator<<(S& os, const basic_string& obj)
	{
		if (obj.size())
			os.write(obj.data(), obj.get_size());
		return os;
	}

	template <typename S>
	friend S& operator>>(S& is, basic_string& obj)
	{
		obj.clean_();
		T c;
		while (is.get(c))
		{
			obj += c;
		}
		return is;
	}

	basic_string& operator+=(const basic_string& other)
	{
		size_t current_size = get_size();
		size_t other_size = other.get_size();
		size_t new_size = current_size + other_size;
		if (new_size > get_capacity())
		{
			size_t new_cap = new_size + 1;
			T* new_ptr = new T[new_cap];

			std::memcpy(new_ptr, get_ptr(), current_size * sizeof(T));
			std::memcpy(new_ptr + current_size, other.get_ptr(),
						(other_size + 1) * sizeof(T));

			clean_();

			is_long_ = true;
			data_.long_str.ptr = new_ptr;
			data_.long_str.capacity = new_cap;
		}
		else
		{
			// копируем данные из other
			std::memcpy(get_ptr() + current_size, other.get_ptr(),
						(other_size + 1) * sizeof(T));
		}
		if (is_long())
		{
			data_.long_str.size = new_size;
		}
		else
		{
			data_.short_str.size = static_cast<unsigned char>(new_size);
		}
		return *this;
	}

	basic_string& operator+=(T symbol)
	{
		size_t current_size = get_size();
		size_t new_size = current_size + 1;
		if (new_size > get_capacity())
		{
			size_t new_cap = new_size + 1;
			T* new_ptr = new T[new_cap];
			std::memcpy(new_ptr, get_ptr(), current_size * sizeof(T));
			std::memcpy(new_ptr + current_size, &symbol, sizeof(T));
			new_ptr[new_size] = '\0';

			clean_();

			is_long_ = true;
			data_.long_str.ptr = new_ptr;
			data_.long_str.size = new_size;
			data_.long_str.capacity = new_cap;
		}
		else
		{
			get_ptr()[current_size] = symbol;
			get_ptr()[current_size + 1] = '\0';
			if (is_long())
			{
				data_.long_str.size = new_size;
			}
			else
			{
				data_.short_str.size = static_cast<unsigned char>(new_size);
			}
		}
		return *this;
	}

	T& operator[](size_t index) noexcept { return get_ptr()[index]; }

	T& at(size_t index)
	{
		if (index >= get_size())
		{
			throw std::out_of_range("Wrong index");
		}
		return get_ptr()[index];
	}

	T* data() { return get_ptr(); }

	const T* data() const { return get_ptr(); }

   private:
	static size_t strlen_(const T* str)
	{
		size_t len = 0;
		while (*str != '\0')
		{
			str++;
			len++;
		}
		return len;
	}

	void clean_()
	{
		if (is_long())
		{
			delete[] data_.long_str.ptr;
			data_.long_str.ptr = nullptr;
		}
		is_long_ = false;
		data_.short_str.size = 0;
		data_.short_str.buffer[0] = '\0';
	}
};
}  // namespace bmstu
