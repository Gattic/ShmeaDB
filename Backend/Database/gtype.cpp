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
#include "gtype.h"

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
	blockSize = sizeof(bool);
	type = BOOLEAN_TYPE;
	block = (char*)malloc(blockSize);
	memcpy(block, &newBlock, blockSize);
}

GType::GType(const char& newBlock)
{
	blockSize = sizeof(char);
	type = CHAR_TYPE;
	block = (char*)malloc(blockSize);
	memcpy(block, &newBlock, blockSize);
}

GType::GType(const short& newBlock)
{
	blockSize = sizeof(short);
	type = SHORT_TYPE;
	block = (char*)malloc(blockSize);
	memcpy(block, &newBlock, blockSize);
}

GType::GType(const int& newBlock)
{
	blockSize = sizeof(int);
	type = INT_TYPE;
	block = (char*)malloc(blockSize);
	memcpy(block, &newBlock, blockSize);
}

GType::GType(const int64_t& newBlock)
{
	blockSize = sizeof(int64_t);
	type = LONG_TYPE;
	block = (char*)malloc(blockSize);
	memcpy(block, &newBlock, blockSize);
}

GType::GType(const float& newBlock)
{
	blockSize = sizeof(float);
	type = FLOAT_TYPE;
	block = (char*)malloc(blockSize);
	memcpy(block, &newBlock, blockSize);
}

GType::GType(const double& newBlock)
{
	blockSize = sizeof(double);
	type = DOUBLE_TYPE;
	block = (char*)malloc(blockSize);
	memcpy(block, &newBlock, blockSize);
}

GType::GType(const std::string& word)
{
	block = NULL;
	blockSize = 0;

	// deduce the type flag
	type = LONG_TYPE;
	if (!isInteger(word))
	{
		type = FLOAT_TYPE;
		if (!isFloat(word))
			type = STRING_TYPE;
	}

	if (type == FLOAT_TYPE)
	{
		float newBlock = atof(word.c_str());
		set(type, &newBlock, sizeof(float));
	}
	else if (type == LONG_TYPE)
	{
		// int
		/*int64_t newBlock=atol(word.c_str());
		void* pVal=NULL;
		if(newBlock < MAX_INT)
		{
			type=INT_TYPE;
			if(newBlock < MAX_SHORT)
			{
				type=SHORT_TYPE;
				if(newBlock < MAX_CHAR)
				{
					type=CHAR_TYPE;

					//malloc for the char
					pVal=(void*)malloc(sizeof(char));
					*((char*)pVal)=(char)newBlock;
				}

				pVal=(void*)malloc(sizeof(short));
				*((short*)pVal)=(short)newBlock;
			}

			pVal=(void*)malloc(sizeof(int));
			*((int*)pVal)=(int)newBlock;
		}*/

		int64_t newBlock = atoll(word.c_str());
		set(type, &newBlock, sizeof(int64_t));
	}
	else if (type == STRING_TYPE)
	{
		set(type, word.c_str(), word.length());
	}
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

char* GType::getBlockCopy() const
{
	if (!block)
		return NULL;

	char* retVal = (char*)malloc(sizeof(char) * blockSize);
	memcpy(retVal, block, blockSize);
	return retVal;
}

int GType::getType() const
{
	return type;
}

std::string GType::getString() const
{
	if (size() == 0)
		return "";

	if (getType() == GType::CHAR_TYPE)
	{
		std::string value = "";
		value += getChar();
		return value;
	}
	else if (getType() == GType::SHORT_TYPE)
	{
		std::string value = "";
		value += shortTOstring(getShort());
		return value;
	}
	else if (getType() == GType::INT_TYPE)
	{
		std::string value(intTOstring(getInt()));
		return value;
	}
	else if (getType() == GType::LONG_TYPE)
	{
		return longTOstring(getLong());
	}
	else if (getType() == GType::FLOAT_TYPE)
	{
		return floatTOstring(getFloat());
	}
	else if (getType() == GType::DOUBLE_TYPE)
	{
		return doubleTOstring(getDouble());
	}
	else if (getType() == GType::BOOLEAN_TYPE)
	{
		std::string value = "";
		if (getBoolean())
			value = "True";
		else
			value = "False";
		return value;
	}

	// String Type (match)
	int newBlockSize = size();
	char* newBlock = (char*)malloc(sizeof(char) * newBlockSize + 1);
	bzero(newBlock, sizeof(char) * newBlockSize + 1);
	memcpy(newBlock, getBlockCopy(), newBlockSize);
	newBlock[newBlockSize] = '\0';
	std::string newStr(newBlock);

	return newStr;
}

const char* GType::getCString() const
{
	if (size() <= 0)
		return "";

	if (getType() == GType::CHAR_TYPE)
	{
		std::string value = "";
		value += getChar();
		return value.c_str();
	}
	else if (getType() == GType::SHORT_TYPE)
	{
		std::string value = "";
		value += shortTOstring(getShort());
		return value.c_str();
	}
	else if (getType() == GType::INT_TYPE)
	{
		std::string value(intTOstring(getInt()));
		return value.c_str();
	}
	else if (getType() == GType::LONG_TYPE)
	{
		std::string value(longTOstring(getLong()));
		return value.c_str();
	}
	else if (getType() == GType::FLOAT_TYPE)
	{
		std::string value(floatTOstring(getFloat()));
		return value.c_str();
	}
	else if (getType() == GType::DOUBLE_TYPE)
	{
		std::string value(doubleTOstring(getDouble()));
		return value.c_str();
	}
	else if (getType() == GType::BOOLEAN_TYPE)
	{
		std::string value = getBoolean() ? "True" : "False";
		return value.c_str();
	}

	// String Type (match)
	int newBlockSize = size();
	char* newBlock = (char*)malloc(sizeof(char) * newBlockSize + 1);
	memcpy(newBlock, getBlockCopy(), newBlockSize);
	newBlock[newBlockSize] = '\0';
	std::string newStr(newBlock);
	return newStr.c_str();
}

char GType::getChar() const
{
	if (size() <= 0)
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
		char value = atoi(getCString());
		return value;
	}

	// Char Type (match)
	if (size() != sizeof(char))
		return 0;

	return *((char*)getBlockCopy());
}

short GType::getShort() const
{
	if (size() <= 0)
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
		short value = atoi(getCString());
		return value;
	}

	// Short Type (match)
	if (size() != sizeof(short))
		return 0;

	return *((short*)getBlockCopy());
}

int GType::getInt() const
{
	if (size() <= 0)
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
		int value = atoi(getCString());
		return value;
	}

	// int Type (match)
	if (size() != sizeof(int))
		return 0;

	return *((int*)getBlockCopy());
}

int64_t GType::getLong() const
{
	if (size() <= 0)
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
		int64_t value = atoll(getCString());
		return value;
	}

	// Long Type (match)
	if (size() != sizeof(int64_t))
		return 0;

	return *((int64_t*)getBlockCopy());
}

float GType::getFloat() const
{
	if (size() <= 0)
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
		float value = atof(getCString());
		return value;
	}

	// Float Type (match)
	if (size() != sizeof(float))
		return 0.0f;

	return *((float*)getBlockCopy());
}

double GType::getDouble() const
{
	if (size() <= 0)
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
		double value = atof(getCString());
		return value;
	}

	// Double Type (match)
	if (size() != sizeof(double))
		return 0.0f;

	return *((double*)getBlockCopy());
}

bool GType::getBoolean() const
{
	if (size() <= 0)
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
		bool value = atoll(getCString());
		return value;
	}

	// Boolean Type (match)
	if (size() != sizeof(bool))
		return false;

	return *((bool*)getBlockCopy());
}

GType::operator char() const
{
	return getChar();
}

GType::operator short() const
{
	return getShort();
}

GType::operator int() const
{
	return getInt();
}

GType::operator int64_t() const
{
	return getLong();
}

GType::operator float() const
{
	return getFloat();
}

GType::operator double() const
{
	return getDouble();
}

GType::operator const char*() const
{
	return getCString();
}

GType::operator std::string() const
{
	return getString();
}

GType::operator bool() const
{
	return getBoolean();
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
	block = (char*)malloc(blockSize * sizeof(char));
	memcpy(block, newBlock, blockSize * sizeof(char));
}

void GType::clean()
{
	if (block)
		free(block);
	block = NULL;

	blockSize = 0;
	type = NULL_TYPE;
}

void GType::operator=(const GType& compValue)
{
	set(compValue.getType(), compValue.block, compValue.size());
}

void GType::operator=(const char& compValue)
{
	set(CHAR_TYPE, &compValue, sizeof(char));
}

void GType::operator=(const short& compValue)
{
	set(SHORT_TYPE, &compValue, sizeof(short));
}

void GType::operator=(const int& compValue)
{
	set(INT_TYPE, &compValue, sizeof(int));
}

void GType::operator=(const int64_t& compValue)
{
	set(LONG_TYPE, &compValue, sizeof(int));
}

void GType::operator=(const float& compValue)
{
	set(FLOAT_TYPE, &compValue, sizeof(float));
}

void GType::operator=(const double& compValue)
{
	set(DOUBLE_TYPE, &compValue, sizeof(double));
}

void GType::operator=(const char* compValue)
{
	std::string newCompValue(compValue);
	set(STRING_TYPE, newCompValue.c_str(), newCompValue.length());
}

void GType::operator=(const std::string& compValue)
{
	set(STRING_TYPE, compValue.c_str(), compValue.length());
}

void GType::operator=(const bool& compValue)
{
	set(BOOLEAN_TYPE, &compValue, sizeof(bool));
}

bool GType::operator==(const GType& cCell2)
{
	// compare the known types
	bool intFlag1 = false;
	bool intFlag2 = false;
	bool floatFlag1 = false;
	bool floatFlag2 = false;
	bool stringFlag1 = false;
	bool stringFlag2 = false;
	bool boolFlag1 = false;
	bool boolFlag2 = false;

	// values
	int64_t intValue1 = 0;
	double floatValue1 = 0.0f;
	std::string stringValue1 = "";
	bool boolValue1 = false;
	int64_t intValue2 = 0;
	double floatValue2 = 0.0f;
	std::string stringValue2 = "";
	bool boolValue2 = false;

	// get the first value
	if (getType() == GType::CHAR_TYPE)
	{
		intFlag1 = true;
		intValue1 = getChar();
	}
	else if (getType() == GType::SHORT_TYPE)
	{
		intFlag1 = true;
		intValue1 = getShort();
	}
	else if (getType() == GType::INT_TYPE)
	{
		intFlag1 = true;
		intValue1 = getInt();
	}
	else if (getType() == GType::LONG_TYPE)
	{
		intFlag1 = true;
		intValue1 = getLong();
	}
	else if (getType() == GType::FLOAT_TYPE)
	{
		floatFlag1 = true;
		floatValue1 = getFloat();
	}
	else if (getType() == GType::DOUBLE_TYPE)
	{
		floatFlag1 = true;
		floatValue1 = getDouble();
	}
	else if (getType() == GType::BOOLEAN_TYPE)
	{
		boolFlag1 = true;
		boolValue1 = getBoolean();
	}
	else if (getType() == GType::STRING_TYPE)
	{
		stringFlag1 = true;
		stringValue1 = getString();
	}

	// get the second value
	if (cCell2.getType() == GType::CHAR_TYPE)
	{
		intFlag2 = true;
		intValue2 = cCell2.getChar();
	}
	else if (cCell2.getType() == GType::SHORT_TYPE)
	{
		intFlag2 = true;
		intValue2 = cCell2.getShort();
	}
	else if (cCell2.getType() == GType::INT_TYPE)
	{
		intFlag2 = true;
		intValue2 = cCell2.getInt();
	}
	else if (cCell2.getType() == GType::LONG_TYPE)
	{
		intFlag2 = true;
		intValue2 = cCell2.getLong();
	}
	else if (cCell2.getType() == GType::FLOAT_TYPE)
	{
		floatFlag2 = true;
		floatValue2 = cCell2.getFloat();
	}
	else if (cCell2.getType() == GType::DOUBLE_TYPE)
	{
		floatFlag2 = true;
		floatValue2 = cCell2.getDouble();
	}
	else if (cCell2.getType() == GType::BOOLEAN_TYPE)
	{
		boolFlag2 = true;
		boolValue2 = cCell2.getBoolean();
	}
	else if (cCell2.getType() == GType::STRING_TYPE)
	{
		stringFlag2 = true;
		stringValue2 = cCell2.getString();
	}

	// compare the gtypes
	if ((intFlag1) && (intFlag2))
		return (intValue1 == intValue2);
	else if ((floatFlag1) && (floatFlag2))
		return (floatValue1 == floatValue2);
	else if ((stringFlag1) && (stringFlag2))
		return (stringValue1 == stringValue2);
	else if ((boolFlag1) && (boolFlag2))
		return (boolValue1 == boolValue2);
	// cross ints and floats
	else if ((intFlag1) && (floatFlag2))
		return (intValue1 == floatValue2);
	else if ((floatFlag1) && (intFlag2))
		return (floatValue1 == intValue2);

	return false;
}

bool GType::operator==(const char& compValue)
{
	GType cCell(compValue);
	return !((*this) == compValue);
}

bool GType::operator==(const short& compValue)
{
	GType cCell(compValue);
	return !((*this) == compValue);
}

bool GType::operator==(const int& compValue)
{
	GType cCell(compValue);
	return !((*this) == compValue);
}

bool GType::operator==(const int64_t& compValue)
{
	GType cCell(compValue);
	return !((*this) == compValue);
}

bool GType::operator==(const float& compValue)
{
	GType cCell(compValue);
	return !((*this) == compValue);
}

bool GType::operator==(const double& compValue)
{
	GType cCell(compValue);
	return !((*this) == compValue);
}

bool GType::operator==(const char* compValue)
{
	std::string newCompValue(compValue);
	return !((*this) == newCompValue);
}

bool GType::operator==(const std::string& compValue)
{
	return !((*this) == compValue);
}

bool GType::operator==(const bool& compValue)
{
	GType cCell(compValue);
	return !((*this) == compValue);
}

bool GType::operator!=(const GType& cCell2)
{
	return !((*this) == cCell2);
}

bool GType::operator!=(const char& compValue)
{
	return !((*this) == compValue);
}

bool GType::operator!=(const short& compValue)
{
	return !((*this) == compValue);
}

bool GType::operator!=(const int& compValue)
{
	return !((*this) == compValue);
}

bool GType::operator!=(const int64_t& compValue)
{
	return !((*this) == compValue);
}

bool GType::operator!=(const float& compValue)
{
	return !((*this) == compValue);
}

bool GType::operator!=(const double& compValue)
{
	return !((*this) == compValue);
}

bool GType::operator!=(const char* compValue)
{
	std::string newCompValue(compValue);
	return !((*this) == compValue);
}

bool GType::operator!=(const std::string& compValue)
{
	return !((*this) == compValue);
}

bool GType::operator!=(const bool& compValue)
{
	return !((*this) == compValue);
}

bool GType::isWhitespace(const char tempNumber)
{
	const std::string options = " \t\r\n";

	int breakPoint = options.find(tempNumber);
	return (breakPoint >= 0);
}

bool GType::isWhitespace(const char* tempNumber)
{
	std::string newStr(tempNumber);
	return isWhitespace(newStr);
}

bool GType::isWhitespace(const std::string tempNumber)
{
	const std::string options = " \t\r\n";

	for (unsigned int i = 0; i < tempNumber.size(); ++i)
	{
		int breakPoint = options.find(tempNumber[i]);
		if (breakPoint == -1)
			return false;
	}

	return true;
}

bool GType::isInteger(const char* tempNumber)
{
	std::string newStr(tempNumber);
	return isInteger(newStr);
}

bool GType::isInteger(const std::string tempNumber)
{
	const std::string options = "0123456789";

	unsigned int negNumber = tempNumber.find('-'), i = 0;
	if ((negNumber != (unsigned int)std::string::npos) && (negNumber > 0))
		return false;
	else if (negNumber == 0)
		i = 1;

	for (; i < tempNumber.length(); ++i)
	{
		int breakPoint = options.find(tempNumber[i]);
		if (breakPoint == -1)
			return false;
	}

	return true;
}

bool GType::isFloat(const char* tempNumber)
{
	std::string newStr(tempNumber);
	return isFloat(newStr);
}

bool GType::isFloat(const std::string tempNumber)
{
	const std::string options = "0123456789.f";

	unsigned int negNumber = tempNumber.find('-'), i = 0;
	if ((negNumber != (unsigned int)std::string::npos) && (negNumber > 0))
		return false;
	else if (negNumber == 0)
		i = 1;

	for (; i < tempNumber.size(); ++i)
	{
		int breakPoint = options.find(tempNumber[i]);
		if (breakPoint == -1)
			return false;
	}

	return true;
}

bool GType::isUpper(const char x)
{
	return ((x >= 0x41) && (x <= 0x5A));
}

bool GType::isUpper(const std::string x)
{
	std::string y = "";
	for (unsigned int i = 0; i < x.length(); ++i)
	{
		char letter = *x.substr(i, 1).c_str();
		if (!isUpper(letter))
			return false;
	}
	return true;
}

char GType::toUpper(char x)
{
	if (isLower(x))
		x -= 0x20;
	return x;
}

std::string GType::toUpper(const std::string x)
{
	std::string y = "";
	for (unsigned int i = 0; i < x.length(); ++i)
	{
		char letter = *x.substr(i, 1).c_str();
		letter = toUpper(letter);
		y += letter;
	}
	return y;
}

bool GType::isLower(const char x)
{
	return ((x >= 0x61) && (x <= 0x7A));
}

bool GType::isLower(const std::string x)
{
	std::string y = "";
	for (unsigned int i = 0; i < x.length(); ++i)
	{
		char letter = *x.substr(i, 1).c_str();
		if (!isLower(letter))
			return false;
	}
	return true;
}

char GType::toLower(char x)
{
	if ((x >= 0x41) && (x <= 0x5A))
		x += 0x20;
	return x;
}

std::string GType::toLower(const std::string x)
{
	std::string y = "";
	for (unsigned int i = 0; i < x.length(); ++i)
	{
		char letter = *x.substr(i, 1).c_str();
		letter = toLower(letter);
		y += letter;
	}
	return y;
}

char GType::toggleCase(char x)
{
	if (isUpper(x))
		return toLower(x);
	else if (isLower(x))
		return toUpper(x);
	else
		return x;
}

std::string GType::toggleCase(const std::string x)
{
	std::string y = "";
	for (unsigned int i = 0; i < x.length(); ++i)
	{
		char letter = *x.substr(i, 1).c_str();
		letter = toggleCase(letter);
		y += letter;
	}
	return y;
}

std::string GType::trim(char* x)
{
	std::string newStr(x);
	return trim(newStr);
}

std::string GType::trim(std::string x)
{
	while ((x.length() > 0) && (isWhitespace(x.substr(0, 1))))
		x = x.substr(1);
	while ((x.length() > 0) && (isWhitespace(x.substr(x.length() - 1, 1))))
		x = x.substr(0, x.length() - 1);
	return x;
}

std::string GType::charTOstring(const char number)
{
	std::stringstream ss;
	ss << number;
	return ss.str();
}

std::string GType::shortTOstring(const short number)
{
	std::stringstream ss;
	ss << number;
	return ss.str();
}

std::string GType::intTOstring(const int number)
{
	std::stringstream ss;
	ss << number;
	return ss.str();
}

std::string GType::GType::longTOstring(const int64_t number)
{
	std::stringstream ss;
	ss << number;
	return ss.str();
}

std::string GType::floatTOstring(const float number)
{
	int precision = 6; // automatically calculate this
	std::stringstream ss;
	if (precision <= 0)
		ss << number;
	else
		ss << std::fixed << std::setprecision(precision) << number;

	return ss.str();
}

std::string GType::doubleTOstring(const double number)
{
	std::stringstream ss;
	ss << number;
	return ss.str();
}
/*!
 * @brief convert timestamp to date string
 * @details convert a timestamp to the a string formatted for use in the Polygon API (mm-d-yyyy)
 * @param ts the timestamp to convert
 * @return the formatted date string
 */
std::string GType::datetimeTOstring(const int64_t ts)
{
	return (dateTOstring(ts) + " " + timeTOstring(ts));
}

std::string GType::dateTOstring(const int64_t ts)
{
	struct tm tstruct;
	tstruct = *localtime((time_t*)&ts);
	int cDay = tstruct.tm_mday;
	int cMonth = tstruct.tm_mon + 1;
	int cYear = tstruct.tm_year + 1900;
	int cHour = tstruct.tm_hour;
	int cMinute = tstruct.tm_min;
	int cSeconds = tstruct.tm_sec;

	time_t time = (time_t)ts;
	std::tm* date = localtime(&time);
	return intTOstring(cMonth) + "-" + intTOstring(cDay) + "-" + intTOstring(cYear);
}

std::string GType::timeTOstring(const int64_t ts)
{
	struct tm tstruct;
	tstruct = *localtime((time_t*)&ts);
	int cDay = tstruct.tm_mday;
	int cMonth = tstruct.tm_mon + 1;
	int cYear = tstruct.tm_year + 1900;
	int cHour = tstruct.tm_hour;
	int cMinute = tstruct.tm_min;
	int cSeconds = tstruct.tm_sec;

	time_t time = (time_t)ts;
	std::tm* date = localtime(&time);
	char buffer[80];
	sprintf(buffer, "%d:%d:%d", cHour, cMinute, cSeconds);
	std::string retString(buffer);
	return retString;
}

unsigned int GType::cfind(const char needle, const char* haystack, const unsigned int len)
{
	for (unsigned int i = 0; i < len; ++i)
	{
		if (haystack[i] == needle)
			return i;
	}

	return (unsigned int)std::string::npos;
}
