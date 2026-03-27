#pragma once

#include <cstring>	// std::memcpy
#include <exception>
#include <iostream>

namespace bmstu
{
template <typename T>
class simple_basic_string;

typedef simple_basic_string<char> string;
typedef simple_basic_string<wchar_t> wstring;
typedef simple_basic_string<char16_t> u16string;
typedef simple_basic_string<char32_t> u32string;

template <typename T>
class simple_basic_string
{
   public:
	/// Конструктор по умолчанию
	simple_basic_string()
	{
		ptr_ = new T[1];
		ptr_[0] = '\0';
		size_ = 0;
	}

	simple_basic_string(size_t size)
	{
		T filler = (T)(' ');
		ptr_ = new T[size + 1];
		size_ = size;
		for (size_t i = 0; i < size_; ++i)
		{
			ptr_[i] = filler;
		}
		ptr_[size_] = '\0';
	}

	simple_basic_string(std::initializer_list<T> il)
	{
		ptr_ = new T[il.size() + 1];
		size_ = il.size();
		size_t i = 0;
		for (T c : il)
		{
			ptr_[i] = c;
			i++;
		}
		ptr_[size_] = '\0';
	}

	/// Конструктор с параметром си-с
	simple_basic_string(const T* c_str)
	{
		size_ = strlen_(c_str);
		ptr_ = new T[size_ + 1];
		std::memcpy(ptr_, c_str, (size_ + 1) * sizeof(T));
	}

	/// Конструктор копирования
	simple_basic_string(const simple_basic_string& other)
	{
		size_ = other.size();
		ptr_ = new T[size_ + 1];
		std::memcpy(ptr_, other.c_str(), (size_ + 1) * sizeof(T));
	}

	/// Перемещающий конструктор
	simple_basic_string(simple_basic_string&& dying)
	{
		ptr_ = dying.ptr_;
		size_ = dying.size_;
		dying.ptr_ = new T[1];
		dying.ptr_[0] = '\0';
		dying.size_ = 0;
	}

	/// Деструктор
	~simple_basic_string() { clean_(); }

	/// Геттер на си-строку
	const T* c_str() const { return ptr_; }

	size_t size() const { return size_; }

	/// Оператор копирующего присваивания
	simple_basic_string& operator=(simple_basic_string&& other)
	{
		if (this == &other)
			return *this;
		clean_();
		ptr_ = other.ptr_;
		size_ = other.size_;
		other.ptr_ = new T[1];
		other.ptr_[0] = '\0';
		other.size_ = 0;

		return *this;
	}

	/// Оператор копирующего присваивания си строки
	simple_basic_string& operator=(const T* c_str)
	{
		size_t new_size = strlen_(c_str);
		T* new_ptr = new T[new_size + 1];
		std::memcpy(new_ptr, c_str, (new_size + 1) * sizeof(T));

		clean_();
		ptr_ = new_ptr;
		size_ = new_size;
		return *this;
	}

	/// Оператор копирующего присваивания
	simple_basic_string& operator=(const simple_basic_string& other)
	{
		size_t new_size = other.size_;
		T* new_ptr = new T[new_size + 1];
		std::memcpy(new_ptr, other.ptr_, (new_size + 1) * sizeof(T));
		clean_();
		ptr_ = new_ptr;
		size_ = new_size;
		return *this;
	}

	friend simple_basic_string<T> operator+(const simple_basic_string<T>& left,
											const simple_basic_string<T>& right)
	{
		simple_basic_string<T> result(left);
		result += right;
		return result;
	}

	// Модернизированный оператор << для вывода в поток, соответствующий типу
	// символов строки
	friend std::basic_ostream<T, std::char_traits<T>>& operator<<(
		std::basic_ostream<T, std::char_traits<T>>& os,
		const simple_basic_string<T>& obj)
	{
		if (obj.size_ > 0)
		{
			os.write(obj.ptr_, static_cast<std::streamsize>(obj.size_));
		}
		return os;
	}

	// Модернизированный оператор >> для чтения из потока, соответствующего типу
	// символов строки
	friend std::basic_istream<T, std::char_traits<T>>& operator>>(
		std::basic_istream<T, std::char_traits<T>>& is,
		simple_basic_string<T>& obj)
	{
		obj.clean_();
		T c;
		while (is.get(c))
		{
			obj += c;
		}
		return is;
	}
	simple_basic_string& operator+=(const simple_basic_string& other)
	{
		size_t new_size = size_ + other.size_;
		T* new_ptr = new T[new_size + 1];
		std::memcpy(new_ptr, ptr_, size_ * sizeof(T));
		std::memcpy(new_ptr + size_, other.ptr_, (other.size_ + 1) * sizeof(T));
		clean_();
		ptr_ = new_ptr;
		size_ = new_size;
		return *this;
	}

	simple_basic_string& operator+=(T symbol)
	{
		size_t new_size = size_ + 1;
		T* new_ptr = new T[new_size + 1];
		std::memcpy(new_ptr, ptr_, size_ * sizeof(T));
		new_ptr[size_] = symbol;
		new_ptr[size_ + 1] = '\0';
		clean_();
		ptr_ = new_ptr;
		size_ = new_size;
		return *this;
	}

	T& operator[](size_t index) noexcept { return *(ptr_ + index); }

	T& at(size_t index)
	{
		if (index >= size_)
		{
			throw std::out_of_range("Wrong index");
		}
		return *(ptr_ + index);
	}
	T* data() { return ptr_; }

   private:
	static size_t strlen_(const T* str)
	{
		size_t ans = 0;
		while (*str != '\0')
		{
			ans++;
			str++;
		}
		return ans;
	}

	void clean_()
	{
		delete[] ptr_;
		ptr_ = nullptr;
		size_ = 0;
	}

	T* ptr_ = nullptr;
	size_t size_;
};
}  // namespace bmstu
