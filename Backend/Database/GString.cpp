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
	type = NULL_TYPE;
	blockSize = 0;
	block = NULL;
}

GString::GString(const GString& g2) : GType(g2)
{
	// Calling parent constructor
}

GString::GString(const GType& g2) : GType(g2)
{
	// Calling parent constructor
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
	if (getType() == CHAR_TYPE)
	{
		intFlag1 = true;
		intValue1 = getChar();
	}
	else if (getType() == SHORT_TYPE)
	{
		intFlag1 = true;
		intValue1 = getShort();
	}
	else if (getType() == INT_TYPE)
	{
		intFlag1 = true;
		intValue1 = getInt();
	}
	else if (getType() == LONG_TYPE)
	{
		intFlag1 = true;
		intValue1 = getLong();
	}
	else if (getType() == FLOAT_TYPE)
	{
		floatFlag1 = true;
		floatValue1 = getFloat();
	}
	else if (getType() == DOUBLE_TYPE)
	{
		floatFlag1 = true;
		floatValue1 = getDouble();
	}
	else if (getType() == BOOLEAN_TYPE)
	{
		boolFlag1 = true;
		boolValue1 = getBoolean();
	}
	else if (getType() == STRING_TYPE)
	{
		stringFlag1 = true;
		stringValue1 = getString();
	}

	// get the second value
	if (cCell2.getType() == CHAR_TYPE)
	{
		intFlag2 = true;
		intValue2 = cCell2.getChar();
	}
	else if (cCell2.getType() == SHORT_TYPE)
	{
		intFlag2 = true;
		intValue2 = cCell2.getShort();
	}
	else if (cCell2.getType() == INT_TYPE)
	{
		intFlag2 = true;
		intValue2 = cCell2.getInt();
	}
	else if (cCell2.getType() == LONG_TYPE)
	{
		intFlag2 = true;
		intValue2 = cCell2.getLong();
	}
	else if (cCell2.getType() == FLOAT_TYPE)
	{
		floatFlag2 = true;
		floatValue2 = cCell2.getFloat();
	}
	else if (cCell2.getType() == DOUBLE_TYPE)
	{
		floatFlag2 = true;
		floatValue2 = cCell2.getDouble();
	}
	else if (cCell2.getType() == BOOLEAN_TYPE)
	{
		boolFlag2 = true;
		boolValue2 = cCell2.getBoolean();
	}
	else if (cCell2.getType() == STRING_TYPE)
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

/*bool GString::isWhitespace(const char tempNumber)
{
	const std::string options = " \t\r\n";

	int breakPoint = options.find(tempNumber);
	return (breakPoint >= 0);
}

bool GString::isWhitespace(const char* tempNumber)
{
	std::string newStr(tempNumber);
	return isWhitespace(newStr);
}

bool GString::isWhitespace(const std::string tempNumber)
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

bool GString::isInteger(const char* tempNumber)
{
	std::string newStr(tempNumber);
	return isInteger(newStr);
}

bool GString::isInteger(const std::string tempNumber)
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

bool GString::isFloat(const char* tempNumber)
{
	std::string newStr(tempNumber);
	return isFloat(newStr);
}

bool GString::isFloat(const std::string tempNumber)
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

bool GString::isUpper(const char x)
{
	return ((x >= 0x41) && (x <= 0x5A));
}

bool GString::isUpper(const std::string x)
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

char GString::toUpper(char x)
{
	if (isLower(x))
		x -= 0x20;
	return x;
}

std::string GString::toUpper(const std::string x)
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

bool GString::isLower(const char x)
{
	return ((x >= 0x61) && (x <= 0x7A));
}

bool GString::isLower(const std::string x)
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

char GString::toLower(char x)
{
	if ((x >= 0x41) && (x <= 0x5A))
		x += 0x20;
	return x;
}

std::string GString::toLower(const std::string x)
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

char GString::toggleCase(char x)
{
	if (isUpper(x))
		return toLower(x);
	else if (isLower(x))
		return toUpper(x);
	else
		return x;
}

std::string GString::toggleCase(const std::string x)
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

std::string GString::trim(char* x)
{
	std::string newStr(x);
	return trim(newStr);
}

std::string GString::trim(std::string x)
{
	while ((x.length() > 0) && (isWhitespace(x.substr(0, 1))))
		x = x.substr(1);
	while ((x.length() > 0) && (isWhitespace(x.substr(x.length() - 1, 1))))
		x = x.substr(0, x.length() - 1);
	return x;
}

std::string GString::charTOstring(const char number)
{
	std::stringstream ss;
	ss << number;
	return ss.str();
}

std::string GString::shortTOstring(const short number)
{
	std::stringstream ss;
	ss << number;
	return ss.str();
}

std::string GString::intTOstring(const int number)
{
	std::stringstream ss;
	ss << number;
	return ss.str();
}

std::string GString::GString::longTOstring(const int64_t number)
{
	std::stringstream ss;
	ss << number;
	return ss.str();
}

std::string GString::floatTOstring(const float number)
{
	int precision = 6; // automatically calculate this
	std::stringstream ss;
	if (precision <= 0)
		ss << number;
	else
		ss << std::fixed << std::setprecision(precision) << number;

	return ss.str();
}

std::string GString::doubleTOstring(const double number)
{
	std::stringstream ss;
	ss << number;
	return ss.str();
}*/

/*!
 * @brief convert timestamp to date string
 * @details convert a timestamp to the a string formatted for use in the Polygon API (mm-d-yyyy)
 * @param ts the timestamp to convert
 * @return the formatted date string
 */
/*std::string GString::datetimeTOstring(const int64_t ts)
{
	return (dateTOstring(ts) + " " + timeTOstring(ts));
}

std::string GString::dateTOstring(const int64_t ts)
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

std::string GString::timeTOstring(const int64_t ts)
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

unsigned int GString::cfind(const char needle, const char* haystack, const unsigned int len)
{
	for (unsigned int i = 0; i < len; ++i)
	{
		if (haystack[i] == needle)
			return i;
	}

	return (unsigned int)std::string::npos;
}*/
