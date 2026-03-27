#pragma once

#include <algorithm>  // для std::max
#include <cstring>	  // для std::memcpy
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

inline void write_rune_to_utf8(std::ostream& os, char32_t cp)
{
	if (cp <= 0x7F)
	{
		os.put(static_cast<char>(cp));
	}
	else if (cp <= 0x7FF)
	{
		os.put(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
		os.put(static_cast<char>(0x80 | (cp & 0x3F)));
	}
	else if (cp <= 0xFFFF)
	{
		os.put(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
		os.put(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
		os.put(static_cast<char>(0x80 | (cp & 0x3F)));
	}
	else if (cp <= 0x10FFFF)
	{
		os.put(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
		os.put(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
		os.put(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
		os.put(static_cast<char>(0x80 | (cp & 0x3F)));
	}
}

template <typename T>
inline void convert_wide_to_utf8(std::ostream& os, const T* data, size_t size)
{
	for (size_t i = 0; i < size; ++i)
	{
		char32_t cp = static_cast<char32_t>(data[i]);
		write_rune_to_utf8(os, cp);
	}
}

template <typename TraitsOut>
inline void convert_utf8_to_wide(std::basic_ostream<wchar_t, TraitsOut>& os,
								 const char* data,
								 size_t size)
{
	auto udata = reinterpret_cast<const unsigned char*>(data);
	size_t i = 0;
	while (i < size)
	{
		char32_t cp = 0;
		int bytes = 0;

		// Читаем заголовочные биты UTF-8
		if (udata[i] <= 0x7F)
		{
			cp = udata[i];
			bytes = 1;
		}
		else if ((udata[i] & 0xE0) == 0xC0)
		{
			cp = udata[i] & 0x1F;
			bytes = 2;
		}
		else if ((udata[i] & 0xF0) == 0xE0)
		{
			cp = udata[i] & 0x0F;
			bytes = 3;
		}
		else if ((udata[i] & 0xF8) == 0xF0)
		{
			cp = udata[i] & 0x07;
			bytes = 4;
		}
		else
		{
			++i;
			continue;
		}  // Пропускаем мусор

		if (i + bytes > size)
			break;	// Защита от обрыва строки

		// Дочитываем байты продолжения (10xxxxxx)
		bool valid = true;
		for (int j = 1; j < bytes; ++j)
		{
			if ((udata[i + j] & 0xC0) != 0x80)
			{
				valid = false;
				break;
			}
			cp = (cp << 6) | (udata[i + j] & 0x3F);
		}

		if (valid)
		{
			os.put(static_cast<wchar_t>(cp));
		}
		i += bytes;
	}
}
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
		T filler = (T)(' ');

		if (size <= SSO_CAPACITY)
		{
			is_long_ = false;
			data_.short_str.size = (unsigned char)(size);
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
			data_.long_str.capacity = size;
			data_.long_str.ptr = new T[get_capacity() + 1];
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
			data_.short_str.size = (unsigned char)(size);
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
			data_.long_str.capacity = size;
			data_.long_str.ptr = new T[get_capacity() + 1];

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
			data_.short_str.size = (unsigned char)(len);
			std::memcpy(data_.short_str.buffer, c_str, (len + 1) * sizeof(T));
		}
		else
		{
			is_long_ = true;
			data_.long_str.capacity = len;
			data_.long_str.size = len;
			data_.long_str.ptr = new T[get_capacity() + 1];
			std::memcpy(data_.long_str.ptr, c_str, (len + 1) * sizeof(T));
		}
	}

	basic_string(const basic_string& other)
	{
		is_long_ = other.is_long_;
		if (is_long())
		{
			size_t cap = other.get_capacity();
			data_.long_str.ptr = new T[cap + 1];
			data_.long_str.size = other.get_size();
			data_.long_str.capacity = cap;
			std::memcpy(data_.long_str.ptr, other.get_ptr(),
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
			dying.data_.long_str.capacity = 0;
		}
		else
		{
			data_.short_str = dying.data_.short_str;
		}
		dying.data_.short_str.size = 0;
		dying.data_.short_str.buffer[0] = '\0';
		dying.is_long_ = false;
	}

	~basic_string() { clean_(); }

	const T* c_str() const { return get_ptr(); }

	size_t size() const
	{
		return is_long() ? data_.long_str.size : (size_t)(data_.short_str.size);
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
			T* new_ptr = new T[cap + 1];
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
			T new_buffer[SSO_CAPACITY + 1];
			std::memcpy(new_buffer, c_str, (len + 1) * sizeof(T));

			clean_();
			std::memcpy(data_.short_str.buffer, new_buffer,
						(len + 1) * sizeof(T));
			data_.short_str.size = (unsigned char)(len);
			is_long_ = false;
		}
		else
		{
			size_t new_cap = len;
			T* new_ptr = new T[new_cap + 1];
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
			other.data_.long_str.capacity = 0;
			other.data_.long_str.size = 0;
		}

		other.data_.short_str.size = 0;
		other.data_.short_str.buffer[0] = '\0';
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

	// Новый OUTPUT
	template <typename CharOut, typename TraitsOut>
	friend std::basic_ostream<CharOut, TraitsOut>& operator<<(
		std::basic_ostream<CharOut, TraitsOut>& os,
		const basic_string<T>& obj)
	{
		if (obj.size() == 0)
			return os;

		// Типы совпадают
		if constexpr (std::is_same_v<CharOut, T>)
		{
			os.write(obj.data(), obj.get_size());
		}
		// Вывод широкой строки в терминал cout
		else if constexpr (std::is_same_v<CharOut, char> &&
						   !std::is_same_v<T, char>)
		{
			bmstu::convert_wide_to_utf8(os, obj.data(), obj.get_size());
		}
		// ===================================================
		// Вывод строки char в широкий поток wcout
		// Работает только при настройки локали
		// ===================================================
		// #include <locale>
		// std::ios::sync_with_stdio(false);
		// std::wcout.imbue(std::locale(""));
		// std::setlocale(LC_ALL, "");
		// std::locale::global(std::locale(""));
		else if constexpr (std::is_same_v<CharOut, wchar_t> &&
						   std::is_same_v<T, char>)
		{
			bmstu::convert_utf8_to_wide(
				os, reinterpret_cast<const char*>(obj.data()), obj.get_size());
		}

		return os;
	}

	// // Старый OUTPUT
	// template <typename S>
	// friend S& operator<<(S& os, const basic_string& obj)
	// {
	// 	T* ptr = (T*)((void*)obj.get_ptr());
	// 	while (*ptr != T())
	// 	{
	// 		os.put(*ptr);
	// 		ptr++;
	// 	}
	// 	return os;
	// }

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
			size_t new_cap = std::max(get_capacity() * 2, new_size);
			T* new_ptr = new T[new_cap + 1];

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
			std::memmove(get_ptr() + current_size, other.get_ptr(),
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
			size_t new_cap = std::max(get_capacity() * 2, new_size);
			current_size * 2;
			T* new_ptr = new T[new_cap + 1];
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
				data_.short_str.size = (unsigned char)(new_size);
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
			data_.long_str.size = 0;
			data_.long_str.capacity = 0;
		}
		is_long_ = false;
		data_.short_str.size = 0;
		data_.short_str.buffer[0] = '\0';
	}
};
}  // namespace bmstu