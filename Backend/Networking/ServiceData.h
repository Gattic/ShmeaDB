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
#ifndef _GSERVICEDATA
#define _GSERVICEDATA

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <time.h>
#include <vector>

namespace shmea {
	class GList;
	class GTable;
};

namespace GNet {

class ServiceData
{
private:

	std::string command;
	int dataType;

public:

	static const int TYPE_ACK = 0;
	static const int TYPE_LIST = 1;
	static const int TYPE_TABLE = 2;

	shmea::GList* listData;
	shmea::GTable* tableData;

	ServiceData(std::string);
	ServiceData(std::string, shmea::GList*);
	ServiceData(std::string, shmea::GTable*);
	ServiceData(const ServiceData&);
	~ServiceData();

	std::string getCommand() const;
	int getDataType() const;
	void setCommand(std::string);
	void setDataType(int);
};
};

#endif
