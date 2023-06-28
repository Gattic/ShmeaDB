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
#include "GDeleter.h"

using namespace shmea;

GType::GType()
{
	type = NULL_TYPE;
	blockSize = 0;
}

GType::GType(const GType& g2)
{
	type = NULL_TYPE;
	blockSize = 0;
	if (g2.blockSize > 0)
		set(g2.type, g2.block.get(), g2.blockSize);
}

GType::GType(const bool& newBlock)
{
	unsigned int newBlockSize = sizeof(bool);
	blockSize = 0;

	set(BOOLEAN_TYPE, &newBlock, newBlockSize);
}

GType::GType(const char& newBlock)
{
	unsigned int newBlockSize = sizeof(char);
	blockSize = 0;

	set(CHAR_TYPE, &newBlock, newBlockSize);
}

GType::GType(const short& newBlock)
{
	unsigned int newBlockSize = sizeof(short);
	blockSize = 0;

	set(SHORT_TYPE, &newBlock, newBlockSize);
}

GType::GType(const int& newBlock)
{
	unsigned int newBlockSize = sizeof(int);
	blockSize = 0;

	set(INT_TYPE, &newBlock, newBlockSize);
}

GType::GType(const int64_t& newBlock)
{
	unsigned int newBlockSize = sizeof(int64_t);
	blockSize = 0;

	set(LONG_TYPE, &newBlock, newBlockSize);
}

GType::GType(const float& newBlock)
{
	unsigned int newBlockSize = sizeof(float);
	blockSize = 0;

	set(FLOAT_TYPE, &newBlock, newBlockSize);
}

GType::GType(const double& newBlock)
{
	unsigned int newBlockSize = sizeof(double);
	blockSize = 0;

	set(DOUBLE_TYPE, &newBlock, newBlockSize);
}

GType::GType(const char* newBlock)
{
	type = NULL_TYPE;
	blockSize = 0;

	// Add the object if its valid
	unsigned int newBlockSize = strlen(newBlock);
	if (newBlockSize > 0)
		set(STRING_TYPE, newBlock, newBlockSize);
}

GType::GType(const char* newBlock, unsigned int len)
{
	type = NULL_TYPE;
	blockSize = 0;

	// Add the object if its valid
	unsigned int newBlockSize = len;
	if (newBlockSize > 0)
		set(STRING_TYPE, newBlock, newBlockSize);
}

GType::GType(Type newType, const void* newBlock, int64_t newBlockSize)
{
	type = NULL_TYPE;
	blockSize = 0;

	// Add the object if its valid
	if (newBlockSize > 0)
		set(newType, newBlock, newBlockSize);
}

GType::~GType()
{
	blockSize = 0;
	type = NULL_TYPE;
}

GType::Type GType::getType() const
{
	return type;
}

const char* GType::c_str() const
{
	if ((!block.get()) || (size() == 0))
		return NULL;

	return block.get();
}

char GType::getChar() const
{
	if ((!block.get()) || (size() == 0))
		return 0;

	switch (this->getType())
	{
		case SHORT_TYPE:
			return this->getShort();
		case INT_TYPE:
			return this->getInt();
		case LONG_TYPE:
			return this->getLong();
		case FLOAT_TYPE:
			return this->getFloat();
		case DOUBLE_TYPE:
			return this->getDouble();
		case BOOLEAN_TYPE:
			return this->getBoolean();
		case STRING_TYPE:
			return *this->block;
	}

	// Char Type (match)
	if (size() != sizeof(char))
		return 0;

	return *((char*)block.get());
}

short GType::getShort() const
{
	if ((!block.get()) || (size() == 0))
		return 0;

	switch (this->getType())
	{
		case CHAR_TYPE:
			return this->getChar();
		case INT_TYPE:
			return this->getInt();
		case LONG_TYPE:
			return this->getLong();
		case FLOAT_TYPE:
			return this->getFloat();
		case DOUBLE_TYPE:
			return this->getDouble();
		case BOOLEAN_TYPE:
			return this->getBoolean();
		case STRING_TYPE:
			return *this->block;
	}

	// Short Type (match)
	if (size() != sizeof(short))
		return 0;

	return *((short*)block.get());
}

int GType::getInt() const
{
	if ((!block.get()) || (size() == 0))
		return 0;

	switch (this->getType())
	{
		case CHAR_TYPE:
			return this->getChar();
		case SHORT_TYPE:
			return this->getShort();
		case LONG_TYPE:
			return this->getLong();
		case FLOAT_TYPE:
			return this->getFloat();
		case DOUBLE_TYPE:
			return this->getDouble();
		case BOOLEAN_TYPE:
			return this->getBoolean();
		case STRING_TYPE: {
			//int value = *block;
			int value = 0;//TODO: PUT THIS IN LONG, AND SHORT; ADD TEST CASES
			for(unsigned int i = 0; i < size(); ++i)
			{
				value<<=8;
				value+=block[size()-i-1];
			}
			return value;
		}
	}

	// int Type (match)
	if (size() != sizeof(int))
		return 0;

	return *((int*)block.get());
}

int64_t GType::getLong() const
{
	if ((!block.get()) || (size() == 0))
		return 0;

	switch (this->getType())
	{
		case CHAR_TYPE:
			return this->getChar();
		case SHORT_TYPE:
			return this->getShort();
		case INT_TYPE:
			return this->getInt();
		case FLOAT_TYPE:
			return this->getFloat();
		case DOUBLE_TYPE:
			return this->getDouble();
		case BOOLEAN_TYPE:
			return this->getBoolean();
		case STRING_TYPE:
			return *this->block;
	}

	// Long Type (match)
	if (size() != sizeof(int64_t))
		return 0;

	return *((int64_t*)block.get());
}

float GType::getFloat() const
{
	if ((!block.get()) || (size() == 0))
		return 0;

	switch (this->getType())
	{
		case CHAR_TYPE:
			return this->getChar();
		case SHORT_TYPE:
			return this->getShort();
		case INT_TYPE:
			return this->getInt();
		case LONG_TYPE:
			return this->getLong();
		case DOUBLE_TYPE:
			return this->getDouble();
		case BOOLEAN_TYPE:
			return this->getBoolean();
		case STRING_TYPE:
			return *this->block;
	}

	// Float Type (match)
	if (size() != sizeof(float))
		return 0.0f;

	return *((float*)block.get());
}

double GType::getDouble() const
{
	if ((!block.get()) || (size() == 0))
		return 0;

	switch (this->getType())
	{
		case CHAR_TYPE:
			return this->getChar();
		case SHORT_TYPE:
			return this->getShort();
		case INT_TYPE:
			return this->getInt();
		case LONG_TYPE:
			return this->getLong();
		case FLOAT_TYPE:
			return this->getFloat();
		case BOOLEAN_TYPE:
			return this->getBoolean();
		case STRING_TYPE:
			return *this->block;
	}

	// Double Type (match)
	if (size() != sizeof(double))
		return 0.0f;

	return *((double*)block.get());
}

bool GType::getBoolean() const
{
	if ((!block.get()) || (size() == 0))
		return 0;

	switch (this->getType())
	{
		case CHAR_TYPE:
			return this->getChar();
		case SHORT_TYPE:
			return this->getShort();
		case INT_TYPE:
			return this->getInt();
		case LONG_TYPE:
			return this->getLong();
		case FLOAT_TYPE:
			return this->getFloat();
		case DOUBLE_TYPE:
			return this->getDouble();
		case STRING_TYPE:
			return *this->block;
	}

	// Boolean Type (match)
	if (size() != sizeof(bool))
		return false;

	return *((bool*)block.get());
}

unsigned int GType::size() const
{
	return blockSize;
}

void GType::set(Type newType, const void* newBlock, int64_t newBlockSize)
{
	if(blockSize == newBlockSize)
	{
		type = newType;
		memcpy(block.get(), newBlock, blockSize); // this is a copy so safe to assume it has the \0 from the block else below
	}
	else
	{
		type = newType;
		blockSize = newBlockSize;
		char* newMem = new char[blockSize + 1];
		block.copy(GPointer<char, array_deleter<char> >(newMem)); // plus one to escape the string, we ignore this character everywhere else
		memcpy(block.get(), newBlock, blockSize);
		block[blockSize] = '\0';
	}
}
