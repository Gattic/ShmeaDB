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
#include "connection.h"
#include "socket.h"
#include "../Database/ServiceData.h"

using namespace GNet;

Connection::Connection(int newSockFD, int newConnectionType, shmea::GString newIP)
{
	name = "";
	ip = newIP;
	sockfd = newSockFD;
	overflow = NULL;
	overflowLen = 0;
	connectionType = newConnectionType;
	key = 420l; // shouldnt matter what this value is
	finished = false;
}

Connection::Connection(const Connection& instance2)
{
	name = instance2.name;
	ip = instance2.ip;
	sockfd = instance2.sockfd;
	overflow = instance2.overflow;
	overflowLen = instance2.overflowLen;
	connectionType = instance2.connectionType;
	key = instance2.key; // shouldnt matter what this value is
	finished = instance2.finished;
}

Connection::~Connection()
{
	finish();

	name = "";
	ip = "";
	sockfd = -1;
	overflow = NULL;
	overflowLen = 0;
	connectionType = EMPTY_TYPE;
	key = 420l;
	finished = false;
}

void Connection::finish()
{
	// tell the client?

	// cleanup on next exitService
	finished = true;

	// close the connection
	close(this->sockfd);
	this->sockfd = -1;
}

shmea::GString Connection::getName() const
{
	return name;
}
shmea::GString Connection::getIP() const
{
	return ip;
}
int Connection::getConnectionType() const
{
	return connectionType;
}
int64_t Connection::getKey() const
{
	return key;
}
bool Connection::isFinished() const
{
	return finished;
}

void Connection::setName(shmea::GString newName)
{
	name = newName;
}

void Connection::setIP(shmea::GString newIP)
{
	ip = newIP;
}

void Connection::setKey(int64_t newKey)
{
	key = newKey;
}

bool Connection::validName(const shmea::GString& tempName)
{
	// Invalid Size
	if ((tempName.length() < 1) || (tempName.length() > 40))
		return false;

	const shmea::GString options =
		"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!@#$%^&*()-_=+[{]};:,<.>/?";

	for (unsigned int i = 0; i < tempName.length(); ++i)
	{
		int breakPoint = options.cfind(tempName[i]);
		if (breakPoint == -1)
			return false;
	}

	return true;
}

int64_t Connection::generateKey()
{
	const shmea::GString options = "0123456789";

	shmea::GString newKey = "";
	for (int i = 0; i < KEY_LENGTH; ++i)
	{
		int newIndex = rand() % options.length();
		char newChar = options[newIndex];
		newKey += newChar;
	}

	int64_t key = atoll(newKey.c_str_unesc());
	return key;
}
