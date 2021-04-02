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
#include "SaveFolder.h"
#include "GTable.h"
#include "SaveTable.h"

using namespace shmea;

SaveFolder::SaveFolder(const GString& newDirName)
{
	clean();
	dname = newDirName;
}

SaveFolder::~SaveFolder()
{
	clean();
}

GString SaveFolder::getPath() const
{
	if (dname.length() == 0)
		return "";

	// get the env and build the path
	GString dirname = "database/" + dname + "/";
	return dirname;
}

SaveTable* SaveFolder::loadItem(const GString& siName)
{
	// database load single item
	SaveTable* newSV = new SaveTable(dname, siName);
	if (!newSV)
		return NULL;

	newSV->loadByName();
	addItem(newSV);
	return newSV;
}

bool SaveFolder::deleteItem(const GString& siName)
{
	// database load single item
	SaveTable* newSV = new SaveTable(dname, siName);
	if (!newSV)
		return false;

	return newSV->deleteByName();
}

SaveTable* SaveFolder::newItem(const GString& siName, const GTable& newTable)
{
	// create the directory if we need to
	struct stat info;
	GString dirname = getPath();
	if (dirname.length() > 0)
	{
		if (stat(dirname.c_str_esc(), &info) != 0)
		{
			// make the directory
			int status = mkdir(dirname.c_str_esc(), S_IRWXU | S_IRWXG | S_IRWXO);
			if (status < 0)
			{
				printf("[DB] %s mkdir failed\n", dirname.c_str_esc());
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
			printf("[DB] %s is not a directory\n", dirname.c_str_esc());
			// return;
		}
	}

	// database load single item
	SaveTable* newSV = new SaveTable(dname, siName);
	newSV->saveByName(newTable);
	if (!newSV)
		addItem(newSV);

	return newSV;
}

void SaveFolder::load()
{
	if (dname.length() == 0)
		return;

	GString folderName = getPath();
	DIR* dir = opendir(folderName.c_str_esc());
	if (!dir)
	{
		printf("[DB] -%s\n", folderName.c_str_esc());
		return;
	}

	// loop through the files in the directory
	struct dirent* ent = NULL;
	while ((ent = readdir(dir)) != NULL)
	{
		// don't want the current directory, parent or hidden files/folders
		GString fname(ent->d_name);
		if (fname[0] == '.')
			continue;

		// Load each file by the name
		SaveTable* newSV = new SaveTable(dname, fname);
		newSV->loadByName();
		addItem(newSV);
	}

	closedir(dir);
}

std::vector<SaveFolder*> SaveFolder::loadFolders()
{
	GString folderName = "database/";
	std::vector<SaveFolder*> folderList;

	DIR* dir;
	struct dirent* ent;
	if ((dir = opendir(folderName.c_str_esc())) != NULL)
	{
		printf("[DB] -%s\n", folderName.c_str_esc());
		return folderList;
	}

	// loop through the directory
	while ((ent = readdir(dir)) != NULL)
	{
		// don't want the current directory, parent or hidden files/folders
		GString fname(ent->d_name);
		if (fname[0] == '.')
			continue;

		printf("Folder Name: %s \n", fname.c_str_esc());
		SaveFolder* newSL = new SaveFolder(fname);
		newSL->load();
		folderList.push_back(newSL);
	}

	closedir(dir);
	return folderList;
}

GString SaveFolder::getName() const
{
	return dname;
}

const std::vector<SaveTable*>& SaveFolder::getItems() const
{
	return saveItems;
}

int SaveFolder::size() const
{
	return saveItems.size();
}

void SaveFolder::addItem(SaveTable* newItem)
{
	if (newItem)
		saveItems.push_back(newItem);
}

void SaveFolder::clean()
{
	dname = "";
	saveItems.clear();
}
