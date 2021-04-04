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
#include "GType.h"

using namespace shmea;

GType::GType()
{
	type = NULL_TYPE;
	blockSize = 0;
	block = NULL;
}

GType::GType(const GType& g2)
{
	type = NULL_TYPE;
	blockSize = 0;
	block = NULL;
	if (g2.blockSize > 0)
		set(g2.type, g2.block, g2.blockSize);
}

GType::GType(const bool& newBlock)
{
	unsigned int newBlockSize = sizeof(bool);
	int newType = BOOLEAN_TYPE;
	block = NULL;

	set(newType, &newBlock, newBlockSize);
}

GType::GType(const char& newBlock)
{
	unsigned int newBlockSize = sizeof(char);
	int newType = CHAR_TYPE;
	block = NULL;

	set(newType, &newBlock, newBlockSize);
}

GType::GType(const short& newBlock)
{
	unsigned int newBlockSize = sizeof(short);
	int newType = SHORT_TYPE;
	block = NULL;

	set(newType, &newBlock, newBlockSize);
}

GType::GType(const int& newBlock)
{
	unsigned int newBlockSize = sizeof(int);
	int newType = INT_TYPE;
	block = NULL;

	set(newType, &newBlock, newBlockSize);
}

GType::GType(const int64_t& newBlock)
{
	unsigned int newBlockSize = sizeof(int64_t);
	int newType = LONG_TYPE;
	block = NULL;

	set(newType, &newBlock, newBlockSize);
}

GType::GType(const float& newBlock)
{
	unsigned int newBlockSize = sizeof(float);
	int newType = FLOAT_TYPE;
	block = NULL;

	set(newType, &newBlock, newBlockSize);
}

GType::GType(const double& newBlock)
{
	unsigned int newBlockSize = sizeof(double);
	int newType = DOUBLE_TYPE;
	block = NULL;

	set(newType, &newBlock, newBlockSize);
}

GType::GType(const char* newBlock)
{
	type = NULL_TYPE;
	blockSize = 0;
	block = NULL;

	// Add the object if its valid
	unsigned int newBlockSize = strlen(newBlock);
	if (newBlockSize > 0)
		set(STRING_TYPE, newBlock, newBlockSize);
}

GType::GType(const char* newBlock, unsigned int len)
{
	type = NULL_TYPE;
	blockSize = 0;
	block = NULL;

	// Add the object if its valid
	unsigned int newBlockSize = len;
	if (newBlockSize > 0)
		set(STRING_TYPE, newBlock, newBlockSize);
}

GType::GType(int newType, const void* newBlock, int64_t newBlockSize)
{
	type = NULL_TYPE;
	blockSize = 0;
	block = NULL;

	// Add the object if its valid
	if (newBlockSize > 0)
		set(newType, newBlock, newBlockSize);
}

GType::~GType()
{
	clean();
}

int GType::getType() const
{
	return type;
}

const char* GType::c_str() const
{
	if ((!block) || (size() == 0))
		return NULL;

	return block;
}

char GType::getChar() const
{
	if ((!block) || (size() == 0))
		return 0;

	if (getType() == GType::SHORT_TYPE)
	{
		char value = getShort();
		return value;
	}
	else if (getType() == GType::INT_TYPE)
	{
		char value = getInt();
		return value;
	}
	else if (getType() == GType::LONG_TYPE)
	{
		char value = getLong();
		return value;
	}
	else if (getType() == GType::FLOAT_TYPE)
	{
		char value = getFloat();
		return value;
	}
	else if (getType() == GType::DOUBLE_TYPE)
	{
		char value = getDouble();
		return value;
	}
	else if (getType() == GType::BOOLEAN_TYPE)
	{
		return (char)getBoolean();
	}
	else if (getType() == GType::STRING_TYPE)
	{
		char value = atoi(c_str());
		return value;
	}

	// Char Type (match)
	if (size() != sizeof(char))
		return 0;

	return *((char*)block);
}

short GType::getShort() const
{
	if ((!block) || (size() == 0))
		return 0;

	if (getType() == GType::CHAR_TYPE)
	{
		short value = getChar();
		return value;
	}
	else if (getType() == GType::INT_TYPE)
	{
		short value = getInt();
		return value;
	}
	else if (getType() == GType::LONG_TYPE)
	{
		short value = getLong();
		return value;
	}
	else if (getType() == GType::FLOAT_TYPE)
	{
		short value = getFloat();
		return value;
	}
	else if (getType() == GType::DOUBLE_TYPE)
	{
		short value = getDouble();
		return value;
	}
	else if (getType() == GType::BOOLEAN_TYPE)
	{
		return (short)getBoolean();
	}
	else if (getType() == GType::STRING_TYPE)
	{
		short value = atoi(c_str());
		return value;
	}

	// Short Type (match)
	if (size() != sizeof(short))
		return 0;

	return *((short*)block);
}

int GType::getInt() const
{
	if ((!block) || (size() == 0))
		return 0;

	if (getType() == GType::CHAR_TYPE)
	{
		int value = getChar();
		return value;
	}
	else if (getType() == GType::SHORT_TYPE)
	{
		int value = getShort();
		return value;
	}
	else if (getType() == GType::LONG_TYPE)
	{
		int value = getLong();
		return value;
	}
	else if (getType() == GType::FLOAT_TYPE)
	{
		int value = getFloat();
		return value;
	}
	else if (getType() == GType::DOUBLE_TYPE)
	{
		int value = getDouble();
		return value;
	}
	else if (getType() == GType::BOOLEAN_TYPE)
	{
		return (int)getBoolean();
	}
	else if (getType() == GType::STRING_TYPE)
	{
		int value = atoi(c_str());
		return value;
	}

	// int Type (match)
	if (size() != sizeof(int))
		return 0;

	return *((int*)block);
}

int64_t GType::getLong() const
{
	if ((!block) || (size() == 0))
		return 0;

	if (getType() == GType::CHAR_TYPE)
	{
		int64_t value = getChar();
		return value;
	}
	else if (getType() == GType::SHORT_TYPE)
	{
		int64_t value = getShort();
		return value;
	}
	else if (getType() == GType::INT_TYPE)
	{
		int64_t value = getInt();
		return value;
	}
	else if (getType() == GType::FLOAT_TYPE)
	{
		int64_t value = getFloat();
		return value;
	}
	else if (getType() == GType::DOUBLE_TYPE)
	{
		int64_t value = getDouble();
		return value;
	}
	else if (getType() == GType::BOOLEAN_TYPE)
	{
		return (int64_t)getBoolean();
	}
	else if (getType() == GType::STRING_TYPE)
	{
		int64_t value = atoll(c_str());
		return value;
	}

	// Long Type (match)
	if (size() != sizeof(int64_t))
		return 0;

	return *((int64_t*)block);
}

float GType::getFloat() const
{
	if ((!block) || (size() == 0))
		return 0;

	if (getType() == GType::CHAR_TYPE)
	{
		float value = getChar();
		return value;
	}
	else if (getType() == GType::SHORT_TYPE)
	{
		float value = getShort();
		return value;
	}
	else if (getType() == GType::INT_TYPE)
	{
		float value = getInt();
		return value;
	}
	else if (getType() == GType::LONG_TYPE)
	{
		float value = getLong();
		return value;
	}
	else if (getType() == GType::DOUBLE_TYPE)
	{
		float value = getDouble();
		return value;
	}
	else if (getType() == GType::BOOLEAN_TYPE)
	{
		return (float)getBoolean();
	}
	else if (getType() == GType::STRING_TYPE)
	{
		float value = atof(c_str());
		return value;
	}

	// Float Type (match)
	if (size() != sizeof(float))
		return 0.0f;

	return *((float*)block);
}

double GType::getDouble() const
{
	if ((!block) || (size() == 0))
		return 0;

	if (getType() == GType::CHAR_TYPE)
	{
		double value = getChar();
		return value;
	}
	else if (getType() == GType::SHORT_TYPE)
	{
		double value = getShort();
		return value;
	}
	else if (getType() == GType::INT_TYPE)
	{
		double value = getInt();
		return value;
	}
	else if (getType() == GType::LONG_TYPE)
	{
		double value = getLong();
		return value;
	}
	else if (getType() == GType::FLOAT_TYPE)
	{
		double value = getFloat();
		return value;
	}
	else if (getType() == GType::BOOLEAN_TYPE)
	{
		return (double)getBoolean();
	}
	else if (getType() == GType::STRING_TYPE)
	{
		double value = atof(c_str());
		return value;
	}

	// Double Type (match)
	if (size() != sizeof(double))
		return 0.0f;

	return *((double*)block);
}

bool GType::getBoolean() const
{
	if ((!block) || (size() == 0))
		return 0;

	if (getType() == GType::CHAR_TYPE)
	{
		bool value = getChar();
		return value;
	}
	else if (getType() == GType::SHORT_TYPE)
	{
		bool value = getShort();
		return value;
	}
	else if (getType() == GType::INT_TYPE)
	{
		bool value = getInt();
		return value;
	}
	else if (getType() == GType::LONG_TYPE)
	{
		bool value = getLong();
		return value;
	}
	else if (getType() == GType::FLOAT_TYPE)
	{
		bool value = getFloat();
		return value;
	}
	else if (getType() == GType::DOUBLE_TYPE)
	{
		bool value = getDouble();
		return value;
	}
	else if (getType() == GType::STRING_TYPE)
	{
		bool value = atoll(c_str());
		return value;
	}

	// Boolean Type (match)
	if (size() != sizeof(bool))
		return false;

	return *((bool*)block);
}

unsigned int GType::size() const
{
	return blockSize;
}

void GType::set(int newType, const void* newBlock, int64_t newBlockSize)
{
	clean();
	type = newType;
	blockSize = newBlockSize;
	block = (char*)malloc(blockSize + 1); // plus one to escape the string, we ignore this character everywhere else
	memcpy(block, newBlock, blockSize);
	block[blockSize] = '\0';
}

void GType::clean()
{
	// Spam bad request and this crashes WHY: TODO CHECK THIS
	if (block)
		free(block);
	block = NULL;

	blockSize = 0;
	type = NULL_TYPE;
}
