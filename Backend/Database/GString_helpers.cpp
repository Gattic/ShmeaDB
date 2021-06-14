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

// TODO: ADD UNIT TESTS
GType* GString::Typify(const char* word, unsigned int wordLen)
{
	// deduce the type flag
	int newType = STRING_TYPE;
	if (isInteger(word))
		newType = LONG_TYPE;
	else
	{
		newType = FLOAT_TYPE;
		if (!isFloat(word))
			newType = STRING_TYPE;
	}

	if (newType == FLOAT_TYPE)
	{
		float newBlock = atof(word);
		GType* retVar = new GType(newType, &newBlock, sizeof(float));
		return retVar;
	}
	else if (newType == LONG_TYPE)
	{
		// int
		/*int64_t newBlock=atol(word);
		void* pVal=NULL;
		if(newBlock < MAX_INT)
		{
			newType=INT_TYPE;
			if(newBlock < MAX_SHORT)
			{
				newType=SHORT_TYPE;
				if(newBlock < MAX_CHAR)
				{
					newType=CHAR_TYPE;
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

		int64_t newBlock = atoll(word);
		GType* retVar = new GType(newType, &newBlock, sizeof(int64_t));
		return retVar;
	}

	GType* retVar = new GType(newType, word, wordLen);
	return retVar;
}

// Member helpers
bool GString::isWhitespace() const
{
	const GString options(" \t\r\n", 4);

	for (unsigned int i = 0; i < size(); ++i)
	{
		unsigned int breakPoint = options.cfind(block[i]);
		if (breakPoint == npos)
			return false;
	}

	return true;
}

bool GString::isInteger() const
{
	const GString options = "0123456789";

	unsigned int negNumber = cfind('-'), i = 0;
	if ((negNumber != npos) && (negNumber > 0))
		return false;
	else if (negNumber == 0)
		i = 1;

	for (; i < size(); ++i)
	{
		unsigned int breakPoint = options.cfind(block[i]);
		if (breakPoint == npos)
			return false;
	}

	return true;
}

bool GString::isFloat() const
{
	const GString options = "0123456789.f";

	unsigned int negNumber = cfind('-'), i = 0;
	if ((negNumber != npos) && (negNumber > 0))
		return false;
	else if (negNumber == 0)
		i = 1;

	for (; i < size(); ++i)
	{
		unsigned int breakPoint = options.cfind(block[i]);
		if (breakPoint == npos)
			return false;
	}

	return true;
}

bool GString::isUpper() const
{
	for (unsigned int i = 0; i < size(); ++i)
	{
		char letter = block[i];
		if (!isUpper(letter))
			return false;
	}
	return true;
}

GString GString::toUpper() const
{
	GString y = "";
	for (unsigned int i = 0; i < size(); ++i)
	{
		char letter = block[i];
		letter = toUpper(letter);
		y += letter;
	}
	return y;
}

bool GString::isLower() const
{
	for (unsigned int i = 0; i < size(); ++i)
	{
		char letter = block[i];
		if (!isLower(letter))
			return false;
	}
	return true;
}

GString GString::toLower() const
{
	GString y = "";
	for (unsigned int i = 0; i < size(); ++i)
	{
		char letter = block[i];
		letter = toLower(letter);
		y += letter;
	}
	return y;
}

GString GString::toggleCase() const
{
	GString y = "";
	for (unsigned int i = 0; i < size(); ++i)
	{
		char letter = block[i];
		letter = toggleCase(letter);
		y += letter;
	}
	return y;
}

GString GString::trim() const
{
	GString y = *this;
	while ((y.size() > 0) && (isWhitespace(y[0])))
		y = y.substr(1);
	while ((y.size() > 0) && (isWhitespace(y[y.size() - 1])))
		y = y.substr(0, y.size() - 1);
	return y;
}

GString GString::Stringify() const
{
	if ((!block) || (size() == 0))
		return "";

	if (getType() == CHAR_TYPE)
	{
		return getChar();
	}
	else if (getType() == SHORT_TYPE)
	{
		return shortTOstring(getShort());
	}
	else if (getType() == INT_TYPE)
	{
		return intTOstring(getInt());
	}
	else if (getType() == LONG_TYPE)
	{
		return longTOstring(getLong());
	}
	else if (getType() == FLOAT_TYPE)
	{
		return floatTOstring(getFloat());
	}
	else if (getType() == DOUBLE_TYPE)
	{
		return doubleTOstring(getDouble());
	}
	else if (getType() == BOOLEAN_TYPE)
	{
		GString value = "";
		if (getBoolean())
			value = "True";
		else
			value = "False";
		return value;
	}

	// String Type (match)
	return *this;
}

// Static helpers
bool GString::isWhitespace(char tempNumber)
{
	const GString options = " \t\r\n";

	int breakPoint = options.cfind(tempNumber);
	return (breakPoint >= 0);
}

bool GString::isWhitespace(const char* str)
{
	GString gTemp(str, strlen(str));
	return gTemp.isWhitespace();
}

bool GString::isWhitespace(const char* str, unsigned int len)
{
	GString gTemp(str, len);
	return gTemp.isWhitespace();
}

bool GString::isInteger(const char* str)
{
	GString gTemp(str, strlen(str));
	return gTemp.isInteger();
}

bool GString::isInteger(const char* str, unsigned int len)
{
	GString gTemp(str, len);
	return gTemp.isInteger();
}

bool GString::isInteger(const GString& str)
{
	return str.isInteger();
}

bool GString::isFloat(const char* str)
{
	GString gTemp(str, strlen(str));
	return gTemp.isFloat();
}

bool GString::isFloat(const char* str, unsigned int len)
{
	GString gTemp(str, len);
	return gTemp.isFloat();
}

bool GString::isFloat(const GString& str)
{
	return str.isFloat();
}

bool GString::isUpper(char x)
{
	return ((x >= 0x41) && (x <= 0x5A));
}

bool GString::isUpper(const char* str)
{
	GString gTemp(str, strlen(str));
	return gTemp.isUpper();
}

bool GString::isUpper(const char* str, unsigned int len)
{
	GString gTemp(str, len);
	return gTemp.isUpper();
}

bool GString::isUpper(const GString& str)
{
	return str.isUpper();
}

char GString::toUpper(char x)
{
	if (isLower(x))
		x -= 0x20;
	return x;
}

GString GString::toUpper(const char* str)
{
	GString gTemp(str, strlen(str));
	return gTemp.toUpper();
}

GString GString::toUpper(const char* str, unsigned int len)
{
	GString gTemp(str, len);
	return gTemp.toUpper();
}

GString GString::toUpper(const GString& str)
{
	return str.toUpper();
}

bool GString::isLower(char x)
{
	return ((x >= 0x61) && (x <= 0x7A));
}

bool GString::isLower(const char* str)
{
	GString gTemp(str, strlen(str));
	return gTemp.isLower();
}

bool GString::isLower(const char* str, unsigned int len)
{
	GString gTemp(str, len);
	return gTemp.isLower();
}

bool GString::isLower(const GString& str)
{
	return str.isLower();
}

char GString::toLower(char x)
{
	if ((x >= 0x41) && (x <= 0x5A))
		x += 0x20;
	return x;
}

GString GString::toLower(const char* str)
{
	GString gTemp(str, strlen(str));
	return gTemp.toLower();
}

GString GString::toLower(const char* str, unsigned int len)
{
	GString gTemp(str, len);
	return gTemp.toLower();
}

GString GString::toLower(const GString& str)
{
	return str.toLower();
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

GString GString::toggleCase(const char* str)
{
	GString gTemp(str, strlen(str));
	return gTemp.toggleCase();
}

GString GString::toggleCase(const char* str, unsigned int len)
{
	GString gTemp(str, len);
	return gTemp.toggleCase();
}

GString GString::trim(const char*  str)
{
	GString gTemp(str, strlen(str));
	return gTemp.trim();
}

GString GString::trim(const char*  str, unsigned int len)
{
	GString gTemp(str, len);
	return gTemp.trim();
}

GString GString::trim(const GString&  str)
{
	return str.trim();
}

GString GString::charTOstring(char cChar)
{
	char buffer[20];
	sprintf(buffer, "%c", cChar);//make the number into string using sprintf function
	return GString(buffer, strlen(buffer));
}

GString GString::shortTOstring(short cShort)
{
	char buffer[20];
	sprintf(buffer, "%d", cShort);//make the number into string using sprintf function
	return GString(buffer, strlen(buffer));
}

GString GString::intTOstring(int cInt)
{
	char buffer[20];
	sprintf(buffer, "%d", cInt);//make the number into string using sprintf function
	return GString(buffer, strlen(buffer));
}

GString GString::longTOstring(int64_t cLong)
{
	char buffer[20];
	sprintf(buffer, "%ld", cLong);//make the number into string using sprintf function
	return GString(buffer, strlen(buffer));
}

GString GString::floatTOstring(float cFloat)
{
	char buffer[20];
	sprintf(buffer, "%f", cFloat);//make the number into string using sprintf function
	return GString(buffer, strlen(buffer));
}

GString GString::doubleTOstring(double cDouble)
{
	char buffer[20];
	sprintf(buffer, "%f", cDouble);//make the number into string using sprintf function
	return GString(buffer, strlen(buffer));
}

GString GString::datetimeTOstring(int64_t ts)
{
	GString newDateStr = (dateTOstring(ts) + " " + timeTOstring(ts));
	return newDateStr;
}

GString GString::dateTOstring(int64_t ts)
{
	//mm-d-yyyy
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
	GString newDateStr = intTOstring(cMonth) + "-" + intTOstring(cDay) + "-" + intTOstring(cYear);
	return newDateStr;
}

GString GString::timeTOstring(int64_t ts)
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
	GString retString = buffer;
	return retString;
}
