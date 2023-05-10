// Copyright 2020 Robert Carneiro, Derek Meer, Matthew Tabak, Eric Lujan
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
// associated documentation files (the "Software"), to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge, publish, distribute,
// sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
// NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#ifndef GVECTOR_H_
#define GVECTOR_H_

// TODO: REPLACE ASSERT WITH GRESULT
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace shmea
{
template<typename T>
class GVector {
public:
	// TODO: Implement custom iterators
	typedef T* iterator;
	typedef const T* const_iterator;
	typedef unsigned int size_type;
private:
	T* m_data;
	size_type m_capacity;
	size_type m_size;
public:
	GVector() : m_size(0), m_capacity(0), m_data(0) {}

	GVector(size_type capacity) :
		m_size(0),
		m_capacity(capacity),
		m_data(new T[capacity]) {}

	GVector(size_type capacity, const T& value) :
		m_size(0),
		m_capacity(capacity),
		m_data(new T[capacity])
	{
		for(size_type i = 0; i < capacity; ++i)
			m_data[i] = value;
	}
	GVector(const GVector& value) :
		m_data(new T[value.m_capacity]),
		m_capacity(value.m_capacity),
		m_size(value.m_size)
	{
		(void)memcpy(m_data, value.m_data, value.m_size * sizeof(T));
	}
	/* virtual ~GVector() { delete [] m_data; } */

	iterator begin() { return m_data; }
	const_iterator cbegin() const { return m_data; }
	iterator end() { return &m_data[this->size()]; }
	const_iterator cend() const { return &m_data[this->size()]; }

	size_type max_size() const { return 0 - 1; }
	size_type size() const { return m_size; }
	size_type capacity() const { return m_capacity; }
	bool empty() const { return m_size == 0; }

	void reserve(size_type new_cap)
	{
		if (new_cap <= m_capacity) return;


	}
	void clear() { m_size = 0; }
	void push_back(T newValue)
	{
		if (m_size == m_capacity) this->expand();
		m_data[this->size()] = newValue;
		m_size++;
	}
	T pop_back() { return m_data[--m_size]; }
	void erase(size_type idx)
	{
		if (empty())
			return;

		assert(idx <= m_size);
		if (m_size == 1)
		{
			this->clear();
			return;
		}

		if (idx == m_size)
		{
			this->pop_back();
		}
		else
		{
			// NOTE: the allocator should be responsible for this
			(void)memmove(&m_data[idx],
					&m_data[idx+1],
					sizeof(T) * (m_size - idx - 1));// because we remove 1
			m_size--;
		}
	}

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
			(void)memmove(&m_data[idx + 1],
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
	void expand()
	{
		if (m_capacity == 0)
		{
			m_data = new T[1];
			m_capacity = 1;
			return;
		}
		// NOTE: the allocator should be responsible for this
		/* T* newData = (T*)realloc(m_data, 2 * m_capacity * sizeof(T)); */
		/* T* newData = new T[2 * m_capacity]; */

		/* printf("cap: %d, size: %d, \n", m_capacity, m_size); */
		T* oldBuffer = m_data;
		T* newBuffer = new T[2 * m_capacity];
		memcpy(newBuffer, oldBuffer, m_size * sizeof(T));
		/* for (size_type i = 0; i < m_size; i++) */
		/* { */
		/* 	newBuffer[i] = oldBuffer[i]; */
		/* } */

		m_data = newBuffer;
		/* if (newBuffer != oldBuffer) delete [] oldBuffer; */
		m_capacity *= 2;
		/* printf("cap: %d, size: %d, \n", m_capacity, m_size); */
	}
};
}
#endif // !GVECTOR_H_