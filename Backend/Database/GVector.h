#ifndef GVECTOR_H_
#define GVECTOR_H_

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace shmea
{
template<typename T>
class GVector {
public:
	typedef T* iterator;
	typedef const T* const_iterator;
	typedef unsigned long size_type;

	GVector() : m_size(0), m_capacity(0), m_data(0) {}
	GVector(size_type capacity) :
		m_size(0),
		m_capacity(capacity),
		m_data(new T[capacity]) {}
	GVector(const GVector& value) :
		m_data(new T[value.m_capacity]),
		m_capacity(value.m_capacity),
		m_size(value.m_size)
	{
		memcpy(m_data, value.m_data, value.m_size * sizeof(T));
	}
	~GVector() { delete [] m_data; }

	iterator begin() { return m_data; }
	const_iterator cbegin() const { return m_data; }
	iterator end() { return &m_data[this->size()]; }
	const_iterator cend() const { return &m_data[this->size()]; }

	size_type max_size() { return 0 - 1; }
	size_type size() { return m_size; }
	size_type capacity() { return m_capacity; }
	bool empty() { return m_size == 0; }

	void clear() { m_size = 0; }
	void push_back(T newValue)
	{
		if (m_size == m_capacity) this->expand();
		m_data[this->size()] = newValue;
		m_size++;
	}
	T pop_back() { return m_data[--m_size]; }
	void insert(size_type idx, T value)
	{
		assert(idx <= m_size);
		if (idx == m_size)
		{
			this->push_back(value);
		}
		else
		{
			// NOTE: the allocator should be responsible for this
			memmove(&m_data[idx + 1],
					&m_data[idx],
					sizeof(T) * (m_size - idx));
			m_data[idx] = value;
			m_size++;
		}
	}

	T& at(size_type idx)
	{
		assert(idx < m_size);
		return m_data[idx];
	}
	const T& at(size_type idx) const
	{
		assert(idx < m_size);
		return m_data[idx];
	}
	T& operator[](size_type idx) { return at(idx); }
	const T& operator[](size_type idx) const { return at(idx); }
private:
	T* m_data;
	size_type m_size;
	size_type m_capacity;
	void expand()
	{
		if (m_capacity == 0)
		{
			m_data = new T[1];
			m_capacity = 1;
			return;
		}
		// NOTE: the allocator should be responsible for this
		// TODO: Implement custom iterators
		realloc(m_data, 2 * m_capacity);
		m_capacity *= 2;
	}
};
}
#endif // !GVECTOR_H_