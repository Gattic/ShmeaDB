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
#include "GObject.h"

using namespace shmea;

GObject::GObject()
{
	//
}

GObject::GObject(const GObject& otherObj)
{
	copy(otherObj);
}

GObject::~GObject()
{
	clear();
}

void GObject::copy(const GObject& otherObj)
{
	memberTables = otherObj.memberTables;
}

/*!
 * @brief add GTable
 * @details add a GTable to the list of memberTables
 * @param item the GTable item to add
 */
void GObject::addTable(const GTable& item)
{
	memberTables.push_back(item);
}

/*!
 * @brief insert GTable
 * @details add a GTable to the list of memberTables at a certain position
 * @param index the index at which to add the item
 * @param item the GTable item to add
 */
void GObject::insertTable(unsigned int index, const GTable& item)
{
	if (index == memberTables.size())
		addTable(item);
	else
		memberTables.insert(memberTables.begin() + index, item);
}

void GObject::setTable(unsigned int index, const GTable& item)
{
	if (index >= memberTables.size())
		return;

	memberTables[index] = item;
}
void GObject::setMembers(const GTable& item)
{
	members = item;
}

void GObject::remove(unsigned int index)
{
	if (index >= memberTables.size())
		return;

	memberTables.erase(memberTables.begin() + index);
}

void GObject::clear()
{
	memberTables.clear();
}

GTable GObject::getTable(unsigned int index) const
{
	if (index >= memberTables.size())
		return 0;

	return memberTables[index];
}

GTable GObject::getMembers() const
{
	return members;
}

unsigned int GObject::size() const
{
	return memberTables.size();
}

bool GObject::empty() const
{
	return !(size() > 0);
}

GTable GObject::operator[](unsigned int index)
{
	if (index >= memberTables.size())
		return 0;

	return memberTables[index];
}

const GTable GObject::operator[](unsigned int index) const
{
	if (index >= memberTables.size())
		return 0;

	return memberTables[index];
}

void GObject::operator=(const GObject& otherObj)
{
	copy(otherObj);
}
