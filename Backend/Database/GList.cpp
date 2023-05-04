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
#include "GList.h"
#include "GResult.h"
#include "GString.h"
#include "GType.h"

using namespace shmea;

GList::GList()
{
	//
}

GList::GList(const GList& list2)
{
	copy(list2);
}

GList::GList(int size, const GType& value)
{
	items = std::vector<GType>(size, value);
}

GList::~GList()
{
	clear();
}

void GList::copy(const GList& list2)
{
	items = list2.items;
}

void GList::addChar(char newBlock)
{
	addPrimitive(GType::CHAR_TYPE, &newBlock);
}

GResult<void> GList::insertChar(unsigned int index, char newBlock)
{
	return insertPrimitive(index, GType::CHAR_TYPE, &newBlock);
}

void GList::addShort(short newBlock)
{
	addPrimitive(GType::SHORT_TYPE, &newBlock);
}

GResult<void> GList::insertShort(unsigned int index, short newBlock)
{
	return insertPrimitive(index, GType::SHORT_TYPE, &newBlock);
}

void GList::addInt(int newBlock)
{
	addPrimitive(GType::INT_TYPE, &newBlock);
}

GResult<void> GList::insertInt(unsigned int index, int newBlock)
{
	return insertPrimitive(index, GType::INT_TYPE, &newBlock);
}

void GList::addLong(int64_t newBlock)
{
	addPrimitive(GType::LONG_TYPE, &newBlock);
}

GResult<void> GList::insertLong(unsigned int index, int64_t newBlock)
{
	return insertPrimitive(index, GType::LONG_TYPE, &newBlock);
}

void GList::addFloat(float newBlock)
{
	addPrimitive(GType::FLOAT_TYPE, &newBlock);
}

GResult<void> GList::insertFloat(unsigned int index, float newBlock)
{
	return insertPrimitive(index, GType::FLOAT_TYPE, &newBlock);
}

void GList::addDouble(double newBlock)
{
	addPrimitive(GType::DOUBLE_TYPE, &newBlock);
}

GResult<void> GList::insertDouble(unsigned int index, double newBlock)
{
	return insertPrimitive(index, GType::DOUBLE_TYPE, &newBlock);
}

void GList::addBoolean(bool newBlock)
{
	addPrimitive(GType::BOOLEAN_TYPE, &newBlock);
}

GResult<void> GList::insertBoolean(unsigned int index, bool newBlock)
{
	return insertPrimitive(index, GType::BOOLEAN_TYPE, &newBlock);
}

void GList::addPrimitive(int newType, const void* newBlock)
{
	insertPrimitive(items.size(), newType, newBlock);
}

GResult<void> GList::insertPrimitive(unsigned int index, int newType, const void* newBlock)
{
	int64_t newBlockSize = 0;
	if (newType == GType::CHAR_TYPE)
		newBlockSize = sizeof(char);
	else if (newType == GType::SHORT_TYPE)
		newBlockSize = sizeof(short);
	else if (newType == GType::INT_TYPE)
		newBlockSize = sizeof(int);
	else if (newType == GType::LONG_TYPE)
		newBlockSize = sizeof(int64_t);
	else if (newType == GType::FLOAT_TYPE)
		newBlockSize = sizeof(float);
	else if (newType == GType::DOUBLE_TYPE)
		newBlockSize = sizeof(double);
	else if (newType == GType::BOOLEAN_TYPE)
		newBlockSize = sizeof(bool);

	// Add the object if its valid
	if (newBlockSize > 0)
		return insertObject(index, newType, newBlock, newBlockSize);
	else
		return result::ERROR_INSERTION_FAILURE;
}

void GList::addString(const GString& newBlock)
{
	insertObject(items.size(), GType::STRING_TYPE, (void*)newBlock.c_str(), newBlock.length());
}

GResult<void> GList::insertString(unsigned int index, const GString& newBlock)
{
	return insertObject(index, GType::STRING_TYPE, (void*)newBlock.c_str(), newBlock.length());
}

void GList::addString(const char* newBlock)
{
	if (newBlock != NULL)
	{
		GString newStr(newBlock);
		insertString(items.size(), newStr);
	}
}

GResult<void> GList::insertString(unsigned int index, const char* newBlock)
{
	if (newBlock != NULL)
	{
		GString newStr(newBlock);
		return insertString(index, newStr);
	}
	return result::ERROR_NULL_POINTER;
}

void GList::addObject(int newType, const void* newBlock, int64_t newBlockSize)
{
	insertObject(items.size(), newType, newBlock, newBlockSize);
}

// All add & insert functions go to this one
GResult<void> GList::insertObject(unsigned int index, int newType, const void* newBlock,
						 int64_t newBlockSize)
{
	// OLD fix the index if need be
	// if (index > items.size())
	// 	index = items.size();
	if (index > items.size())
		return result::ERROR_OUT_OF_RANGE;

	// Add the bundle item
	GType newItem(newType, newBlock, newBlockSize);
	return insertGType(index, newItem);
}

/*!
 * @brief add GType
 * @details add a GType to the list of items
 * @param item the GType item to add
 */
void GList::addGType(const GType& item)
{
	items.push_back(item);
}

/*!
 * @brief insert GType
 * @details add a GType to the list of items at a certain position
 * @param index the index at which to add the item
 * @param item the GType item to add
 */
GResult<void> GList::insertGType(unsigned int index, const GType& item)
{
	if (index == items.size())
	{
		addGType(item);
		return result::SUCCESS;
	}
	else
	{
		items.insert(items.begin() + index, item);
		return result::SUCCESS;
	}
}

GResult<void> GList::setGType(unsigned int index, const GType& item)
{
	if (index >= items.size())
		return result::ERROR_OUT_OF_RANGE;

	items[index] = item;
	return result::SUCCESS;
}

GResult<void> GList::remove(unsigned int index)
{
	if (index >= items.size())
		return result::ERROR_OUT_OF_RANGE;

	items.erase(items.begin() + index);
	return result::SUCCESS;
}

void GList::clear()
{
	items.clear();
}

GResult<GString> GList::getString(unsigned int index) const
{
	if (index >= items.size())
		return result::ERROR_OUT_OF_RANGE;

	if (items[index].size() == 0)
		return GString("");

	return GString(items[index]);
}

GResult<const char*> GList::c_str(unsigned int index) const
{
	if (index >= items.size())
		return result::ERROR_OUT_OF_RANGE;

	if (items[index].size() <= 0)
		return "";

	return items[index].c_str();
}

GResult<char> GList::getChar(unsigned int index) const
{
	if (index >= items.size())
		return result::ERROR_OUT_OF_RANGE;

	if (items[index].size() != sizeof(char))
		return result::ERROR_TYPE_ERROR;

	return items[index].getChar();
}

GResult<short> GList::getShort(unsigned int index) const
{
	if (index >= items.size())
		return result::ERROR_OUT_OF_RANGE;

	if (items[index].size() != sizeof(short))
		return result::ERROR_TYPE_ERROR;

	return items[index].getShort();
}

GResult<int> GList::getInt(unsigned int index) const
{
	if (index >= items.size())
		return result::ERROR_OUT_OF_RANGE;

	if (items[index].size() != sizeof(int))
		return result::ERROR_TYPE_ERROR;

	return items[index].getInt();
}

GResult<int64_t> GList::getLong(unsigned int index) const
{
	if (index >= items.size())
		return result::ERROR_OUT_OF_RANGE;

	if (items[index].size() != sizeof(int64_t))
		return result::ERROR_TYPE_ERROR;

	return items[index].getLong();
}

GResult<float> GList::getFloat(unsigned int index) const
{
	if (index >= items.size())
		return result::ERROR_OUT_OF_RANGE;

	if (items[index].size() != sizeof(float))
		return result::ERROR_TYPE_ERROR;

	return items[index].getFloat();
}

GResult<double> GList::getDouble(unsigned int index) const
{
	if (index >= items.size())
		return result::ERROR_OUT_OF_RANGE;

	if (items[index].size() != sizeof(double))
		return result::ERROR_TYPE_ERROR;

	return items[index].getDouble();
}

GResult<bool> GList::getBoolean(unsigned int index) const
{
	if (index >= items.size())
		return result::ERROR_OUT_OF_RANGE;

	if (items[index].size() != sizeof(bool))
		return result::ERROR_TYPE_ERROR;

	return items[index].getBoolean();
}

GResult<GType> GList::getGType(unsigned int index) const
{
	if (index >= items.size())
		return result::ERROR_OUT_OF_RANGE;

	return items[index];
}

GResult<int> GList::getType(unsigned int index) const
{
	if (index >= items.size())
		return result::ERROR_OUT_OF_RANGE;

	return items[index].getType();
}

unsigned int GList::size() const
{
	return items.size();
}

bool GList::empty() const
{
	return !(size() > 0);
}

/*!
 * @brief standardize GList
 * @details standardize the values in a GList; that is, map the values from their existing range to
 * the range of -1.0 to 1.0
 */
void GList::standardize()
{
	// Standardize the initialization of the weights
	if (size() <= 0)
		return;

	// Set the min and max of the weights
	xMin = 0.0f;
	xMax = 0.0f;

	// iterate through the rows
	for (unsigned int r = 0; r < size(); ++r)
	{
		GType cCell = getGType(r).unwrap(); // panics on error
		float cell = 0.0f;

		switch (cCell.getType()) {
			case GType::STRING_TYPE:
				// OHE: total unique words
				break;
			case GType::CHAR_TYPE:
				cell = cCell.getChar();
				break;
			case GType::SHORT_TYPE:
				cell = cCell.getShort();
				break;
			case GType::INT_TYPE:
				cell = cCell.getInt();
				break;
			case GType::LONG_TYPE:
				cell = cCell.getLong();
				break;
			case GType::FLOAT_TYPE:
				cell = cCell.getFloat();
				break;
			case GType::DOUBLE_TYPE:
				cell = cCell.getDouble();
				break;
			case GType::BOOLEAN_TYPE:
				cell = cCell.getBoolean() ? 1.0f : 0.0f;
				break;
		}

		if (r == 0)
		{
			xMin = cell;
			xMax = cell;
		}

		// Check the mins and maxes
		if (cell < xMin)
			xMin = cell;
		if (cell > xMax)
			xMax = cell;
	}

	// standardize the weights
	xRange = xMax - xMin;
	if (xRange == 0.0f)
		return;
	// iterate through the rows
	for (unsigned int r = 0; r < size(); ++r)
	{
		// Adjust the children
		GType cCell = getGType(r).unwrap(); // panics on error
		float cell = 0.0f;
		if (cCell.getType() == GType::STRING_TYPE)
		{
			// OHE: total unique words
		}
		else if (cCell.getType() == GType::CHAR_TYPE)
		{
			cell = cCell.getChar();
			cell = (((cell - xMin) / (xRange)) - 0.5f);
			cCell.set(GType::FLOAT_TYPE, &cell, sizeof(float));
		}
		else if (cCell.getType() == GType::SHORT_TYPE)
		{
			cell = cCell.getShort();
			cell = (((cell - xMin) / (xRange)) - 0.5f);
			cCell.set(GType::FLOAT_TYPE, &cell, sizeof(float));
		}
		else if (cCell.getType() == GType::INT_TYPE)
		{
			cell = cCell.getInt();
			cell = (((cell - xMin) / (xRange)) - 0.5f);
			cCell.set(GType::FLOAT_TYPE, &cell, sizeof(float));
		}
		else if (cCell.getType() == GType::LONG_TYPE)
		{
			cell = cCell.getLong();
			cell = (((cell - xMin) / (xRange)) - 0.5f);
			cCell.set(GType::FLOAT_TYPE, &cell, sizeof(float));
		}
		else if (cCell.getType() == GType::FLOAT_TYPE)
		{
			cell = cCell.getFloat();
			cell = (((cell - xMin) / (xRange)) - 0.5f);
			cCell.set(GType::FLOAT_TYPE, &cell, sizeof(float));
		}
		else if (cCell.getType() == GType::DOUBLE_TYPE)
		{
			cell = cCell.getDouble();
			cell = (((cell - xMin) / (xRange)) - 0.5f);
			cCell.set(GType::FLOAT_TYPE, &cell, sizeof(float));
		}
		else if (cCell.getType() == GType::BOOLEAN_TYPE)
		{
			cell = cCell.getBoolean() ? 1.0f : 0.0f; // 1 or 0 if sigmoid
			cell = (((cell - xMin) / (xRange)) - 0.5f);
			cCell.set(GType::FLOAT_TYPE, &cell, sizeof(float));
		}
	}
}

void GList::print() const
{
	for (unsigned int i = 0; i < size(); ++i)
	{
		if (items[i].getType() == GType::STRING_TYPE)
			printf("%s", items[i].c_str());
		else if (items[i].getType() == GType::CHAR_TYPE)
			printf("%c", items[i].getChar());
		else if (items[i].getType() == GType::SHORT_TYPE)
			printf("%d", items[i].getShort());
		else if (items[i].getType() == GType::INT_TYPE)
			printf("%d", items[i].getInt());
		else if (items[i].getType() == GType::LONG_TYPE)
			printf("%ld", items[i].getLong());
		else if (items[i].getType() == GType::FLOAT_TYPE)
			printf("%f", items[i].getFloat());
		else if (items[i].getType() == GType::DOUBLE_TYPE)
			printf("%f", items[i].getDouble());
		else if (items[i].getType() == GType::BOOLEAN_TYPE)
			printf("%s", items[i].getBoolean() ? "True" : "False");

		if (i < size() - 1)
			printf(",");
	}

	printf("\n");
}

GType GList::operator[](unsigned int index)
{
	if (index >= items.size())
		return GType();

	return items[index];
}

const GType GList::operator[](unsigned int index) const
{
	if (index >= items.size())
		return GType();

	return items[index];
}

void GList::operator=(const GList& list2)
{
	copy(list2);
}