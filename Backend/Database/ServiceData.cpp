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

ServiceData::ServiceData(GNet::Connection* newConnection, GString newCommand)
{
	cConnection = newConnection;
	timesent = 0;
	sid = generateSID();
	command = newCommand;
	serviceKey = "";
	type = TYPE_ACK;
	serviceNum = -1;
	responseServiceNum = -1;
}
ServiceData::ServiceData(const ServiceData& instance2)
{
	cConnection = instance2.cConnection;
	timesent = 0;
	sid = instance2.sid;
	command = instance2.command;
	serviceKey = instance2.serviceKey;
	repList = instance2.repList;
	repTable = instance2.repTable;
	repObj = instance2.repObj;
	type = instance2.type;
	serviceNum = instance2.serviceNum;
	responseServiceNum = instance2.responseServiceNum;
}

ServiceData::~ServiceData()
{
	cConnection = NULL;
	timesent = 0;
	sid = "";
	command = "";
	serviceKey = "";
	type = TYPE_ACK;
	serviceNum = -1;
	responseServiceNum = -1;
}

void ServiceData::set(GString newServiceKey)
{
	serviceKey = newServiceKey;
	type = TYPE_ACK;
}

void ServiceData::set(GString newServiceKey, const GList& newList)
{
	serviceKey = newServiceKey;
	repList = newList;
	type = TYPE_LIST;
}

void ServiceData::set(GString newServiceKey, const GTable& newTable)
{
	serviceKey = newServiceKey;
	repTable = newTable;
	type = TYPE_TABLE;
}

void ServiceData::set(GString newServiceKey, const GObject& newObj)
{
	serviceKey = newServiceKey;
	repObj = newObj;
	type = TYPE_NETWORK_POINTER;
}

void ServiceData::set(GString newServiceKey, const Serializable& newNP)
{
	serviceKey = newServiceKey;
	repObj = newNP.serialize();
	type = TYPE_NETWORK_POINTER;
}

void ServiceData::set(const GList& newList)
{
	serviceKey = "";
	repList = newList;
	type = TYPE_LIST;
}

void ServiceData::set(const GTable& newTable)
{
	serviceKey = "";
	repTable = newTable;
	type = TYPE_TABLE;
}

void ServiceData::set(const GObject& newObj)
{
	serviceKey = "";
	repObj = newObj;
	type = TYPE_NETWORK_POINTER;
}

void ServiceData::set(const Serializable& newNP)
{
	serviceKey = "";
	repObj = newNP.serialize();
	type = TYPE_NETWORK_POINTER;
}

const GList& ServiceData::getList() const
{
	return repList;
}

const GTable& ServiceData::getTable() const
{
	return repTable;
}

const GObject& ServiceData::getObj() const
{
	return repObj;
}

void ServiceData::setList(const GList& newList)
{
	repList = newList;
}

void ServiceData::setTable(const GTable& newTable)
{
	repTable = newTable;
}

void ServiceData::setObj(const GObject& newObj)
{
	repObj = newObj;
}

GNet::Connection* ServiceData::getConnection() const
{
	return cConnection;
}

int64_t ServiceData::getTimesent() const
{
	return timesent;
}

GString ServiceData::getSID() const
{
	return sid;
}

GString ServiceData::getCommand() const
{
	return command;
}

GString ServiceData::getServiceKey() const
{
	return serviceKey;
}

int64_t ServiceData::getServiceNum() const
{
	return serviceNum;
}

int64_t ServiceData::getResponseServiceNum() const
{
	return responseServiceNum;
}

int ServiceData::getType() const
{
	return type;
}

const GList& ServiceData::getArgList() const
{
	return argList;
}

void ServiceData::setTimesent(int64_t newTS)
{
	timesent = newTS;
}

void ServiceData::setSID(GString newSID)
{
	sid = newSID;
}

void ServiceData::setCommand(GString newCommand)
{
	command = newCommand;
}

void ServiceData::setServiceKey(GString newServiceKey)
{
	serviceKey = newServiceKey;
}


void ServiceData::assignServiceNum()
{
	static int64_t serviceCounter = 0;
	serviceNum = ++serviceCounter;
}

void ServiceData::assignResponseServiceNum()
{
	responseServiceNum = serviceNum + 1;
}

void ServiceData::setServiceNum(int64_t newServiceNum)
{
	serviceNum = newServiceNum;
}

void ServiceData::setResponseServiceNum(int64_t newResponseServiceNum)
{
	responseServiceNum = newResponseServiceNum;
}

void ServiceData::setType(int newType)
{
	type = newType;
}

void ServiceData::setArgList(const GList& newList)
{
	argList = newList;
}

bool ServiceData::validSID(const GString& testSID)
{
	if(testSID.length() != SID_LENGTH) return false;

	const GString options=
		"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!@#$%^&*()-_=+[{]};:,<.>/?";

	for(unsigned int i=0;i<testSID.length();++i)
	{
		int breakPoint=options.cfind(testSID[i]);
		if(breakPoint == -1) return false;
	}

	return true;
}

GString ServiceData::generateSID()
{
	const GString options=
		"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!@#$%^&*()-_=+[{]};:,<.>/?";
	GString newSID="";
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

bool ServiceData::operator<(const ServiceData& sd2) const
{
	return getServiceNum() < sd2.getServiceNum();
}

bool ServiceData::operator>(const ServiceData& sd2) const
{
	return getServiceNum() > sd2.getServiceNum();
}
