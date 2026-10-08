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
#include "connection.h"
#include "socket.h"
#include "../Database/ServiceData.h"

using namespace GNet;

Connection::Connection(int newSockFD, int newConnectionType, shmea::GString newIP)
{
	name = "";
	ip = newIP;
	port = "";
	sockfd = newSockFD;
	overflow = "";
	connectionType = newConnectionType;
	cryptEnabled = true;
	key = 420l; // shouldnt matter what this value is
	finished = false;
	protocol = PROTO_TCP;
	closeOnFinish = true;
	inFlightServices = 0;
	pendingSends = 0;
	lastSeenSec = (int64_t)time(NULL);
}

Connection::Connection(int newSockFD, int newConnectionType, shmea::GString newIP, shmea::GString newPort)
{
	name = "";
	ip = newIP;
	port = newPort;
	sockfd = newSockFD;
	overflow = "";
	connectionType = newConnectionType;
	cryptEnabled = true;
	key = 420l; // shouldnt matter what this value is
	finished = false;
	protocol = PROTO_TCP;
	closeOnFinish = true;
	inFlightServices = 0;
	pendingSends = 0;
	lastSeenSec = (int64_t)time(NULL);
}

Connection::~Connection()
{
	finish();

	name = "";
	ip = "";
	port = "";
	sockfd = INVALID_SOCKET_VALUE;
	connectionType = EMPTY_TYPE;
	cryptEnabled = true;
	key = 420l;
	finished = false;
}

void Connection::finish()
{
	// tell the client?

	// cleanup on next exitService
	finished = true;

	// close the connection
	if (closeOnFinish)
		G_CLOSE_SOCKET(this->sockfd);
	this->sockfd = INVALID_SOCKET_VALUE;
}

shmea::GString Connection::getName() const
{
	return name;
}
shmea::GString Connection::getIP() const
{
	return ip;
}
shmea::GString Connection::getPort() const
{
	return port;
}
int Connection::getConnectionType() const
{
	return connectionType;
}
int64_t Connection::getKey() const
{
	return key;
}

bool Connection::isEncrypted() const
{
	return cryptEnabled;
}

bool Connection::isFinished() const
{
	return finished;
}

int Connection::getProtocol() const
{
	return protocol;
}

int64_t Connection::getLastSeenSec() const
{
	return lastSeenSec;
}

unsigned int Connection::getInFlightServices() const
{
	return (unsigned int)(inFlightServices < 0 ? 0 : inFlightServices);
}

unsigned int Connection::getPendingSends() const
{
	return (unsigned int)(pendingSends < 0 ? 0 : pendingSends);
}

void Connection::setName(shmea::GString newName)
{
	name = newName;
}

void Connection::setIP(shmea::GString newIP)
{
	ip = newIP;
}

void Connection::setPort(shmea::GString newPort)
{
	port = newPort;
}

void Connection::enableEncryption()
{
	cryptEnabled = true;
}

void Connection::disableEncryption()
{
	cryptEnabled = false;
}

void Connection::setKey(int64_t newKey)
{
	key = newKey;
}

void Connection::setProtocol(int newProtocol)
{
	protocol = newProtocol;
}

void Connection::setCloseOnFinish(bool value)
{
	closeOnFinish = value;
}

void Connection::noteSeen()
{
	int64_t now = (int64_t)time(NULL);
#if defined(__GNUC__)
	__sync_lock_test_and_set(&lastSeenSec, now);
#else
	lastSeenSec = now;
#endif
}

void Connection::incInFlight()
{
#if defined(__GNUC__)
	__sync_add_and_fetch(&inFlightServices, 1);
#else
	++inFlightServices;
#endif
}

void Connection::decInFlight()
{
	// Clamp at 0 defensively (should never underflow).
#if defined(__GNUC__)
	int v = __sync_sub_and_fetch(&inFlightServices, 1);
	if (v < 0)
		__sync_lock_test_and_set(&inFlightServices, 0);
#else
	if (inFlightServices > 0)
		--inFlightServices;
	else
		inFlightServices = 0;
#endif
}

void Connection::incPendingSends()
{
#if defined(__GNUC__)
	__sync_add_and_fetch(&pendingSends, 1);
#else
	++pendingSends;
#endif
}

void Connection::decPendingSends()
{
#if defined(__GNUC__)
	int v = __sync_sub_and_fetch(&pendingSends, 1);
	if (v < 0)
		__sync_lock_test_and_set(&pendingSends, 0);
#else
	if (pendingSends > 0)
		--pendingSends;
	else
		pendingSends = 0;
#endif
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

	int64_t key = atoll(newKey.c_str());
	return key;
}
