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
#ifndef _GSERIALIZABLE
#define _GSERIALIZABLE

#include "GList.h"
#include "GTable.h"
#include "GObject.h"
#include "ServiceData.h"
#include <algorithm>
#include <dirent.h>
#include <map>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/signal.h>
#include <sys/stat.h>
#include <vector>

namespace shmea {
class Serializable
{
private:
	static const std::string NEED_ESCAPING;
	static const char ESC_CHAR;

	static bool escapeSeparators(char**, int64_t&);
	static bool addDelimiter(char**, int64_t&, bool);
	static bool addItemToSerial(char**, int64_t&, char*, int, int, int);
	static bool serializeItem(const GType&, bool, char**, int64_t&);

	static bool isDelimiterAt(const char*, int, const char*);
	static int findNextDelimiterIndex(const char*, int, const char*, const int, const int);
	static bool isEscaped(const int, const char*);
	static int getDelimiterIdx(const char*, const int, const char*, const int, bool);

	static int deserializeType(char**, int64_t&, int64_t&);
	static int64_t deserializeSize(char**, int64_t&, int64_t&);
	static int unescapeCharacter(char**, int64_t&, int);
	static char* deserializeContent(char**, int64_t&, int64_t&, int64_t, int);

public:
	static int Serialize(const GList&, char**, bool = false); // to byte stream
	static int Serialize(const GTable&, char**, bool = false);	   // to byte stream
	static int Serialize(const GObject&, char**, bool = false);	   // to byte stream
	static int Serialize(const ServiceData*, char**);	   // to byte stream
	static void Deserialize(GList&, const char*, int64_t&, int = 0); // from byte stream; make this private?
	static void Deserialize(GTable&, const char*, int64_t&);	  // from byte stream
	static void Deserialize(GObject&, const char*, int64_t&);	  // from byte stream
	static void Deserialize(ServiceData*, const char*, int64_t);	  // from byte stream

	virtual GObject* serialize() const = 0;
	virtual void deserialize(const GObject*) = 0;

};
};

#endif
