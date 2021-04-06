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
#include "GString.h"

using namespace shmea;

GString::GString()
{
	blockSize = 0;
	block = NULL;

	initEmpty();
}

GString::GString(const GString& g2) : GType(g2)
{
	// Calling parent constructor
}

GString::GString(const GType& g2) : GType(g2)
{
	// Calling parent constructor
}


void GString::initEmpty()
{
	clean();
	type = STRING_TYPE;
	block = (char*)malloc(1); // only the escape for the string
	block[blockSize] = '\0';
}

GString::GString(const char* newBlock)
{
	type = NULL_TYPE;
	blockSize = 0;
	block = NULL;

	// Add the object if its valid
	unsigned int newBlockSize = strlen(newBlock);
	if (newBlockSize > 0)
		set(STRING_TYPE, newBlock, newBlockSize);
	else
		initEmpty();
}

GString::GString(const char* newBlock, unsigned int len)
{
	type = NULL_TYPE;
	blockSize = 0;
	block = NULL;

	// Add the object if its valid
	unsigned int newBlockSize = len;
	if (newBlockSize > 0)
		set(STRING_TYPE, newBlock, newBlockSize);
	else
		initEmpty();
}

GString::GString(const bool& cBool) : GType(cBool)
{
	type = STRING_TYPE;
}

GString::GString(const char& cChar) : GType(cChar)
{
	type = STRING_TYPE;
}

GString::GString(const short& cShort) : GType(cShort)
{
	type = STRING_TYPE;
}

GString::GString(const int& cInt) : GType(cInt)
{
	type = STRING_TYPE;
}

GString::GString(const int64_t& cLong) : GType(cLong)
{
	type = STRING_TYPE;
}

GString::GString(const float& cFloat) : GType(cFloat)
{
	type = STRING_TYPE;
}

GString::GString(const double& cDouble) : GType(cDouble)
{
	type = STRING_TYPE;
}

GString::~GString()
{
	//
}

unsigned int GString::length() const
{
	return size();
}

const char& GString::operator[](const unsigned int& index) const
{
	if(index >= length())
	{
		char buffer[256];
		sprintf(buffer, "GStr[%d] out of range", index);
		throw std::out_of_range(buffer);
	}

	return block[index];
}

char& GString::operator[](const unsigned int& index)
{
	if(index >= length())
	{
		char buffer[256];
		sprintf(buffer, "GStr[%d] out of range", index);
		throw std::out_of_range(buffer);
	}

	return block[index];
}

bool GString::operator==(const GType& cCell2) const
{
	return GType::operator==(cCell2);
}

bool GString::operator!=(const GType& cCell2) const
{
	return !((*this) == cCell2);
}

bool GString::operator==(const char* cCell2) const
{
	return (*this == GString(cCell2));
}

bool GString::operator!=(const char* cCell2) const
{
	return (*this != GString(cCell2));
}

// NOT APART OF THE CLASS
GString operator+ (const GString& lhs, const GString& rhs)
{
	unsigned int newBlockSize = lhs.length() + rhs.size();
	char* newBlock = (char*)malloc(newBlockSize);
	memcpy(newBlock, lhs.c_str(), lhs.length());
	memcpy(&newBlock[lhs.length()], rhs.c_str(), rhs.size());

	GString retStr(newBlock, newBlockSize);
	free(newBlock);

	return retStr;
}

// NOT APART OF THE CLASS
GString operator+ (const GString& lhs, const char* rhs)
{
	unsigned int newBlockSize = lhs.length() + strlen(rhs);
	char* newBlock = (char*)malloc(newBlockSize);
	memcpy(newBlock, lhs.c_str(), lhs.length());
	memcpy(&newBlock[lhs.length()], rhs, strlen(rhs));

	GString retStr(newBlock, newBlockSize);
	free(newBlock);

	return retStr;
}

// NOT APART OF THE CLASS
GString operator+ (const GString& lhs, char rhs)
{
	unsigned int newBlockSize = lhs.length() + 1;
	char* newBlock = (char*)malloc(newBlockSize);
	memcpy(newBlock, lhs.c_str(), lhs.length());
	newBlock[newBlockSize-1] = rhs;

	GString retStr(newBlock, newBlockSize);
	free(newBlock);

	return retStr;
}

// NOT APART OF THE CLASS
GString operator+ (const char* lhs, const GString& rhs)
{
	unsigned int newBlockSize = strlen(lhs) + rhs.size();
	char* newBlock = (char*)malloc(newBlockSize);
	memcpy(newBlock, lhs, strlen(lhs));
	memcpy(&newBlock[strlen(lhs)], rhs.c_str(), rhs.size());

	GString retStr(newBlock, newBlockSize);
	free(newBlock);

	return retStr;
}

// NOT APART OF THE CLASS
GString operator+ (char lhs, const GString& rhs)
{
	unsigned int newBlockSize = rhs.length() + 1;
	char* newBlock = (char*)malloc(newBlockSize);
	newBlock[0] = lhs;
	memcpy(&newBlock[1], rhs.c_str(), rhs.length());

	GString retStr(newBlock, newBlockSize);
	free(newBlock);

	return retStr;
}

GString GString::operator+=(const char& cChar)
{
	unsigned int newBlockSize = length() + 1;
	char* newBlock = (char*)malloc(newBlockSize);
	memcpy(newBlock, block, length());
	newBlock[newBlockSize-1] = cChar;

	set(getType(), newBlock, newBlockSize);
	free(newBlock);

	return *this;
}

GString GString::operator+=(const GType& str2)
{
	unsigned int newBlockSize = length() + str2.size();
	char* newBlock = (char*)malloc(newBlockSize);
	memcpy(newBlock, block, length());
	memcpy(&newBlock[length()], str2.c_str(), str2.size());

	set(getType(), newBlock, newBlockSize);
	free(newBlock);

	return *this;
}

GString GString::operator+=(const GString& str2)
{
	unsigned int newBlockSize = length() + str2.size();
	char* newBlock = (char*)malloc(newBlockSize);
	memcpy(newBlock, block, length());
	memcpy(&newBlock[length()], str2.c_str(), str2.size());

	set(getType(), newBlock, newBlockSize);
	free(newBlock);

	return *this;
}

GString GString::operator+=(const char* str2)
{
	unsigned int newBlockSize = length() + strlen(str2);
	char* newBlock = (char*)malloc(newBlockSize);
	memcpy(newBlock, block, length());
	memcpy(&newBlock[length()], str2, strlen(str2));

	set(getType(), newBlock, newBlockSize);
	free(newBlock);

	return *this;
}

GString GString::substr(unsigned int start, unsigned int len) const
{
	if(blockSize == 0)
		return "";

	if(start >= blockSize)
		return "";

	if((len == 0) || (len > start + blockSize))
		len = blockSize - start;

	//Empty source
	if(len == 0)
		return "";

	return GString(&block[start], len);
}
