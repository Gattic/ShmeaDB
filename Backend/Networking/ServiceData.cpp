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
#include "../Database/Serializable.h"

using namespace GNet;

ServiceData::ServiceData(Connection* newOrigin, Connection* newDestination,
	std::string newCommand)
{
	origin = newOrigin;
	destination = newDestination;
	sid = generateSID();
	command = newCommand;
	listData = NULL;
	tableData = NULL;
}

ServiceData::ServiceData(Connection* newOrigin, Connection* newDestination,
	std::string newCommand, shmea::GList* newList)
{
	origin = newOrigin;
	destination = newDestination;
	sid = generateSID();
	command = newCommand;
	listData = newList;
	tableData = NULL;
}

ServiceData::ServiceData(Connection* newOrigin, Connection* newDestination,
	std::string newCommand, shmea::GTable* newTable)
{
	origin = newOrigin;
	destination = newDestination;
	sid = generateSID();
	command = newCommand;
	listData = NULL;
	tableData = newTable;
}

ServiceData::ServiceData(Connection* newOrigin, Connection* newDestination,
	std::string newCommand, shmea::Serializable* newNP)
{
	origin = newOrigin;
	destination = newDestination;
	sid = generateSID();
	command = newCommand;
	listData = NULL;
	tableData = newNP->toGTable();
}

ServiceData::ServiceData(const ServiceData& instance2)
{
	origin = instance2.origin;
	destination = instance2.destination;
	sid = instance2.sid;
	command = instance2.command;
	listData = instance2.listData;
	tableData = instance2.tableData;
}

ServiceData::~ServiceData()
{
	origin = NULL;
	destination = NULL;
	sid = "";
	command = "";
	listData = NULL;
	tableData = NULL;
}

Connection* ServiceData::getOrigin()
{
	return origin;
}

Connection* ServiceData::getDestination()
{
	return destination;
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

bool ServiceData::validSID(const std::string& testSID)
{
	if(testSID.length() != SID_LENGTH) return false;

	const std::string options=
		"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!@#$%^&*()-_=+[{]};:,<.>/?";

	for(int i=0;i<testSID.length();++i)
	{
		int breakPoint=options.find(testSID[i]);
		if(breakPoint == -1) return false;
	}

	return true;
}

std::string ServiceData::generateSID()
{
	const std::string options=
		"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!@#$%^&*()-_=+[{]};:,<.>/?";
	std::string newSID="";
	do
	{
		newSID="";
		for(int i=0;i<SID_LENGTH;++i)
		{
			int newIndex=rand()%options.length();
			char newChar=options[newIndex];
			newSID+=newChar;
		}
	} while(!validSID(newSID));// && check if its in the data structure to avoid hijacking

	return newSID;
}
