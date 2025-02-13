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
    int64_t newBlockSize;

    // Use a switch statement for better performance
    switch (newType)
    {
        case GType::CHAR_TYPE:   newBlockSize = sizeof(char);    break;
        case GType::SHORT_TYPE:  newBlockSize = sizeof(short);   break;
        case GType::INT_TYPE:    newBlockSize = sizeof(int);     break;
        case GType::LONG_TYPE:   newBlockSize = sizeof(int64_t); break;
        case GType::FLOAT_TYPE:  newBlockSize = sizeof(float);   break;
        case GType::DOUBLE_TYPE: newBlockSize = sizeof(double);  break;
        case GType::BOOLEAN_TYPE: newBlockSize = sizeof(bool);   break;
        default:
            // Invalid type; do nothing
            return;
    }

    // Call insertObject if type is valid
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

void GList::insertObject(unsigned int index, GType::Type newType, const void* newBlock, int64_t newBlockSize)
{
    // Fix the index if it exceeds the vector size
    index = (index > items.size()) ? items.size() : index;

    // Construct the new item in place
    // if (index == items.size())
    // {
    //     // Append to the end
    //     items.emplace_back(newType, newBlock, newBlockSize);
    // }
    // else
    // {
    //     // Insert at the specified index
    items.insert(items.begin() + index, GType(newType, newBlock, newBlockSize));
    // }
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
void GList::standardize(unsigned int inputType)
{
    // 1) If there's no data, nothing to do
    if (size() == 0)
        return;

    // 2) Determine xMin, xMax
    if (inputType == 1)
    {
        // Image data: known range [0..255]
        xMin = 0.0f;
        xMax = 255.0f;
    }
    else
    {
        // Other data: find min & max via first pass
        xMin = 0.0f;
        xMax = 0.0f;

        bool firstNumericValue = true;
        for (unsigned int r = 0; r < size(); ++r)
        {
            // Use a reference to avoid copying GType
            GType& cCell = items[r];
            float cell   = 0.0f;

            // Convert to float using switch-case
            switch (cCell.getType())
            {
                case GType::STRING_TYPE:
                    // Possibly skip or handle differently
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
                    cell = static_cast<float>(cCell.getLong());
                    break;
                case GType::FLOAT_TYPE:
                    cell = cCell.getFloat();
                    break;
                case GType::DOUBLE_TYPE:
                    cell = static_cast<float>(cCell.getDouble());
                    break;
                case GType::BOOLEAN_TYPE:
                    cell = cCell.getBoolean() ? 1.0f : 0.0f;
                    break;
                default:
                    // Unknown or other types
                    break;
            }

            // Update xMin, xMax
            if (firstNumericValue)
            {
                xMin = cell;
                xMax = cell;
                firstNumericValue = false;
            }
            else
            {
                if (cell < xMin) xMin = cell;
                if (cell > xMax) xMax = cell;
            }
        }
    }

    // 3) Compute xRange
    xRange = xMax - xMin;
    if (xRange == 0.0f)
        return; // All values are the same => no transformation needed

    // 4) Second pass: normalize + shift in-place
    for (unsigned int r = 0; r < size(); ++r)
    {
        // Reference to GType in items
        GType& cCell = items[r];
        float cell   = 0.0f;

        switch (cCell.getType())
        {
            case GType::STRING_TYPE:
                // Skip or handle strings differently
                continue;
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
                cell = static_cast<float>(cCell.getLong());
                break;
            case GType::FLOAT_TYPE:
                cell = cCell.getFloat();
                break;
            case GType::DOUBLE_TYPE:
                cell = static_cast<float>(cCell.getDouble());
                break;
            case GType::BOOLEAN_TYPE:
                cell = cCell.getBoolean() ? 1.0f : 0.0f;
                break;
            default:
                continue;
        }

        // Scale from [xMin..xMax] to [0..1], then shift => [-0.5..+0.5]
        cell = ((cell - xMin) / xRange) - 0.5f;

        // Store the updated float in-place
        // (Better if you have cCell.setFloat(cell) or an inline method)
        cCell.set(GType::FLOAT_TYPE, &cell, sizeof(float));

        // No need to do items[r] = cCell; 
        // cCell is already a reference to items[r].
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
