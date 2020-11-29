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
#include "GType.h"

namespace shmea {

class GString : public GType
{
private:

public:
	GString();
	GString(const GString&);
	GString(const char*);
	virtual ~GString();

	// gets
	const char* c_str() const;
	unsigned int length() const;

	//== operators
	bool operator==(const GString&);

	//!= operators
	bool operator!=(const GString&);

	// helpers
	/*static bool isWhitespace(const char);
	static bool isWhitespace(const char*);
	static bool isWhitespace(const std::string);

	static bool isInteger(const char*);
	static bool isInteger(const std::string);

	static bool isFloat(const char*);
	static bool isFloat(const std::string);

	static bool isUpper(const char);
	static bool isUpper(const std::string);

	static char toUpper(char);
	static std::string toUpper(const std::string);

	static bool isLower(const char);
	static bool isLower(const std::string);

	static char toLower(char);
	static std::string toLower(const std::string);

	static char toggleCase(char);
	static std::string toggleCase(const std::string);

	static std::string trim(std::string);
	static std::string trim(char*);

	static std::string datetimeTOstring(const int64_t);
	static std::string dateTOstring(const int64_t);
	static std::string timeTOstring(const int64_t);

	static unsigned int cfind(const char, const char*, const unsigned int);*/
};
};

#endif
