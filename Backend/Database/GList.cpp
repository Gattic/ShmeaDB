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
	//items = GVector<GType>(size, value);
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

void GList::loadWords(const GString& fname)
{
	if (fname.length() == 0)
		return;

	FILE* fd = fopen(fname.c_str(), "ro");
	printf("[WORDS] %c%s\n", (fd != NULL) ? '+' : '-', fname.c_str());

	if (!fd)
		return;

	// Allocate a buffer
	int MAX_LINE_SIZE = 1024;
	char buffer[MAX_LINE_SIZE];
	char *ptr = NULL;

	shmea::GList newRow;
	bzero(buffer, MAX_LINE_SIZE);
	while( !feof( fd ) )
	{
		fgets(&buffer[0], MAX_LINE_SIZE, fd);
		GString delim_char(' '); //delimiter

		if (!feof(fd))
		{
			buffer[strlen(buffer)-1] = '\0';
			ptr = strtok(buffer, (const char*)delim_char.c_str());
			while (ptr)
			{
				GString word(ptr);
				newRow.addString(word.makeAlphaNum().toLower());

				// Get the next token
				ptr = strtok( NULL, (const char *)delim_char.c_str() );
			}
		}
	}

	printf( "[CSV] %d cells of data\n", newRow.size());
	copy(newRow);

	// EOF
	fclose(fd);
}

void GList::addChar(char newBlock)
{
	addPrimitive(GType::CHAR_TYPE, &newBlock);
}

void GList::insertChar(unsigned int index, char newBlock)
{
	insertPrimitive(index, GType::CHAR_TYPE, &newBlock);
}

void GList::addShort(short newBlock)
{
	addPrimitive(GType::SHORT_TYPE, &newBlock);
}

void GList::insertShort(unsigned int index, short newBlock)
{
	insertPrimitive(index, GType::SHORT_TYPE, &newBlock);
}

void GList::addInt(int newBlock)
{
	addPrimitive(GType::INT_TYPE, &newBlock);
}

void GList::insertInt(unsigned int index, int newBlock)
{
	insertPrimitive(index, GType::INT_TYPE, &newBlock);
}

void GList::addLong(int64_t newBlock)
{
	addPrimitive(GType::LONG_TYPE, &newBlock);
}

void GList::insertLong(unsigned int index, int64_t newBlock)
{
	insertPrimitive(index, GType::LONG_TYPE, &newBlock);
}

void GList::addFloat(float newBlock)
{
	addPrimitive(GType::FLOAT_TYPE, &newBlock);
}

void GList::insertFloat(unsigned int index, float newBlock)
{
	insertPrimitive(index, GType::FLOAT_TYPE, &newBlock);
}

void GList::addDouble(double newBlock)
{
	addPrimitive(GType::DOUBLE_TYPE, &newBlock);
}

void GList::insertDouble(unsigned int index, double newBlock)
{
	insertPrimitive(index, GType::DOUBLE_TYPE, &newBlock);
}

void GList::addBoolean(bool newBlock)
{
	addPrimitive(GType::BOOLEAN_TYPE, &newBlock);
}

void GList::insertBoolean(unsigned int index, bool newBlock)
{
	insertPrimitive(index, GType::BOOLEAN_TYPE, &newBlock);
}

void GList::addPrimitive(GType::Type newType, const void* newBlock)
{
	insertPrimitive(items.size(), newType, newBlock);
}

void GList::insertPrimitive(unsigned int index, GType::Type newType, const void* newBlock)
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
		insertObject(index, newType, newBlock, newBlockSize);
}

void GList::addString(const GString& newBlock)
{
	insertObject(items.size(), GType::STRING_TYPE, (void*)newBlock.c_str(), newBlock.length());
}

void GList::insertString(unsigned int index, const GString& newBlock)
{
	insertObject(index, GType::STRING_TYPE, (void*)newBlock.c_str(), newBlock.length());
}

void GList::addString(const char* newBlock)
{
	if (newBlock != NULL)
	{
		GString newStr(newBlock);
		insertString(items.size(), newStr);
	}
}

void GList::insertString(unsigned int index, const char* newBlock)
{
	if (newBlock != NULL)
	{
		GString newStr(newBlock);
		insertString(index, newStr);
	}
}

void GList::addObject(GType::Type newType, const void* newBlock, int64_t newBlockSize)
{
	insertObject(items.size(), newType, newBlock, newBlockSize);
}

// All add & insert functions go to this one
void GList::insertObject(unsigned int index, GType::Type newType, const void* newBlock,
						 int64_t newBlockSize)
{
	// fix the index if need be
	if (index > items.size())
		index = items.size();

	// Add the bundle item
	GType newItem(newType, newBlock, newBlockSize);
	if (index == items.size())
		addGType(newItem);
	else // insert
		insertGType(index, newItem);
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
void GList::insertGType(unsigned int index, const GType& item)
{
	if (index == items.size())
		addGType(item);
	else
		items.insert(items.begin() + index, item);//TODO: FIX after iterators
		//items.insert(index, item); // GVector
}

void GList::setGType(unsigned int index, const GType& item)
{
	if (index >= items.size())
		return;

	items[index] = item;
}

void GList::remove(unsigned int index)
{
	if (index >= items.size())
		return;

	items.erase(items.begin() + index);//TODO: FIX after iterators
	//items.erase(index); // GVector
}

void GList::clear()
{
	items.clear();
}

GString GList::getString(unsigned int index) const
{
	if (index >= items.size())
		return "";

	if (items[index].size() == 0)
		return "";

	return items[index];
}

const char* GList::c_str(unsigned int index) const
{
	if (index >= items.size())
		return "";

	if (items[index].size() <= 0)
		return "";

	return items[index].c_str();
}

char GList::getChar(unsigned int index) const
{
	if (index >= items.size())
		return 0;

	if (items[index].size() != sizeof(char))
		return 0;

	return items[index].getChar();
}

short GList::getShort(unsigned int index) const
{
	if (index >= items.size())
		return 0;

	if (items[index].size() != sizeof(short))
		return 0;

	return items[index].getShort();
}

int GList::getInt(unsigned int index) const
{
	if (index >= items.size())
		return 0;

	if (items[index].size() != sizeof(int))
		return 0;

	return items[index].getInt();
}

int64_t GList::getLong(unsigned int index) const
{
	if (index >= items.size())
		return 0;

	if (items[index].size() != sizeof(int64_t))
		return 0;

	return items[index].getLong();
}

float GList::getFloat(unsigned int index) const
{
	if (index >= items.size())
		return 0.0f;

	if (items[index].size() != sizeof(float))
		return 0.0f;

	return items[index].getFloat();
}

double GList::getDouble(unsigned int index) const
{
	if (index >= items.size())
		return 0.0f;

	if (items[index].size() != sizeof(double))
		return 0.0f;

	return items[index].getDouble();
}

bool GList::getBoolean(unsigned int index) const
{
	if (index >= items.size())
		return false;

	if (items[index].size() != sizeof(bool))
		return false;

	return items[index].getBoolean();
}

GType GList::getGType(unsigned int index) const
{
	if (index >= items.size())
		return GType();

	return items[index];
}

int GList::getType(unsigned int index) const
{
	if (index >= items.size())
		return GType::NULL_TYPE;

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
		GType cCell = getGType(r);
		float cell = 0.0f;
		if (cCell.getType() == GType::STRING_TYPE)
		{
			// OHE: total unique words
		}
		else if (cCell.getType() == GType::CHAR_TYPE)
			cell = cCell.getChar();
		else if (cCell.getType() == GType::SHORT_TYPE)
			cell = cCell.getShort();
		else if (cCell.getType() == GType::INT_TYPE)
			cell = cCell.getInt();
		else if (cCell.getType() == GType::LONG_TYPE)
			cell = cCell.getLong();
		else if (cCell.getType() == GType::FLOAT_TYPE)
			cell = cCell.getFloat();
		else if (cCell.getType() == GType::DOUBLE_TYPE)
			cell = cCell.getDouble();
		else if (cCell.getType() == GType::BOOLEAN_TYPE)
			cell = cCell.getBoolean() ? 1.0f : 0.0f;

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
		GType cCell = getGType(r);
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
		items[r] = cCell;
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
