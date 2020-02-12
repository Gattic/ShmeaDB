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
#include "saveitem.h"
#include "gtable.h"
#include "maxid.h"

using namespace shmea;

SaveItem::SaveItem(const std::string& newDirName, const std::string& newName)
{
	clean();
	dname = newDirName;
	name = newName;
}

SaveItem::~SaveItem()
{
	clean();
}

std::string SaveItem::getPath() const
{
	if (dname.length() == 0)
		return "";

	if (name.length() == 0)
		return "";

	// Build the path
	return "database/" + dname + "/" + name;
}

int64_t SaveItem::getID() const
{
	return id;
}

std::string SaveItem::getName() const
{
	return name;
}

GTable SaveItem::getTable() const
{
	return value;
}

void SaveItem::loadByName()
{
	if (dname.length() == 0)
		return;

	if (name.length() == 0)
		return;

	// Set the contents
	std::string fname = getPath();
	GTable newValue(fname, ',', GTable::TYPE_FILE);
	value = newValue;
}

void SaveItem::loadByID(int64_t newID)
{
	/*if (name.length() == 0)
		return;

	// Is the ID valid
	id = newID;
	if (id < 0)
		return;

	// is the id less than the max id?
	if (id > MaxID::getMaxID(name))
		return;

	// Set the contents now that we know the directory name
	loadByName(dname);*/
}

void SaveItem::saveByName(const GTable& newTable) const
{
	if (name.length() == 0)
		return;

	// Update the UID database
	// TODO

	// save the file
	std::string fname = getPath();
	newTable.save(fname);
}

void SaveItem::saveByID(const GTable& newTable)
{
	/*if (name.length() == 0)
		return;

	// generate a new id from maxID
	if (id < 0)
		id = MaxID::generateID(name);

	// save the file
	std::string fname = getPath();
	newTable.save(fname);*/
}

/*void SaveItem::saveUID(std::string nuid)
{
	struct stat info;
	std::string dirname = buildDataDir();
	if (dirname.length() > 0)
	{
		if (stat(dirname.c_str(), &info) != 0)
		{
			// make the directory
			int status = mkdir(dirname.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
			if (status < 0)
			{
				printf("[DB] %s mkdir failed\n", dirname.c_str());
				return;
			}
		}
		else if (info.st_mode & S_IFDIR)
		{
			// directory exists
			// do nothing
		}
		else
		{
			// path is not a directory
			printf("[DB] %s is not a directory\n", dirname.c_str());
			return;
		}
	}

	// open file for writing
	std::string fname = buildUIDPath(name);
	FILE* fd = fopen(fname.c_str(), "a");
	if (fd != NULL)
	{
		// save the unique id
		printf("[DB] !%s\n", fname.c_str());

		// check if ID already exist
		std::ifstream infile(fname.c_str());
		std::stringstream buffer;
		buffer << infile.rdbuf();
		std::string content = buffer.str();
		if (content.find(nuid) == -1)
			fprintf(fd, "%s\n", nuid.c_str());

		// close the fd
		fclose(fd);
	}
	else
		printf("[DB] Max ID Save Error\n");
}*/

bool SaveItem::deleteByName()
{
	if (dname.length() == 0)
		return false;

	if (name.length() == 0)
		return false;

	// Set the contents
	std::string fname = getPath();
	return (remove(fname.c_str()) == 0);
}

void SaveItem::clean()
{
	id = -1;
	dname = "";
	name = "";
}

void SaveItem::print() const
{
	if (name.length() == 0)
	{
		printf("[DB] Requires a name\n");
		return;
	}

	/*if (!value)
	{
		printf("[DB] -%s\n", getPath().c_str());
		return;
	}*/

	// Print the contents
	printf("File Name: %s\n", name.c_str());
	value.print();
}
