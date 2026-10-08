// Copyright 2026 Robert Carneiro, Derek Meer, Matthew Tabak, Eric Lujan
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

bool SaveFolder::checkFolder()
{
	GString dirname = getPath();
	if (dirname.length() > 0)
	{
		if (!shmea::GDir::exists(dirname.c_str()))
		{
			if (!shmea::GDir::makeDir(dirname.c_str()))
			{
				printf("[DB] %s mkdir failed\n", dirname.c_str());
				return false;
			}
		}
		// GDir::exists() confirms it's a directory, not a file
	}
	return true;
}

SaveTable* SaveFolder::newItem(const GString& siName, const GTable& newTable)
{
	GString dirname = getPath();
	if (dirname.length() > 0)
	{
		if (!shmea::GDir::exists(dirname.c_str()))
		{
			if (!shmea::GDir::makeDir(dirname.c_str()))
			{
				printf("[DB] %s mkdir failed\n", dirname.c_str());
			}
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
	shmea::GDir dir;
	if (!dir.open(folderName.c_str()))
	{
		printf("[DB] -%s\n", folderName.c_str());
		return;
	}

	// loop through the files in the directory
	shmea::GDirEntry ent;
	while (dir.next(ent))
	{
		// Load each file by the name
		GString fname(ent.name.c_str());
		SaveTable* newSV = new SaveTable(dname, fname);
		newSV->loadByName();
		addItem(newSV);
	}
	// GDir::~GDir() closes automatically
}

std::vector<SaveFolder*> SaveFolder::loadFolders()
{
	GString folderName = "database/";
	std::vector<SaveFolder*> folderList;

	shmea::GDir dir;
	if (!dir.open(folderName.c_str()))
	{
		printf("[DB] -%s\n", folderName.c_str());
		return folderList;
	}

	// loop through the directory
	shmea::GDirEntry ent;
	while (dir.next(ent))
	{
		GString fname(ent.name.c_str());
		printf("Folder Name: %s \n", fname.c_str());
		SaveFolder* newSL = new SaveFolder(fname);
		newSL->load();
		folderList.push_back(newSL);
	}
	// GDir::~GDir() closes automatically

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
