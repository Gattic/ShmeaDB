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
#ifndef _GLIST
#define _GLIST

#include "GResult.h"
#include "GType.h"
#include "GString.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

namespace shmea {
class Serializable;

class GList
{
	friend class Serializable;

private:
	//
	std::vector<GType> items;
	float xMin;
	float xMax;
	float xRange;

	//
	void addPrimitive(int, const void*);
	void addObject(int, const void*, int64_t);
	GResult<void> insertPrimitive(unsigned int, int, const void*);
	GResult<void> insertObject(unsigned int, int, const void*, int64_t);

public:
	GList();
	GList(const GList&);
	GList(int, const GType&);
	virtual ~GList();

	// sets
	void copy(const GList&);

	void addChar(char);
	GResult<void> insertChar(unsigned int, char);

	void addShort(short);
	GResult<void> insertShort(unsigned int, short);

	void addInt(int);
	GResult<void> insertInt(unsigned int, int);

	void addLong(int64_t);
	GResult<void> insertLong(unsigned int, int64_t);

	void addFloat(float);
	GResult<void> insertFloat(unsigned int, float);

	void addDouble(double);
	GResult<void> insertDouble(unsigned int, double);

	void addBoolean(bool);
	GResult<void> insertBoolean(unsigned int, bool);

	void addString(const GString&);
	GResult<void> insertString(unsigned int, const GString&);

	void addString(const char*);
	GResult<void> insertString(unsigned int, const char*);

	void addGType(const GType&);
	GResult<void> insertGType(unsigned int, const GType&);
	GResult<void> setGType(unsigned int, const GType&);

	GResult<void> remove(unsigned int);
	void clear();

	// gets
	GResult<GString> getString(unsigned int) const;
	GResult<const char*> c_str(unsigned int) const;
	GResult<char> getChar(unsigned int) const;
	GResult<short> getShort(unsigned int) const;
	GResult<int> getInt(unsigned int) const;
	GResult<int64_t> getLong(unsigned int) const;
	GResult<float> getFloat(unsigned int) const;
	GResult<double> getDouble(unsigned int) const;
	GResult<bool> getBoolean(unsigned int) const;
	GResult<GType> getGType(unsigned int) const;
	GResult<int> getType(unsigned int) const;

	//
	unsigned int size() const;
	bool empty() const;
	void standardize();
	float unstandardize(float) const;
	void print() const;

	// operators
	GType operator[](unsigned int);
	const GType operator[](unsigned int) const;
	void operator=(const GList&);
};
};

#endif