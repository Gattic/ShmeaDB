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
#include "savelist.h"
#include "gtable.h"
#include "maxid.h"
#include "saveitem.h"

using namespace shmea;

SaveList::SaveList(const std::string& newDirName)
{
	clean();
	dname = newDirName;
}

SaveList::~SaveList()
{
	clean();
}

std::string SaveList::getPath() const
{
	if (dname.length() == 0)
		return "";

	// get the env and build the path
	std::string dirname = "database/" + dname + "/";
	return dirname;
}

SaveItem* SaveList::loadItem(const std::string& siName)
{
	// database load single item
	SaveItem* newSV = new SaveItem(dname, siName);
	if (!newSV)
		return NULL;

	newSV->loadByName();
	addItem(newSV);
	return newSV;
}

bool SaveList::deleteItem(const std::string& siName)
{
	// database load single item
	SaveItem* newSV = new SaveItem(dname, siName);
	if (!newSV)
		return false;

	return newSV->deleteByName();
}

SaveItem* SaveList::newItem(const std::string& siName, const GTable& newTable)
{
	// create the directory if we need to
	struct stat info;
	std::string dirname = getPath();
	if (dirname.length() > 0)
	{
		if (stat(dirname.c_str(), &info) != 0)
		{
			// make the directory
			int status = mkdir(dirname.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
			if (status < 0)
			{
				printf("[DB] %s mkdir failed\n", dirname.c_str());
				// return;
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
			// return;
		}
	}

	// database load single item
	SaveItem* newSV = new SaveItem(dname, siName);
	newSV->saveByName(newTable);
	if (!newSV)
		addItem(newSV);

	return newSV;
}

void SaveList::load()
{
	if (dname.length() == 0)
		return;

	std::string folderName = getPath();
	DIR* dir = opendir(folderName.c_str());
	if (!dir)
	{
		printf("[DB] -%s\n", folderName.c_str());
		return;
	}

	// loop through the files in the directory
	struct dirent* ent = NULL;
	while ((ent = readdir(dir)) != NULL)
	{
		// don't want the current directory, parent or hidden files/folders
		std::string fname(ent->d_name);
		if (fname[0] == '.')
			continue;

		// Load each file by the name
		SaveItem* newSV = new SaveItem(dname, fname);
		newSV->loadByName();
		addItem(newSV);
	}

	closedir(dir);
}

std::vector<SaveList*> SaveList::loadFolders()
{
	std::string folderName = "database/";
	std::vector<SaveList*> folderList;

	DIR* dir;
	struct dirent* ent;
	if ((dir = opendir(folderName.c_str())) != NULL)
	{
		printf("[DB] -%s\n", folderName.c_str());
		return folderList;
	}

	// loop through the directory
	while ((ent = readdir(dir)) != NULL)
	{
		// don't want the current directory, parent or hidden files/folders
		std::string fname(ent->d_name);
		if (fname[0] == '.')
			continue;

		printf("Folder Name: %s \n", fname.c_str());
		SaveList* newSL = new SaveList(fname);
		newSL->load();
		folderList.push_back(newSL);
	}

	closedir(dir);
	return folderList;
}

std::string SaveList::getName() const
{
	return dname;
}

const std::vector<SaveItem*>& SaveList::getItems() const
{
	return saveItems;
}

int SaveList::size() const
{
	return saveItems.size();
}

void SaveList::addItem(SaveItem* newItem)
{
	if (newItem)
		saveItems.push_back(newItem);
}

void SaveList::clean()
{
	dname = "";
	saveItems.clear();
}
