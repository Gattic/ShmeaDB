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
#ifndef _GSTRING
#define _GSTRING

#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <stdexcept>
#include "GType.h"

namespace shmea {

class GString : public GType
{
private:

public:
	GString();
	GString(const GString&);
	GString(const GType&);
	GString(const char*);
	GString(const char*, unsigned int);

	// Our GType constructors
	GString(const bool&);
	GString(const char&);
	GString(const short&);
	GString(const int&);
	GString(const int64_t&);
	GString(const float&);
	GString(const double&);

	virtual ~GString();

	// gets
	unsigned int length() const;

	//operators
	const char& operator[](const unsigned int&) const;
	char& operator[](const unsigned int&);
	bool operator==(const GString&) const;
	bool operator!=(const GString&) const;
	GString operator+(const char&) const;
	GString operator+(const GType&) const;
	GString operator+(const char*) const;
	GString operator+=(const char&);
	GString operator+=(const GType&);
	GString operator+=(const char*);

	// Strng Helpers
	GString substr(unsigned int, unsigned int=0) const;

	// members that call  GType helpers
	/*bool isWhitespace(const char);
	bool isWhitespace(const char*);
	bool isWhitespace(const std::string);

	bool isInteger(const char*);
	bool isInteger(const std::string);

	bool isFloat(const char*);
	bool isFloat(const std::string);

	bool isUpper(const char);
	bool isUpper(const std::string);

	char toUpper(char);
	std::string toUpper(const std::string);

	bool isLower(const char);
	bool isLower(const std::string);

	char toLower(char);
	std::string toLower(const std::string);

	char toggleCase(char);
	std::string toggleCase(const std::string);

	std::string trim(std::string);
	std::string trim(char*);

	std::string datetimeTOstring(const int64_t);
	std::string dateTOstring(const int64_t);
	std::string timeTOstring(const int64_t);

	unsigned int cfind(const char, const char*, const unsigned int);*/
};
};

#endif
