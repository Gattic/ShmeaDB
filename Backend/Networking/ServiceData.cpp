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
#include "ServiceData.h"

using namespace GNet;

ServiceData::ServiceData(std::string newCommand)
{
	command = newCommand;
	listData = NULL;
	tableData = NULL;
}

ServiceData::ServiceData(std::string newCommand, shmea::GList* newList)
{
	command = newCommand;
	listData = newList;
	tableData = NULL;
}

ServiceData::ServiceData(std::string newCommand, shmea::GTable* newTable)
{
	command = newCommand;
	listData = NULL;
	tableData = newTable;
}

ServiceData::ServiceData(const ServiceData& instance2)
{
	command = instance2.command;
	listData = instance2.listData;
	tableData = instance2.tableData;
}

ServiceData::~ServiceData()
{
	command = "";
	listData = NULL;
	tableData = NULL;
}

std::string ServiceData::getCommand() const
{
	return command;
}

int ServiceData::getDataType() const
{
	return dataType;
}

void ServiceData::setCommand(std::string newCommand)
{
	command = newCommand;
}

void ServiceData::setDataType(int newDataType)
{
	dataType = newDataType;
}
