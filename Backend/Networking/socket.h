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
#ifndef _GSOCKET
#define _GSOCKET

#include "platform.h"
#include "../Database/GString.h"
#include "../Database/GLogger.h"
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>         // Needed for HANDLE, CreateMutex, etc.
    #ifdef _MSC_VER
	#pragma comment(lib, "ws2_32.lib")
	#endif
#else
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <sys/socket.h>
    #include <sys/types.h>
	#include <sys/select.h>
    #include <unistd.h>
	#include <netdb.h>
#endif
#include "GThread.h"
#include "GMutex.h"
#include "GCondition.h"
#include <iostream>
#include <map>
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <utility>
#include <vector>

namespace shmea {
class ServiceData;
};

namespace GNet {
class GServer;
class Connection;

class Sockets
{
private:
	static const int64_t DEFAULT_KEY = 420l;
	static const shmea::GString ANYADDR;

	shmea::GString PORT;
	GMutex* inMutex;
	GMutex* outMutex;
	std::map<int64_t, shmea::ServiceData*> inboundLists; // Vector of sds instead? Make the key advanced to take hostnames, usernames,  etc; too
	std::map<int64_t, shmea::ServiceData*> outboundLists; // Vector of sds instead?
	int udpfd;

	void initSockets();

	// ServiceData* emptyResponseList();

public:
	static const shmea::GString LOCALHOST;

	shmea::GPointer<shmea::GLogger> logger;

	Sockets();
	Sockets(const GServer*);
	~Sockets();

	// functions
	void initSockets(const shmea::GString&);
	void closeSockets();
	const shmea::GString getPort();
	void setPort(shmea::GString);
	int openServerConnection();
	int openClientConnection(const shmea::GString&, const shmea::GString&);
	int openUDPServerSocket();
	int getUDPSocketFD() const { return udpfd; }
	void readConnection(Connection*, const int&, std::vector<shmea::ServiceData*>&);
	void readConnectionHelper(Connection*, const int&, std::vector<shmea::ServiceData*>&);
	int writeConnection(const Connection*, const int&, shmea::ServiceData*);
	void closeConnection(const int&);

	bool anyInboundLists();
	bool anyOutboundLists();

	bool readLists(Connection*);
	bool readUDPDatagram(GServer*);
	void processLists(GServer*);
	void writeLists(GServer*);
	void addResponseList(GServer*, Connection*, shmea::ServiceData*);
};
};

#endif
