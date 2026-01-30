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
#include "socket.h"
#include "../Database/GString.h"
#include "../Database/Serializable.h"
#include "connection.h"
#include "crypt.h"
#include "main.h"
#include "service.h"

using namespace GNet;

const shmea::GString Sockets::ANYADDR = "0.0.0.0";
const shmea::GString Sockets::LOCALHOST = "127.0.0.1";

void Sockets::initSockets()
{
	//logger->setPrintLevel(shmea::GLogger::LOG_INFO);
	PORT = "45019";
	inMutex = new GMutex();
	outMutex = new GMutex();
	udpfd = SHMEA_INVALID_SOCKET;
}

Sockets::Sockets() : logger(shmea::GPointer<shmea::GLogger>(new shmea::GLogger()))
{
	initSockets();
}

Sockets::Sockets(const GServer* serverInstance) : logger(serverInstance->logger)
{
	initSockets();
}

Sockets::~Sockets()
{
	delete inMutex;
	inMutex = NULL;
	delete outMutex;
	outMutex = NULL;
	
}

const shmea::GString Sockets::getPort()
{
	return PORT;
}

void Sockets::setPort(const shmea::GString newPort)
{
	PORT = newPort;
}

sock_t Sockets::openClientConnection(const shmea::GString& serverIP,
                                     const shmea::GString& serverPort)
{
    SHMEA_WINSOCK_INIT(); // no-op on Linux, required on Windows (if you use that helper)

    addrinfo hints{};
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* result = nullptr;
    int status = ::getaddrinfo(serverIP.c_str(), serverPort.c_str(), &hints, &result);
    if (status != 0 || !result)
    {
        logger->error("SOCKS", "Get client addr info fail");
        return SHMEA_INVALID_SOCKET;
    }

    sock_t sockfd = SHMEA_INVALID_SOCKET;

    // if you later want to iterate rp, use rp. For now, just use result.
    sockfd = ::socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sockfd == SHMEA_INVALID_SOCKET)
    {
        logger->error("SOCKS", "Could not open client socket");
        ::freeaddrinfo(result);
        return SHMEA_INVALID_SOCKET;
    }

    int bufVal = 0;
    ::setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, SOCKOPT_CAST &bufVal, sizeof(bufVal));
    ::setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, SOCKOPT_CAST &bufVal, sizeof(bufVal));

    status = ::connect(sockfd, result->ai_addr, (SHMEA_SOCKLEN)result->ai_addrlen);
    ::freeaddrinfo(result);

    if (status != 0)
    {
        logger->error("SOCKS", "Could not connect to the server!");
        CLOSESOCK(sockfd);
        return SHMEA_INVALID_SOCKET;
    }

    // debug buffer sizes (optional)
    int bufferSize = 0;
    SHMEA_SOCKLEN bufferSizeLen = sizeof(bufferSize);

    if (::getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, SOCKOPT_GCAST &bufferSize, &bufferSizeLen) == 0)
        std::cout << "Client Receive buffer size: " << bufferSize << " bytes\n";
    else
        std::cout << "getsockopt(SO_RCVBUF) failed: " << SHMEA_LAST_SOCK_ERR() << "\n";

    bufferSizeLen = sizeof(bufferSize);
    if (::getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, SOCKOPT_GCAST &bufferSize, &bufferSizeLen) == 0)
        std::cout << "Client Send buffer size: " << bufferSize << " bytes\n";
    else
        std::cout << "getsockopt(SO_SNDBUF) failed: " << SHMEA_LAST_SOCK_ERR() << "\n";

    return sockfd;
}


sock_t Sockets::openServerConnection()
{
    SHMEA_WINSOCK_INIT(); // no-op on Linux

    sock_t sockfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd == SHMEA_INVALID_SOCKET)
    {
        logger->error("SOCKS", "Could not open server socket");
        return SHMEA_INVALID_SOCKET;
    }

    int optval = 1;
    ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, SOCKOPT_CAST &optval, sizeof(optval));
    ::setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, SOCKOPT_CAST &optval, sizeof(optval));
    ::setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, SOCKOPT_CAST &optval, sizeof(optval));

#if defined(SO_REUSEPORT)
    ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, SOCKOPT_CAST &optval, sizeof(optval));
#endif

    addrinfo hints{};
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    // optional (more canonical for server bind):
    // hints.ai_flags = AI_PASSIVE;

    addrinfo* result = nullptr;
    int status = ::getaddrinfo(ANYADDR.c_str(), PORT.c_str(), &hints, &result);
    if (status != 0 || !result)
    {
        logger->error("SOCKS", "Get server addr info fail");
        CLOSESOCK(sockfd);
        return SHMEA_INVALID_SOCKET;
    }

    status = ::bind(sockfd, result->ai_addr, (SHMEA_SOCKLEN)result->ai_addrlen);
    ::freeaddrinfo(result);

    if (status != 0)
    {
        logger->error("SOCKS", "Could not bind server!");
        CLOSESOCK(sockfd);
        return SHMEA_INVALID_SOCKET;
    }

    if (::listen(sockfd, 64) != 0)
    {
        logger->error("SOCKS", "listen() failed");
        CLOSESOCK(sockfd);
        return SHMEA_INVALID_SOCKET;
    }

    // Optional debug buffer size checks:
    int bufferSize = 0;
    SHMEA_SOCKLEN bufferSizeLen = sizeof(bufferSize);

    if (::getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, SOCKOPT_GCAST &bufferSize, &bufferSizeLen) == 0)
        std::cout << "Server Receive buffer size: " << bufferSize << " bytes\n";
    else
        std::cout << "getsockopt(SO_RCVBUF) failed: " << SHMEA_LAST_SOCK_ERR() << "\n";

    bufferSizeLen = sizeof(bufferSize);
    if (::getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, SOCKOPT_GCAST &bufferSize, &bufferSizeLen) == 0)
        std::cout << "Server Send buffer size: " << bufferSize << " bytes\n";
    else
        std::cout << "getsockopt(SO_SNDBUF) failed: " << SHMEA_LAST_SOCK_ERR() << "\n";

    return sockfd;
}


sock_t Sockets::openUDPServerSocket()
{
    SHMEA_WINSOCK_INIT();

    sock_t s = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == SHMEA_INVALID_SOCKET)
    {
        logger->error("SOCKS", "Could not open UDP socket");
        return SHMEA_INVALID_SOCKET;
    }

    int optval = 1;
    ::setsockopt(s, SOL_SOCKET, SO_REUSEADDR, SOCKOPT_CAST &optval, sizeof(optval));

#if defined(SO_REUSEPORT)
    ::setsockopt(s, SOL_SOCKET, SO_REUSEPORT, SOCKOPT_CAST &optval, sizeof(optval));
#endif

    addrinfo hints{};
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    addrinfo* result = nullptr;
    int status = ::getaddrinfo(ANYADDR.c_str(), PORT.c_str(), &hints, &result);
    if (status != 0 || !result)
    {
        logger->error("SOCKS", "Get UDP addr info fail");
        CLOSESOCK(s);
        return SHMEA_INVALID_SOCKET;
    }

    status = ::bind(s, result->ai_addr, (SHMEA_SOCKLEN)result->ai_addrlen);
    ::freeaddrinfo(result);

    if (status != 0)
    {
        logger->error("SOCKS", "Could not bind UDP socket");
        CLOSESOCK(s);
        return SHMEA_INVALID_SOCKET;
    }

    udpfd = s;
    return s;
}


void Sockets::readConnection(Connection* origin, const sock_t& sockfd, std::vector<shmea::ServiceData*>& srvcList)
{
	readConnectionHelper(origin, sockfd, srvcList);

	// remove the empty data lists
	/*for (unsigned int i = 0; i < srvcList.size(); ++i)
	{
		if (srvcList[i].size() <= 0)
		{
			srvcList.erase(srvcList.begin() + i);
			--i;
		}
	}*/
}

void Sockets::readConnectionHelper(Connection* origin, const sock_t& sockfd, std::vector<shmea::ServiceData*>& srvcList)
{
	if (origin == NULL)
		return;

	// Accumulate raw network-order data and parse as many complete frames as available
	shmea::GString raw = origin->overflow;
	origin->overflow = "";

	char buffer[4096];
	int bytesRead = SHMEA_RECV(sockfd, buffer, (int)sizeof(buffer));
	if (bytesRead == -1)
	{
		logger->error("SOCKS", "[READER] Error: 3");
		return;
	}
	if (bytesRead == 0)
		return; // peer closed or no data

	raw += shmea::GString(buffer, bytesRead);

	while (raw.length() >= sizeof(unsigned int) * 2)
	{
		unsigned int blockSize = ntohl(*(unsigned int*)(&raw[0]));
		if (blockSize < sizeof(unsigned int) * 2)
		{
			logger->error("SOCKS", shmea::GString::format("Invalid blockSize: %u", blockSize));
			return;
		}

		if (raw.length() < blockSize)
			break; // wait for more bytes

		unsigned int padding = ntohl(*(unsigned int*)(&raw[4]));
		unsigned int payloadNetLen = blockSize - (sizeof(unsigned int) * 2);
		shmea::GString payloadHost = "";
		for (unsigned int i = 8; i < 8 + payloadNetLen; i += sizeof(unsigned int))
		{
			unsigned int cIntBlock = ntohl(*(unsigned int*)(&raw[i]));
			payloadHost += shmea::GString((const char*)&cIntBlock, sizeof(unsigned int));
		}
		// Remove padding from the tail of the host-order payload
		if (padding > 0 && payloadHost.length() >= padding)
			payloadHost = payloadHost.substr(0, payloadHost.length() - padding);

		int64_t key = origin->getKey();
		if (origin->isEncrypted())
		{
			Crypt crypt; // TODO: MOVE THIS TO SERIALIZE
			crypt.decrypt((int64_t*)payloadHost.c_str(), key, payloadHost.length() / 8);
			if (crypt.error)
			{
				logger->error("CRYPT", shmea::GString::format("Readside Error: %d", crypt.error));
				return;
			}
			shmea::ServiceData* cData = new shmea::ServiceData(origin, "");
			shmea::GString cStr = crypt.dText;
			shmea::Serializable::Deserialize(cData, cStr);
			cData->setTimesent(crypt.getTimesent());
			srvcList.push_back(cData);
		}
		else
		{
			shmea::ServiceData* cData = new shmea::ServiceData(origin, "");
			shmea::Serializable::Deserialize(cData, payloadHost);
			srvcList.push_back(cData);
		}

		// Consume this frame from the raw buffer
		raw = raw.substr(blockSize);
	}

	// Save any leftover raw bytes for the next read
	origin->overflow = raw;
}

int Sockets::writeConnection(const Connection* cConnection, const sock_t& sockfd, shmea::ServiceData* cData)
{
	int64_t key = DEFAULT_KEY;

	if (cConnection != NULL)
		key = cConnection->getKey();

	// Add the version and message type to the front of every packet
	// writeList.insertString(0, version.getString());

	// Convert to packet format
	cData->assignServiceNum();

	shmea::GString rawData = shmea::Serializable::Serialize(cData);
	if (rawData.length() == 0)
	{
		logger->error("SOCKS", "[WRITER] Error: 0");
		return -1;
	}

	// Encrypt
	Crypt crypt;//TODO: MOVE THIS TO SERIALIZE
	if(cConnection->isEncrypted())
	{
	    crypt.encrypt(rawData.c_str(), key, rawData.length());

	    if (crypt.error)
	    {
	    	logger->error("CRYPT", shmea::GString::format("Writeside Error: %d", crypt.error));
	    	return -1;
	    }

	    /*printf("WRITE-dText[%d]: %s\n", crypt.sizeClaimed, crypt.dText);
	    printf("Key Write: %lld\n", key);
	    for(int i=0;i<crypt.sizeClaimed;++i)
	    	printf("eTextWrite[%d]: 0x%016llX\n", i, crypt.eText.substr(i*sizeof(int64_t), sizeof(int64_t));*/

	    /*printf("WRITE-dText[%d]: %s\n", crypt.sizeClaimed, crypt.dText);
	    printf("Key Write: %lld\n", key);
	    if(crypt.dText[crypt.sizeClaimed-1] == 0)
	    for(unsigned int rCounter=0;rCounter<crypt.sizeClaimed;++rCounter)
	    {
	    	printf("WRITE[%u]: 0x%02X:%c\n", rCounter, crypt.dText[rCounter], crypt.dText[rCounter]);
	    	if(crypt.dText[rCounter] == 0x7C)
	    		printf("-------------------------------\n");
	    }*/
	}

	shmea::GString newStr = "";
	if(cConnection->isEncrypted())
	    newStr = crypt.eText;
	else
	    newStr = rawData;
	unsigned int newBlockSize = newStr.length() + 8; // plus size and padding
    // Pad up to the next 4-byte boundary
    unsigned int newPadding = (4 - (newStr.length() % 4)) % 4; // 0..3 padding bytes
	newBlockSize += newPadding;
	shmea::GString sizeInt = shmea::GString((const char*)&newBlockSize, sizeof(unsigned int));
	shmea::GString paddingInt = shmea::GString((const char*)&newPadding, sizeof(unsigned int));

	logger->debug("SOCKS", shmea::GString::format("newBlockSize: %u", newBlockSize));
	unsigned int zeros = 0;
	newStr += shmea::GString((const char*)&zeros, newPadding);
	newStr = sizeInt + paddingInt + newStr;

	shmea::GString writeStr = "";
	for (unsigned int i = 0; i < newStr.length(); i+=sizeof(unsigned int)) // TODO support uneven writes using newPadding
	{
		unsigned int writeVal = htonl(*((unsigned int*)(newStr.substr(i, sizeof(unsigned int)).c_str())));
		writeStr += shmea::GString((const char*)&writeVal, sizeof(unsigned int));
	}

	unsigned int writeLen = 0;
	if (cConnection && cConnection->getProtocol() == Connection::PROTO_UDP)
	{
		// Send full datagram via UDP
		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(atoi(cConnection->getPort().c_str()));
		inet_pton(AF_INET, cConnection->getIP().c_str(), &addr.sin_addr);
		int sent = sendto(sockfd, writeStr.c_str(), writeStr.length(), 0, (struct sockaddr*)&addr, sizeof(addr));
		if (sent >= 0)
			writeLen = (unsigned int)sent;
	}
	else
	{
		for (unsigned int i = 0; i < writeStr.length(); i+=1024)
		{
		    if(writeStr.length()-i < 1024)
		        writeLen += SHMEA_SEND(sockfd, writeStr.c_str()+i, writeStr.length()-i);
		    else
		        writeLen += SHMEA_SEND(sockfd, writeStr.c_str()+i, 1024);
		}
	}

	if ((writeLen != writeStr.length()) || (newBlockSize != newStr.length()))
	    logger->error("SOCKS", shmea::GString::format("Write Error: %u/%u : %u/%u", writeLen, writeStr.length(), newBlockSize, newStr.length()));
	else
	    logger->verbose("SOCKS", shmea::GString::format("Write Success: %u/%u : %u/%u", writeLen, writeStr.length(), newBlockSize, newStr.length()));

	// write to the sock
	return writeLen;
}

void Sockets::closeConnection(const sock_t& sockfd)
{
	CLOSESOCK(sockfd);

}

/*!
 * @brief read lists from connection
 * @details read pending lists from a connection
 * @param origin the connection Connection
 * @return false if the Connection should log out (unable to read), false otherwise
 */
bool Sockets::readLists(Connection* origin)
{
    if (!origin)
	{
        return false;
	}
    // Capture overflow size to detect partial progress
    unsigned int overflowBefore = origin->overflow.length();

    std::vector<shmea::ServiceData*> srvcList;
    readConnection(origin, origin->sockfd, srvcList);

    // If no complete services arrived but overflow grew, we made progress (partial frame)
    if (srvcList.size() == 0 && origin->overflow.length() > overflowBefore)
	{
        return true;
	}
    if (srvcList.size() == 0)
	{
        return false;
	}
	// loop through the srvcList
	for (unsigned int i = 0; i < srvcList.size(); ++i)
	{
		// get the data from the data list
		shmea::ServiceData* cData = srvcList[i];

		// Check the version
		/*shmea::GString clientVersion = cData.getString(0);
		cData.remove(0);*/
		/*if (version != clientVersion)
			return false;*/

		inMutex->lock();

		int64_t serviceNum = cData->getServiceNum();
		std::map<int64_t, shmea::ServiceData*>::iterator itr = inboundLists.find(serviceNum);
		if(itr == inboundLists.end())
			inboundLists.insert(std::pair<int64_t, shmea::ServiceData*>(serviceNum, cData));
		else
		{
			logger->warning("SOCKS", shmea::GString::format("ServiceNum colision: %ld !!!!", serviceNum));
			inboundLists[serviceNum] = cData;
		}

		inMutex->unlock();
	}

	return true;
}

bool Sockets::readUDPDatagram(GServer* serverInstance)
{
	if (udpfd == SHMEA_INVALID_SOCKET)
		return false;

	char buffer[4096]; 
	struct sockaddr_in from;
	SHMEA_SOCKLEN fromlen = sizeof(from);
	int bytes = recvfrom(udpfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&from, &fromlen);
	if (bytes <= 0)
		return false;

	char fromIP[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &from.sin_addr, fromIP, INET_ADDRSTRLEN);
	shmea::GString ip = fromIP;
	shmea::GString port = shmea::GString::format("%d", ntohs(from.sin_port));

	Connection* tempConn = new Connection(udpfd, Connection::CLIENT_TYPE, ip, port);
	tempConn->setProtocol(Connection::PROTO_UDP);
	tempConn->setCloseOnFinish(false);
	if (serverInstance && !serverInstance->isEncryptedByDefault())
		tempConn->disableEncryption();

	// UDP datagram carries already network-order encapsulated frame per existing protocol
	// Reuse readConnectionHelper by simulating a stream read from datagram contents
	// Build srvcList and enqueue to inboundLists
	std::vector<shmea::ServiceData*> srvcList;
	// emulate origin overflow-less path by temporarily injecting buffer
	// Minimal wrapper: deserialize directly (unencrypted path supported)
	// We need to mirror TCP framing: [size(4)][padding(4)][payload(net32 chunks)]
	if ((unsigned int)bytes < sizeof(unsigned int)*2)
	{
		delete tempConn;
		return false;
	}
	// Create a temporary string with the datagram
	shmea::GString chunk(buffer, bytes);

	// Since readConnectionHelper expects to read() from fd, we replicate core branch here
	unsigned int blockSize = ntohl(*(unsigned int*)(&chunk[0]));
	unsigned int padding = ntohl(*(unsigned int*)(&chunk[4]));
	if (blockSize != (unsigned int)bytes)
	{
		logger->warning("SOCKS", shmea::GString::format("UDP size mismatch: %u != %u", blockSize, bytes));
	}
	shmea::GString payload = "";
	for (unsigned int i = 8; i + sizeof(unsigned int) <= (unsigned int)bytes; i += sizeof(unsigned int))
	{
		unsigned int v = ntohl(*(unsigned int*)(&chunk[i]));
		payload += shmea::GString((const char*)&v, sizeof(unsigned int));
	}
	if (padding > 0 && payload.length() >= padding)
		payload = payload.substr(0, payload.length() - padding);

	Crypt crypt;
	if (tempConn->isEncrypted())
	{
		int64_t key = tempConn->getKey();
		crypt.decrypt((int64_t*)payload.c_str(), key, payload.length()/8);
		if (crypt.error)
		{
			logger->error("CRYPT", shmea::GString::format("UDP Readside Error: %d", crypt.error));
			delete tempConn;
			return false;
		}
		shmea::ServiceData* cData = new shmea::ServiceData(tempConn, "");
		shmea::GString cStr = crypt.dText;
		shmea::Serializable::Deserialize(cData, cStr);
		cData->setTimesent(crypt.getTimesent());
		srvcList.push_back(cData);
	}
	else
	{
		shmea::ServiceData* cData = new shmea::ServiceData(tempConn, "");
		shmea::Serializable::Deserialize(cData, payload);
		srvcList.push_back(cData);
	}

	for (unsigned int i = 0; i < srvcList.size(); ++i)
	{
		inMutex->lock();
		int64_t serviceNum = srvcList[i]->getServiceNum();
		std::map<int64_t, shmea::ServiceData*>::iterator itr = inboundLists.find(serviceNum);
		if (itr == inboundLists.end())
			inboundLists.insert(std::pair<int64_t, shmea::ServiceData*>(serviceNum, srvcList[i]));
		else
			inboundLists[serviceNum] = srvcList[i];
		inMutex->unlock();
	}

	return true;
}

/*!
 * @brief process lists
 * @details create new services from the lists in the "inbound" map
 * @param cConnection the connection Connection
 */
void Sockets::processLists(GServer* serverInstance)
{
	while (!inboundLists.empty())
	{
		inMutex->lock();
		shmea::ServiceData* nextSD = (*inboundLists.begin()).second;
		inboundLists.erase(inboundLists.begin());
		inMutex->unlock();
		GNet::Service::ExecuteService(serverInstance, nextSD, nextSD->getConnection());
	}
}

/*!
 * @brief write lists
 * @details write lists in the "outbound" map to the socket
 * @param cConnection the connection Connection
 */
void Sockets::writeLists(GServer* serverInstance)
{
	if (!serverInstance)
		return;

	if (!anyOutboundLists())
		return;

	outMutex->lock();
	shmea::ServiceData* nextOutbound = (*outboundLists.begin()).second;
	outboundLists.erase(outboundLists.begin());
	serverInstance->send(nextOutbound);
	outMutex->unlock();
}

/*!
 * @brief any inbound data?
 * @details are there any inbound data?
 * @return true if any inbound unprocessed service lists, false otherwise
 */
bool Sockets::anyInboundLists()
{
	return !inboundLists.empty();
}

/*!
 * @brief any outbound data?
 * @details are there any outbound data?
 * @return true if any outbound unprocessed service lists, false otherwise
 */
bool Sockets::anyOutboundLists()
{
	return !outboundLists.empty();
}
void Sockets::addResponseList(GServer* serverInstance, Connection* cConnection, shmea::ServiceData* cData)
{
	if (!cConnection)
		return;

	if (!cData)
		return;

	outMutex->lock();

	int64_t serviceNum = cData->getServiceNum();
	std::map<int64_t, shmea::ServiceData*>::iterator itr = outboundLists.find(serviceNum);
	if(itr == outboundLists.end())
		outboundLists.insert(std::pair<int64_t, shmea::ServiceData*>(serviceNum, cData));
	else
	{
		logger->warning("SOCKS", shmea::GString::format("ServiceNum colision: %ld !!!!", serviceNum));
		outboundLists[serviceNum] = cData;
	}

	serverInstance->wakeWriter();
	outMutex->unlock();
}
