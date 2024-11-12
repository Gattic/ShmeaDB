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

GType::operator const char*() const
{
	return c_str();
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

GType::operator bool() const
{
	return getBoolean();
}

//==========EQUALS SET==========

GType& GType::operator=(const GType& compValue)
{
	set(compValue.getType(), compValue.block, compValue.size());
	return *this;
}

GType& GType::operator=(const char& compValue)
{
	set(CHAR_TYPE, &compValue, sizeof(char));
	return *this;
}

GType& GType::operator=(const short& compValue)
{
	set(SHORT_TYPE, &compValue, sizeof(short));
	return *this;
}

GType& GType::operator=(const int& compValue)
{
	set(INT_TYPE, &compValue, sizeof(int));
	return *this;
}

GType& GType::operator=(const int64_t& compValue)
{
	set(LONG_TYPE, &compValue, sizeof(int));
	return *this;
}

GType& GType::operator=(const float& compValue)
{
	set(FLOAT_TYPE, &compValue, sizeof(float));
	return *this;
}

GType& GType::operator=(const double& compValue)
{
	set(DOUBLE_TYPE, &compValue, sizeof(double));
	return *this;
}

GType& GType::operator=(const char* compValue)
{
	GType newCompValue(compValue);
	set(STRING_TYPE, newCompValue.c_str(), newCompValue.size());
	return *this;
}

GType& GType::operator=(const bool& compValue)
{
	set(BOOLEAN_TYPE, &compValue, sizeof(bool));
	return *this;
}

//==========EQUALS COMPARISON==========

bool GType::operator==(const GType& cCell2) const
{
	// compare the known types
	bool intFlag1 = false;
	bool intFlag2 = false;
	bool floatFlag1 = false;
	bool floatFlag2 = false;
	bool doubleFlag1 = false;
	bool doubleFlag2 = false;
	bool stringFlag1 = false;
	bool stringFlag2 = false;
	bool boolFlag1 = false;
	bool boolFlag2 = false;

	// values
	int64_t intValue1 = 0;
	double floatValue1 = 0.0f;
	double doubleValue1 = 0.0f;
	bool boolValue1 = false;
	int64_t intValue2 = 0;
	double floatValue2 = 0.0f;
	double doubleValue2 = 0.0f;
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
		doubleFlag1 = true;
		doubleValue1 = getDouble();
	}
	else if (getType() == GType::BOOLEAN_TYPE)
	{
		boolFlag1 = true;
		boolValue1 = getBoolean();
	}
	else if (getType() == GType::STRING_TYPE)
	{
		stringFlag1 = true;
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
		doubleFlag2 = true;
		doubleValue2 = cCell2.getDouble();
	}
	else if (cCell2.getType() == GType::BOOLEAN_TYPE)
	{
		boolFlag2 = true;
		boolValue2 = cCell2.getBoolean();
	}
	else if (cCell2.getType() == GType::STRING_TYPE)
	{
		stringFlag2 = true;
	}

	// compare the gtypes
	if ((intFlag1) && (intFlag2))
		return (intValue1 == intValue2);
	else if ((floatFlag1) && (floatFlag2))
		return (floatValue1 == floatValue2);
	else if ((doubleFlag1) && (doubleFlag2))
		return (doubleValue1 == doubleValue2);
	else if ((stringFlag1) && (stringFlag2))
		return ((size() == cCell2.size()) && (strncmp(block, cCell2.block, size()) == 0));//strings
	else if ((boolFlag1) && (boolFlag2))
		return (boolValue1 == boolValue2);
	// cross ints and floats
	else if ((intFlag1) && (floatFlag2))
		return (intValue1 == floatValue2);
	else if ((intFlag1) && (doubleFlag2))
		return (intValue1 == doubleValue2);
	else if ((floatFlag1) && (intFlag2))
		return (floatValue1 == intValue2);
	else if ((floatFlag1) && (doubleFlag2))
		return (floatValue1 == doubleValue2);
	else if ((doubleFlag1) && (intFlag2))
		return (doubleValue1 == intValue2);
	else if ((doubleFlag1) && (floatFlag2))
		return (doubleValue1 == floatValue2);

	return false;
}

bool GType::operator==(const char& compValue) const
{
	return getChar() == compValue;
}

bool GType::operator==(const short& compValue) const
{
	return getShort() == compValue;
}

bool GType::operator==(const int& compValue) const
{
	return getInt() == compValue;
}

bool GType::operator==(const int64_t& compValue) const
{
	return getLong() == compValue;
}

bool GType::operator==(const float& compValue) const
{
	return getFloat() == compValue;
}

bool GType::operator==(const double& compValue) const
{
	return getDouble() == compValue;
}

bool GType::operator==(const char* compValue) const
{
	if (this->blockSize != strlen(compValue)) return false;
	return (strncmp(block, compValue, strlen(compValue)) == 0);
}

bool GType::operator==(const bool& compValue) const
{
	return getBoolean() == compValue;
}

//==========NOT EQUALS==========

bool GType::operator!=(const GType& compValue) const
{
	return !((*this) == compValue);
}

bool GType::operator!=(const char& compValue) const
{
	return !((*this) == compValue);
}

bool GType::operator!=(const short& compValue) const
{
	return !((*this) == compValue);
}

bool GType::operator!=(const int& compValue) const
{
	return !((*this) == compValue);
}

bool GType::operator!=(const int64_t& compValue) const
{
	return !((*this) == compValue);
}

bool GType::operator!=(const float& compValue) const
{
	return !((*this) == compValue);
}

bool GType::operator!=(const double& compValue) const
{
	return !((*this) == compValue);
}

bool GType::operator!=(const char* compValue) const
{
	GType newCompValue(compValue);
	return !((*this) == compValue);
}

bool GType::operator!=(const bool& compValue) const
{
	return !((*this) == compValue);
}

//==========LESS THAN==========

bool GType::operator<(const GType& cCell2) const
{
	// compare the known types
	bool intFlag1 = false;
	bool intFlag2 = false;
	bool floatFlag1 = false;
	bool floatFlag2 = false;
	bool doubleFlag1 = false;
	bool doubleFlag2 = false;
	bool stringFlag1 = false;
	bool stringFlag2 = false;
	bool boolFlag1 = false;
	bool boolFlag2 = false;

	// values
	int64_t intValue1 = 0;
	double floatValue1 = 0.0f;
	double doubleValue1 = 0.0f;
	bool boolValue1 = false;
	int64_t intValue2 = 0;
	double floatValue2 = 0.0f;
	double doubleValue2 = 0.0f;
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
		doubleFlag1 = true;
		doubleValue1 = getDouble();
	}
	else if (getType() == GType::BOOLEAN_TYPE)
	{
		boolFlag1 = true;
		boolValue1 = getBoolean();
	}
	else if (getType() == GType::STRING_TYPE)
	{
		stringFlag1 = true;
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
		doubleFlag2 = true;
		doubleValue2 = cCell2.getDouble();
	}
	else if (cCell2.getType() == GType::BOOLEAN_TYPE)
	{
		boolFlag2 = true;
		boolValue2 = cCell2.getBoolean();
	}
	else if (cCell2.getType() == GType::STRING_TYPE)
	{
		stringFlag2 = true;
	}

	// compare the gtypes
	if ((intFlag1) && (intFlag2))
		return (intValue1 < intValue2);
	else if ((floatFlag1) && (floatFlag2))
		return (floatValue1 < floatValue2);
	else if ((doubleFlag1) && (doubleFlag2))
		return (doubleValue1 < doubleValue2);
	else if ((stringFlag1) && (stringFlag2))
		return (strncmp(block, cCell2.block, size()) < 0);//strings
	else if ((boolFlag1) && (boolFlag2))
		return (boolValue1 < boolValue2);
	// cross ints and floats
	else if ((intFlag1) && (floatFlag2))
		return (intValue1 < floatValue2);
	else if ((intFlag1) && (doubleFlag2))
		return (intValue1 < doubleValue2);
	else if ((floatFlag1) && (intFlag2))
		return (floatValue1 < intValue2);
	else if ((floatFlag1) && (doubleFlag2))
		return (floatValue1 < doubleValue2);
	else if ((doubleFlag1) && (intFlag2))
		return (doubleValue1 < intValue2);
	else if ((doubleFlag1) && (floatFlag2))
		return (doubleValue1 < floatValue2);

	return false;
}

bool GType::operator<(const char& compValue) const
{
	return getChar() < compValue;
}

bool GType::operator<(const short& compValue) const
{
	return getShort() < compValue;
}

bool GType::operator<(const int& compValue) const
{
	return getInt() < compValue;
}

bool GType::operator<(const int64_t& compValue) const
{
	return getLong() < compValue;
}

bool GType::operator<(const float& compValue) const
{
	return getFloat() < compValue;
}

bool GType::operator<(const double& compValue) const
{
	return getDouble() < compValue;
}

bool GType::operator<(const char* compValue) const
{
	GType cCell(compValue);
	return ((*this) < cCell);
}

bool GType::operator<(const bool& compValue) const
{
	return getBoolean() < compValue;
}

//==========GREATER THAN==========

bool GType::operator>(const GType& cCell2) const
{
	// compare the known types
	bool intFlag1 = false;
	bool intFlag2 = false;
	bool floatFlag1 = false;
	bool floatFlag2 = false;
	bool doubleFlag1 = false;
	bool doubleFlag2 = false;
	bool stringFlag1 = false;
	bool stringFlag2 = false;
	bool boolFlag1 = false;
	bool boolFlag2 = false;

	// values
	int64_t intValue1 = 0;
	double floatValue1 = 0.0f;
	double doubleValue1 = 0.0f;
	bool boolValue1 = false;
	int64_t intValue2 = 0;
	double floatValue2 = 0.0f;
	double doubleValue2 = 0.0f;
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
		doubleFlag1 = true;
		doubleValue1 = getDouble();
	}
	else if (getType() == GType::BOOLEAN_TYPE)
	{
		boolFlag1 = true;
		boolValue1 = getBoolean();
	}
	else if (getType() == GType::STRING_TYPE)
	{
		stringFlag1 = true;
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
		doubleFlag2 = true;
		doubleValue2 = cCell2.getDouble();
	}
	else if (cCell2.getType() == GType::BOOLEAN_TYPE)
	{
		boolFlag2 = true;
		boolValue2 = cCell2.getBoolean();
	}
	else if (cCell2.getType() == GType::STRING_TYPE)
	{
		stringFlag2 = true;
	}

	// compare the gtypes
	if ((intFlag1) && (intFlag2))
		return (intValue1 > intValue2);
	else if ((floatFlag1) && (floatFlag2))
		return (floatValue1 > floatValue2);
	else if ((doubleFlag1) && (doubleFlag2))
		return (doubleValue1 > doubleValue2);
	else if ((stringFlag1) && (stringFlag2))
		return (strncmp(block, cCell2.block, size()) > 0);//strings
	else if ((boolFlag1) && (boolFlag2))
		return (boolValue1 > boolValue2);
	// cross ints and floats
	else if ((intFlag1) && (floatFlag2))
		return (intValue1 > floatValue2);
	else if ((intFlag1) && (doubleFlag2))
		return (intValue1 > doubleValue2);
	else if ((floatFlag1) && (intFlag2))
		return (floatValue1 > intValue2);
	else if ((floatFlag1) && (doubleFlag2))
		return (floatValue1 > doubleValue2);
	else if ((doubleFlag1) && (intFlag2))
		return (doubleValue1 > intValue2);
	else if ((doubleFlag1) && (floatFlag2))
		return (doubleValue1 > floatValue2);

	return false;
}

bool GType::operator>(const char& compValue) const
{
	return getChar() > compValue;
}

bool GType::operator>(const short& compValue) const
{
	return getShort() > compValue;
}

bool GType::operator>(const int& compValue) const
{
	return getInt() > compValue;
}

bool GType::operator>(const int64_t& compValue) const
{
	return getLong() > compValue;
}

bool GType::operator>(const float& compValue) const
{
	return getFloat() > compValue;
}

bool GType::operator>(const double& compValue) const
{
	return getDouble() > compValue;
}

bool GType::operator>(const char* compValue) const
{
	GType cCell(compValue);
	return ((*this) > cCell);
}

bool GType::operator>(const bool& compValue) const
{
	return getBoolean() > compValue;
}

//==========LESS THAN EQUALS TO==========

bool GType::operator<=(const GType& cCell2) const
{
	// compare the known types
	bool intFlag1 = false;
	bool intFlag2 = false;
	bool floatFlag1 = false;
	bool floatFlag2 = false;
	bool doubleFlag1 = false;
	bool doubleFlag2 = false;
	bool stringFlag1 = false;
	bool stringFlag2 = false;
	bool boolFlag1 = false;
	bool boolFlag2 = false;

	// values
	int64_t intValue1 = 0;
	double floatValue1 = 0.0f;
	double doubleValue1 = 0.0f;
	bool boolValue1 = false;
	int64_t intValue2 = 0;
	double floatValue2 = 0.0f;
	double doubleValue2 = 0.0f;
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
		doubleFlag1 = true;
		doubleValue1 = getDouble();
	}
	else if (getType() == GType::BOOLEAN_TYPE)
	{
		boolFlag1 = true;
		boolValue1 = getBoolean();
	}
	else if (getType() == GType::STRING_TYPE)
	{
		stringFlag1 = true;
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
		doubleFlag2 = true;
		doubleValue2 = cCell2.getDouble();
	}
	else if (cCell2.getType() == GType::BOOLEAN_TYPE)
	{
		boolFlag2 = true;
		boolValue2 = cCell2.getBoolean();
	}
	else if (cCell2.getType() == GType::STRING_TYPE)
	{
		stringFlag2 = true;
	}

	// compare the gtypes
	if ((intFlag1) && (intFlag2))
		return (intValue1 <= intValue2);
	else if ((floatFlag1) && (floatFlag2))
		return (floatValue1 <= floatValue2);
	else if ((doubleFlag1) && (doubleFlag2))
		return (doubleValue1 <= doubleValue2);
	else if ((stringFlag1) && (stringFlag2))
		return (strncmp(block, cCell2.block, size()) <= 0);//strings
	else if ((boolFlag1) && (boolFlag2))
		return (boolValue1 <= boolValue2);
	// cross ints and floats
	else if ((intFlag1) && (floatFlag2))
		return (intValue1 <= floatValue2);
	else if ((intFlag1) && (doubleFlag2))
		return (intValue1 <= doubleValue2);
	else if ((floatFlag1) && (intFlag2))
		return (floatValue1 <= intValue2);
	else if ((floatFlag1) && (doubleFlag2))
		return (floatValue1 <= doubleValue2);
	else if ((doubleFlag1) && (intFlag2))
		return (doubleValue1 <= intValue2);
	else if ((doubleFlag1) && (floatFlag2))
		return (doubleValue1 <= floatValue2);

	return false;
}

bool GType::operator<=(const char& compValue) const
{
	return getChar() <= compValue;
}

bool GType::operator<=(const short& compValue) const
{
	return getShort() <= compValue;
}

bool GType::operator<=(const int& compValue) const
{
	return getInt() <= compValue;
}

bool GType::operator<=(const int64_t& compValue) const
{
	return getLong() <= compValue;
}

bool GType::operator<=(const float& compValue) const
{
	return getFloat() <= compValue;
}

bool GType::operator<=(const double& compValue) const
{
	return getDouble() <= compValue;
}

bool GType::operator<=(const char* compValue) const
{
	GType cCell(compValue);
	return ((*this) <= cCell);
}

bool GType::operator<=(const bool& compValue) const
{
	return getBoolean() <= compValue;
}

//==========GREATER THAN EQUALS TO==========

bool GType::operator>=(const GType& cCell2) const
{
	// compare the known types
	bool intFlag1 = false;
	bool intFlag2 = false;
	bool floatFlag1 = false;
	bool floatFlag2 = false;
	bool doubleFlag1 = false;
	bool doubleFlag2 = false;
	bool stringFlag1 = false;
	bool stringFlag2 = false;
	bool boolFlag1 = false;
	bool boolFlag2 = false;

	// values
	int64_t intValue1 = 0;
	double floatValue1 = 0.0f;
	double doubleValue1 = 0.0f;
	bool boolValue1 = false;
	int64_t intValue2 = 0;
	double floatValue2 = 0.0f;
	double doubleValue2 = 0.0f;
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
		doubleFlag1 = true;
		doubleValue1 = getDouble();
	}
	else if (getType() == GType::BOOLEAN_TYPE)
	{
		boolFlag1 = true;
		boolValue1 = getBoolean();
	}
	else if (getType() == GType::STRING_TYPE)
	{
		stringFlag1 = true;
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
		doubleFlag2 = true;
		doubleValue2 = cCell2.getDouble();
	}
	else if (cCell2.getType() == GType::BOOLEAN_TYPE)
	{
		boolFlag2 = true;
		boolValue2 = cCell2.getBoolean();
	}
	else if (cCell2.getType() == GType::STRING_TYPE)
	{
		stringFlag2 = true;
	}

	// compare the gtypes
	if ((intFlag1) && (intFlag2))
		return (intValue1 >= intValue2);
	else if ((floatFlag1) && (floatFlag2))
		return (floatValue1 >= floatValue2);
	else if ((doubleFlag1) && (doubleFlag2))
		return (doubleValue1 >= doubleValue2);
	else if ((stringFlag1) && (stringFlag2))
		return (strncmp(block, cCell2.block, size()) >= 0);//strings
	else if ((boolFlag1) && (boolFlag2))
		return (boolValue1 >= boolValue2);
	// cross ints and floats
	else if ((intFlag1) && (floatFlag2))
		return (intValue1 >= floatValue2);
	else if ((intFlag1) && (doubleFlag2))
		return (intValue1 >= doubleValue2);
	else if ((floatFlag1) && (intFlag2))
		return (floatValue1 >= intValue2);
	else if ((floatFlag1) && (doubleFlag2))
		return (floatValue1 >= doubleValue2);
	else if ((doubleFlag1) && (intFlag2))
		return (doubleValue1 >= intValue2);
	else if ((doubleFlag1) && (floatFlag2))
		return (doubleValue1 >= floatValue2);
	return false;
}


bool GType::operator>=(const char& compValue) const
{
	return getChar() >= compValue;
}

bool GType::operator>=(const short& compValue) const
{
	return getShort() >= compValue;
}

bool GType::operator>=(const int& compValue) const
{
	return getInt() >= compValue;
}

bool GType::operator>=(const int64_t& compValue) const
{
	return getLong() >= compValue;
}

bool GType::operator>=(const float& compValue) const
{
	return getFloat() >= compValue;
}

bool GType::operator>=(const double& compValue) const
{
	return getDouble() >= compValue;
}

bool GType::operator>=(const char* compValue) const
{
	GType cCell(compValue);
	return ((*this) >= cCell);
}

bool GType::operator>=(const bool& compValue) const
{
	return getBoolean() >= compValue;
}
