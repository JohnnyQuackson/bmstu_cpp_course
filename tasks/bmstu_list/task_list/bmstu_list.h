#include <cstddef>
#include <iterator>
#include <ostream>
#include "abstract_iterator.h"

namespace bmstu
{
template <typename T>
class list
{
	struct node
	{
		node() = default;

		node(node* prev, const T& value, node* next)
			: value_(value), next_node_(next), prev_node_(prev)
		{
		}

		T value_;
		node* next_node_ = nullptr;
		node* prev_node_ = nullptr;
	};

   public:
	template <typename ValueType>
	struct base_iterator
		: public abstract_iterator<base_iterator<ValueType>,
								   ValueType,
								   std::bidirectional_iterator_tag>
	{
		using abstract_iterator_father =
			abstract_iterator<base_iterator<ValueType>,
							  ValueType,
							  std::bidirectional_iterator_tag>;

		node* current;

		base_iterator() : current(nullptr) {}

		base_iterator(node* n) : current(n) {}
		base_iterator& operator++() override
		{
			current = current->next_node_;
			return *this;
		}
		base_iterator& operator--() override
		{
			current = current->prev_node_;
			return *this;
		}
		base_iterator operator++(int) override
		{
			auto tmp(current);
			current = current->next_node_;
			return tmp;
		}
		base_iterator operator--(int) override
		{
			auto tmp(current);
			current = current->prev_node_;
			return tmp;
		}
		base_iterator& operator+=(
			const typename abstract_iterator_father::difference_type& n)
			override
		{
			if (n > 0)
			{
				for (auto i = 0; i < n; ++i)
				{
					++(*this);
				}
			}
			else if (n < 0)
			{
				for (auto i = 0; i > n; --i)
				{
					--(*this);
				}
			}
			return *this;
		}
		base_iterator& operator-=(
			const typename abstract_iterator_father::difference_type& n)
			override
		{
			if (n > 0)
			{
				for (auto i = 0; i < n; ++i)
				{
					--(*this);
				}
			}
			else if (n < 0)
			{
				for (auto i = 0; i > n; --i)
				{
					++(*this);
				}
			}
			return *this;
		}
		base_iterator operator+(
			const abstract_iterator_father::difference_type& n) const override
		{
			auto tmp = *this;
			tmp += n;
			return tmp;
		}
		base_iterator operator-(
			const abstract_iterator_father::difference_type& n) const override
		{
			auto tmp = *this;
			tmp -= n;
			return tmp;
		}

		abstract_iterator_father::reference operator*() const override
		{
			return current->value_;
		}

		abstract_iterator_father::pointer operator->() const override
		{
			return &(current->value_);
		}

		bool operator==(const base_iterator& other) const override
		{
			return current == other.current;
		}

		bool operator!=(const base_iterator& other) const override
		{
			return current != other.current;
		}

		template <typename OtherValueType>
		bool operator==(const base_iterator<OtherValueType>& other) const
		{
			return current == other.current;
		}
		template <typename OtherValueType>
		bool operator!=(const base_iterator<OtherValueType>& other) const
		{
			return current != other.current;
		}
		explicit operator bool() const override { return current != nullptr; }

		using diff_t = typename abstract_iterator_father::difference_type;

		diff_t operator-(const base_iterator& other) const override
		{
			if (*this == other)
				return 0;
			diff_t n = 0;
			auto tmp_for = *this;
			auto tmp_back = *this;
			while (true)
			{
				++n;
				if (tmp_back.current != nullptr)
				{
					--tmp_back;
					if (tmp_back == other)
					{
						return n;
					}
				}
				if (tmp_for.current != nullptr)
				{
					++tmp_for;
					if (tmp_for == other)
					{
						return -n;
					}
				}
			}
		}
	};

	struct iterator : public base_iterator<T>
	{
		using base_iterator<T>::base_iterator;
	};

	struct const_iterator : public base_iterator<const T>
	{
		const_iterator(const iterator& other)
			: base_iterator<const T>(other.current)
		{
		}
		using base_iterator<const T>::base_iterator;
	};

	list()
	{
		head_ = new node();
		tail_ = new node();
		head_->next_node_ = tail_;
		tail_->prev_node_ = head_;
		size_ = 0;
	}

	template <typename it>
	list(it begin, it end) : list()
	{
		it now = begin;
		while (now != end)
		{
			push_back(*now);
			++now;
		}
	}

	list(std::initializer_list<T> values) : list()
	{
		for (T c : values)
		{
			push_back(c);
		}
	}

	list(const list& other) : list()
	{
		node* now = other.head_->next_node_;
		while (now != other.tail_)
		{
			push_back(now->value_);
			now = now->next_node_;
		}
	}

	list(list&& other) : list() { swap(other); }

#pragma endregion
#pragma region pushs

	template <typename Type>
	void push_back(const Type& value)
	{
		node* last = tail_->prev_node_;
		node* new_last = new node(last, value, tail_);
		tail_->prev_node_ = new_last;
		last->next_node_ = new_last;
		++size_;
	}

	template <typename Type>
	void push_front(const Type& value)
	{
		node* first = head_->next_node_;
		node* new_first = new node(head_, value, first);
		head_->next_node_ = new_first;
		first->prev_node_ = new_first;
		++size_;
	}

#pragma endregion

	bool empty() const

		noexcept
	{
		return (size_ == 0u);
	}

	~list()
	{
		clear();
		delete head_;
		delete tail_;
	}

	void clear()
	{
		node* now = head_->next_node_;
		while (now != tail_)
		{
			now = now->next_node_;
			delete now->prev_node_;
		}
		head_->next_node_ = tail_;
		tail_->prev_node_ = head_;
		size_ = 0;
	}

	size_t size() const { return size_; }

	void swap(list& other)

		noexcept
	{
		node* tmp;
		tmp = other.head_;
		other.head_ = head_;
		head_ = tmp;
		tmp = other.tail_;
		other.tail_ = tail_;
		tail_ = tmp;
		size_t tmp_size;
		tmp_size = other.size_;
		other.size_ = size_;
		size_ = tmp_size;
	}

	friend void swap(list& l, list& r) { l.swap(r); }

#pragma region iterators

	iterator begin()

		noexcept
	{
		return iterator{head_->next_node_};
	}

	iterator end()

		noexcept
	{
		return iterator{tail_};
	}

	const_iterator begin() const

		noexcept
	{
		return const_iterator{head_->next_node_};
	}

	const_iterator end() const

		noexcept
	{
		return const_iterator{tail_};
	}

	const_iterator cbegin() const

		noexcept
	{
		return const_iterator{head_->next_node_};
	}

	const_iterator cend() const

		noexcept
	{
		return const_iterator{tail_};
	}

#pragma endregion

	T& operator[](size_t pos)
	{
		auto cur = begin();
		for (size_t i = 0; i < pos; i++)
		{
			++cur;
		}
		return *cur;
	}

	const T& operator[](size_t pos) const
	{
		auto cur = begin();
		for (size_t i = 0; i < pos; i++)
		{
			++cur;
		}
		return *cur;
	}

	friend bool operator==(const list& l, const list& r)
	{
		if (l.size_ != r.size_)
			return false;
		size_t tmp_size = l.size_;
		const_iterator it_l = l.begin(), it_r = r.begin();
		for (size_t i = 0; i < tmp_size; i++)
		{
			if (*it_l != *it_r)
			{
				return false;
			}
			++it_l;
			++it_r;
		}
		return true;
	}

	friend bool operator!=(const list& l, const list& r) { return !(l == r); }

	friend auto operator<=>(const list& lhs, const list& rhs)
	{
		return lexicographical_compare_(lhs, rhs);
	}

	friend std::ostream& operator<<(std::ostream& os, const list& other)
	{
		os << '{';
		if (other.size_ == 0)
		{
			os << '}';
			return os;
		}
		const_iterator now = other.begin(), end = other.end();
		--end;
		while (now != end)
		{
			os << *now << ", ";
			++now;
		}
		os << *now << '}';
		return os;
	}

	iterator insert(const_iterator pos, const T& value)
	{
		node* now = pos.current;
		node* new_node = new node(now->prev_node_, value, now);
		now->prev_node_->next_node_ = new_node;
		now->prev_node_ = new_node;
		++size_;
		return iterator{new_node};
	}

	list& reverse()
	{
		node* cur_left = head_->next_node_;
		node* cur_right = tail_->prev_node_;
		node* tmp1;
		node* tmp2;
		for (size_t i = 0; i < size_ / 2; i++)
		{
			tmp1 = cur_left->next_node_;
			tmp2 = cur_left->prev_node_;

			cur_left->next_node_ = cur_right->next_node_;
			cur_right->next_node_->prev_node_ = cur_left;
			cur_left->prev_node_ = cur_right->prev_node_;
			cur_right->prev_node_->next_node_ = cur_left;

			cur_right->next_node_ = tmp1;
			cur_right->prev_node_ = tmp2;
			tmp1->prev_node_ = cur_right;
			tmp2->next_node_ = cur_right;

			cur_left = cur_right->next_node_;
			cur_right = cur_left->prev_node_;
		}
		return *this;
	}

   private:
	static auto lexicographical_compare_(const list<T>& lhs, const list<T>& rhs)
	{
		size_t min_size = std::min(lhs.size_, rhs.size_);
		auto cur_lhs = lhs.begin(), cur_rhs = rhs.begin();
		for (size_t i = 0; i < min_size; i++)
		{
			if (*cur_lhs != *cur_rhs)
			{
				return (*cur_lhs <=> *cur_rhs);
			}
			++cur_lhs;
			++cur_rhs;
		}

		return lhs.size_ <=> rhs.size_;
	}

	size_t size_ = 0;
	node* tail_ = nullptr;
	node* head_ = nullptr;
};
}  // namespace bmstu