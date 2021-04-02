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
#include "maxid.h"

using namespace shmea;

std::map<GString, int64_t> MaxID::maxID;

int64_t MaxID::generateID(const GString& name)
{
	// load the max id into memory
	if (maxID.find(name) != maxID.end())
		loadMaxID(name);

	// increase the max id
	++maxID[name];

	saveMaxID(name);
	return maxID[name];
}

GString MaxID::buildMaxIDFName(const GString& name)
{
	if (name.length() == 0)
		return "";

	GString fname = "database/" + name + ".maxid";
	return fname;
}

void MaxID::loadMaxID(const GString& name)
{
	if (name.length() == 0)
		return;

	// Load from the id
	GString fname = buildMaxIDFName(name);

	// load the maxid from the file
	FILE* fd = fopen(fname.c_str_esc(), "r");
	printf("[DB] %c%s\n", (fd != NULL) ? '+' : '-', fname.c_str_esc());
	if (fd != NULL)
	{
		// get the file size
		fseek(fd, 0, SEEK_END);
		int64_t fSize = ftell(fd);
		fseek(fd, 0, SEEK_SET);

		// read in the data
		char* buffer = (char*)malloc(fSize * sizeof(char));
		int64_t newFSize = fread(buffer, 1, fSize, fd);
		if (newFSize != fSize)
		{
			printf("[DB] ~%s\n", fname.c_str_esc());
			fclose(fd);
		}

		// parse max id file
		parseMaxIDFile(name, buffer, fSize);

		// close the fd
		fclose(fd);
	}
}

void MaxID::saveMaxID(const GString& name)
{
	if (name.length() == 0)
		return;

	if (maxID.find(name) == maxID.end())
		return;

	// open the file for writing
	GString fname = buildMaxIDFName(name);
	FILE* fd = fopen(fname.c_str_esc(), "w");
	if (fd != NULL)
	{
		// save the max id
		printf("[DB] !%s\n", fname.c_str_esc());
		fprintf(fd, "%lld\n", maxID[name]);

		// close the fd
		fclose(fd);
	}
	else
		printf("[DB] Max ID Save Error\n");
}

void MaxID::parseMaxIDFile(const GString& name, char* fileContents, int64_t fSize)
{
	if (name.length() == 0)
		return;

	if (!fileContents)
		return;

	int64_t newMaxID = -1;
	int len = 0;
	while (fSize > 0)
	{
		if ((fileContents[len] == '\n') || (len == fSize))
		{
			// get the line contents
			char* line = (char*)malloc(sizeof(char) * (len + 1)); //+1 because of indexing
			bzero(line, len + 1);								  //+1 because of indexing
			memcpy(line, fileContents, len);

			int nl = (len == fSize) ? 0 : 1; // no nl if its the last line
			fSize -= len + nl;
			memcpy(fileContents, &fileContents[len + nl], fSize);
			bzero(&fileContents[fSize], len);

			// set the new max id
			newMaxID = atoll(line);

			// free the memory
			if (fSize == 0)
				free(fileContents);
			free(line);

			// set vars for new line
			len = 0;

			// done here
			break;
		}
		else
			++len;
	}

	maxID[name] = newMaxID;
}

int64_t MaxID::getMaxID(const GString& tableName)
{
	// requires a name
	if (tableName.length() == 0)
		return -1;

	// name already in memory
	if (maxID.find(tableName) != maxID.end())
		return maxID[tableName];

	// load the max id into memory
	loadMaxID(tableName);

	// return the max id if it loaded in correctly
	if (maxID.find(tableName) != maxID.end())
		return maxID[tableName];

	// load error
	return -1;
}
