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

using namespace shmea;

ServiceData::ServiceData(GNet::Connection* newConnection)
{
	cConnection = newConnection;
	sid = generateSID();
	command = "";
	repList = NULL;
	repTable = NULL;
	repObj = NULL;
	type = TYPE_ACK;
}

ServiceData::ServiceData(GNet::Connection* newConnection, std::string newCommand)
{
	cConnection = newConnection;
	sid = generateSID();
	command = newCommand;
	repList = NULL;
	repTable = NULL;
	repObj = NULL;
	type = TYPE_ACK;
}

ServiceData::ServiceData(GNet::Connection* newConnection, std::string newCommand, GList* newList)
{
	cConnection = newConnection;
	sid = generateSID();
	command = newCommand;
	repList = newList;
	repTable = NULL;
	repObj = NULL;
	type = TYPE_LIST;
}

ServiceData::ServiceData(GNet::Connection* newConnection, std::string newCommand, GTable* newTable)
{
	cConnection = newConnection;
	sid = generateSID();
	command = newCommand;
	repList = NULL;
	repTable = newTable;
	repObj = NULL;
	type = TYPE_TABLE;
}

ServiceData::ServiceData(GNet::Connection* newConnection, std::string newCommand, Serializable* newNP)
{
	cConnection = newConnection;
	sid = generateSID();
	command = newCommand;
	repList = NULL;
	repTable = NULL;
	repObj = newNP->serialize();
	type = TYPE_NETWORK_POINTER;
}

ServiceData::ServiceData(const ServiceData& instance2)
{
	cConnection = instance2.cConnection;
	sid = instance2.sid;
	command = instance2.command;
	repList = instance2.repList;
	repTable = instance2.repTable;
	repObj = instance2.repObj;
	type = instance2.type;
}

ServiceData::~ServiceData()
{
	cConnection = NULL;
	sid = "";
	command = "";
	repList = NULL;
	repTable = NULL;
	repObj = NULL;
	type = TYPE_ACK;
}

const GList* ServiceData::getList() const
{
	return repList;
}

const GTable* ServiceData::getTable() const
{
	return repTable;
}

const GObject* ServiceData::getObj() const
{
	return repObj;
}

void ServiceData::setList(GList* newList)
{
	if(!newList)
		return;

	repList = newList;
}

void ServiceData::setTable(GTable* newTable)
{
	if(!newTable)
		return;

	repTable = newTable;
}

void ServiceData::setObj(GObject* newObj)
{
	if(!newObj)
		return;

	repObj = newObj;
}

GNet::Connection* ServiceData::getConnection() const
{
	return cConnection;
}

std::string ServiceData::getSID() const
{
	return sid;
}

std::string ServiceData::getCommand() const
{
	return command;
}

int ServiceData::getType() const
{
	return type;
}

void ServiceData::setSID(std::string newSID)
{
	sid = newSID;
}

void ServiceData::setCommand(std::string newCommand)
{
	command = newCommand;
}

void ServiceData::setType(int newType)
{
	type = newType;
}

bool ServiceData::validSID(const std::string& testSID)
{
	if(testSID.length() != SID_LENGTH) return false;

	const std::string options=
		"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!@#$%^&*()-_=+[{]};:,<.>/?";

	for(unsigned int i=0;i<testSID.length();++i)
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
		for(unsigned int i=0;i<SID_LENGTH;++i)
		{
			int newIndex=rand()%options.length();
			char newChar=options[newIndex];
			newSID+=newChar;
		}
	} while(!validSID(newSID));// && check if its in the data structure to avoid hijacking

	return newSID;
}
