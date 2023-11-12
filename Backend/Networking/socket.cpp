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
	inMutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	outMutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));

	pthread_mutex_init(inMutex, NULL);
	pthread_mutex_init(outMutex, NULL);
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
	pthread_mutex_destroy(inMutex);
	if (inMutex)
		free(inMutex);

	pthread_mutex_destroy(outMutex);
	if (outMutex)
		free(outMutex);
}

const shmea::GString Sockets::getPort()
{
	return PORT;
}

void Sockets::setPort(const shmea::GString newPort)
{
	PORT = newPort;
}

int Sockets::openClientConnection(const shmea::GString& serverIP, const shmea::GString& serverPort)
{
	struct addrinfo* result;
	struct addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	int status = getaddrinfo(serverIP.c_str(), PORT.c_str(), &hints, &result);
	if (status < 0)
	{
		logger->error("SOCKS", "Get client addr info fail");
		return -1;
	}

	// get the ip
	char fromIP[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &result->ai_addr->sa_data[2], fromIP, INET_ADDRSTRLEN);
	shmea::GString clientIP = fromIP;

	// get the ip
	int sockfd = -1;
	struct addrinfo* rp;
	// for(rp=result;rp!=NULL;rp=rp->ai_next)
	{
		sockfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (sockfd < 0)
		{
			logger->error("SOCKS", "Could not open client socket");
			return -1; // continue;
		}

		/*int optval=1;
		int sockopts=SO_REUSEADDR;
		#ifdef (SO_REUSEPORT)
			sockopts|=SO_REUSEPORT;
		#endif
		setsockopt(sockfd, SOL_SOCKET, sockopts, &optval, sizeof(optval));*/

		// Having no buffer will force the socket to wait to send until the previous transaction is done.
		// Ths knowledge cannot be found anywhere so please do not delete this comment
		int bufVal = 0;
		setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &bufVal, sizeof(bufVal));
		setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &bufVal, sizeof(bufVal));

		status = connect(sockfd, result->ai_addr, result->ai_addrlen);
		if (status < 0)
		{
			logger->error("SOCKS", "Could not connect to the server!");
			return -1; // continue;
		}

// Get the size of the receive buffer
    int bufferSize;
    socklen_t bufferSizeLen = sizeof(bufferSize);
    if (getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &bufferSize, &bufferSizeLen) == 0) {
        std::cout << "Client Receive buffer size: " << bufferSize << " bytes" << std::endl;
    } else {
        perror("getsockopt");
    }

// Get the size of the receive buffer
    bufferSizeLen = sizeof(bufferSize);
    if (getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &bufferSize, &bufferSizeLen) == 0) {
        std::cout << "Client SEND buffer size: " << bufferSize << " bytes" << std::endl;
    } else {
        perror("getsockopt");
    }

	}

	freeaddrinfo(result);

	return sockfd;
}

int Sockets::openServerConnection()
{
	int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd < 0)
	{
		logger->error("SOCKS", "Could not open server socket");
		return -1;
	}

	int optval = 1;
	int sockopts = SO_REUSEADDR | SO_KEEPALIVE | TCP_NODELAY;
#if (SO_REUSEPORT)
	sockopts |= SO_REUSEPORT;
#endif
	setsockopt(sockfd, SOL_SOCKET, sockopts, &optval, sizeof(optval));

	struct addrinfo* result;
	struct addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	int status = getaddrinfo(ANYADDR.c_str(), PORT.c_str(), &hints, &result);
	if (status < 0)
	{
		logger->error("SOCKS", "Get server addr info fail");
		return -1;
	}

// Get the size of the receive buffer
    int bufferSize = 0;
    socklen_t bufferSizeLen = sizeof(bufferSize);
    if (getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &bufferSize, &bufferSizeLen) == 0) {
        std::cout << "Server Receive buffer size: " << bufferSize << " bytes" << std::endl;
    } else {
        perror("getsockopt");
    }

    bufferSize = 0;
    bufferSizeLen = sizeof(bufferSize);
    bufferSizeLen = sizeof(bufferSize);
    if (getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &bufferSize, &bufferSizeLen) == 0) {
        std::cout << "Server Write buffer size: " << bufferSize << " bytes" << std::endl;
    } else {
        perror("getsockopt");
    }

	status = bind(sockfd, result->ai_addr, result->ai_addrlen);
	if (status < 0)
	{
		logger->error("SOCKS", "Could not bind server!");
		return -1;
	}

	listen(sockfd, 64);
	freeaddrinfo(result);

	return sockfd;
}

void Sockets::readConnection(Connection* origin, const int& sockfd, std::vector<shmea::ServiceData*>& srvcList)
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

void Sockets::readConnectionHelper(Connection* origin, const int& sockfd, std::vector<shmea::ServiceData*>& srvcList)
{
	if (origin == NULL)
		return;

	shmea::GString cOverflow = origin->overflow;
	int64_t key = origin->getKey();

	shmea::GString eText = "";
	unsigned int eTotal = 0; // in bytes
	unsigned int endPadding = 0; // in bytes

	// bytes read
	unsigned int eByteCounter = 0;

	// Incase we read an amount that is not divisible by 4
	unsigned int readOverflow = 0;
	unsigned int readOverflowLen = 0; // in bytes

	do
	{
		char buffer[1025];
		bzero(buffer, 1025);
		*buffer = readOverflow;
		unsigned int bytesLeft = eTotal-eByteCounter;
		if(bytesLeft == 0) bytesLeft = 1024;
		bytesLeft = bytesLeft > 1024 ? 1024-readOverflow : bytesLeft;
		unsigned int bytesRead = read(sockfd, &buffer[readOverflowLen], bytesLeft);
		bytesRead+=readOverflowLen;
		//if ((bytesRead == 0) || (bytesRead == -1))
		if (bytesRead == (unsigned int)-1)
		{
			logger->error("SOCKS", "[READER] Error: 3");
			return;
		}

		shmea::GString bufferStr = shmea::GString(buffer, bytesRead);
		if(cOverflow.length() > 0)
		{
		    logger->debug("SOCKS", shmea::GString::format("cOverflow.length(): %u", cOverflow.length()));
		    bytesRead += cOverflow.length();
		    bufferStr = cOverflow + bufferStr;
		    cOverflow = "";
		    origin->overflow = "";
		}

		// If we read nothing, then the other side probabled dced
		if(bytesRead == 0)
		    return;

		bool headerIteration = false;
		if(eTotal == 0)
		{
		    headerIteration = true;

		    // The total bytes to read
		    unsigned int newSize = ntohl(*(unsigned int*)(&bufferStr[0]));
		    eTotal = newSize;
		    eByteCounter += sizeof(unsigned int);
		    logger->debug("SOCKS", shmea::GString::format("eTotal: %u", eTotal));

		    // Padding at the end in bytes
		    unsigned int newPadding = ntohl(*(unsigned int*)(&bufferStr[4]));
		    endPadding = newPadding;
		    eByteCounter += sizeof(unsigned int);
		    logger->debug("SOCKS", shmea::GString::format("endPadding: %u", endPadding));
		}

		unsigned int headerOffset = 0;
		if(headerIteration)
		    headerOffset = 8;

		// We will deal with the overflow later
		readOverflowLen = bytesRead % sizeof(unsigned int);
		bytesRead -= readOverflowLen;

		// Convert the content from network byte order
		shmea::GString newStr = "";
		for(unsigned int i=headerOffset; i < bytesRead; i+=sizeof(unsigned int))
		{
		    unsigned int cIntBlock = ntohl(*(unsigned int*)(&bufferStr[i]));
		    newStr += shmea::GString((const char*)&cIntBlock, sizeof(unsigned int));
		    eByteCounter += sizeof(unsigned int);
		}

		if(readOverflowLen > 0)
		    readOverflow = *(unsigned int*)(&bufferStr[bytesRead]);

		eText += newStr;

		//logger->debug("SOCKS", shmea::GString::format("eByteCounter: %u/%u/%u", eByteCounter, eText.length(), eTotal));
	} while ((eByteCounter < eTotal) || (readOverflowLen > 0));

	// We read a part of the next request
	unsigned int extraSize = eByteCounter - eTotal;
	if(extraSize > 0)
	{
	    //logger->debug("SOCKS", shmea::GString::format("Extra Size: %u", extraSize));
	    origin->overflow = eText.substr(eTotal);

	    eText = eText.substr(0, eByteCounter-extraSize-sizeof(int)*2);
	    //logger->debug("SOCKS", shmea::GString::format("new-eTextLen: %u", eText.length()));
	}
	else
	    origin->overflow = "";

	// Decrypt
	Crypt crypt;//TODO: MOVE THIS TO SERIALIZE
	if(origin->isEncrypted())
	{
	    //crypt.decryptHeader(eText, key);
	    crypt.decrypt((int64_t*)eText.c_str(), key, eText.length() / 8);

	    if((eText.length()-crypt.sizeClaimed*sizeof(int64_t)) > 0)
	        logger->warning("SOCKS", shmea::GString::format("CryptOverrun: %u", eText.length()-crypt.sizeClaimed*sizeof(int64_t)));

	    if (crypt.error)
	    {
	        logger->error("CRYPT", shmea::GString::format("Readside Error: %d", crypt.error));
	        return;
	    }

	    //logger->debug("SOCKS", shmea::GString::format("crypt.sizeClaimed: %lu", crypt.sizeClaimed*sizeof(int64_t)));
	    if(crypt.sizeClaimed*sizeof(int64_t) != eTotal-(sizeof(int)*2))
	    {
	        logger->error("SOCKS", shmea::GString::format("RCV Misalignment: %lu != %lu", crypt.sizeClaimed*sizeof(int64_t), eTotal-(sizeof(int)*2)));
	        return;
	    }
	    else
	    {
	        logger->verbose("SOCKS", shmea::GString::format("RCV Success: %lu == %lu", (crypt.sizeClaimed*sizeof(int64_t))+(sizeof(int)*2), eTotal));
	    }

	    //if (crypt.sizeCurrent < crypt.sizeClaimed)
	    //else if (crypt.sizeCurrent == crypt.sizeClaimed)

	    // Recreate the ServiceData to run later
	    shmea::ServiceData* cData = new shmea::ServiceData(origin, "");
	    shmea::GString cStr = crypt.dText;
	    shmea::Serializable::Deserialize(cData, cStr);
	    cData->setTimesent(crypt.getTimesent());
	    srvcList.push_back(cData); // minus the key
	}
	else
	{
	    // Recreate the ServiceData to run later
	    shmea::ServiceData* cData = new shmea::ServiceData(origin, "");
	    shmea::Serializable::Deserialize(cData, eText);
	    srvcList.push_back(cData);
	}
}

int Sockets::writeConnection(const Connection* cConnection, const int& sockfd, shmea::ServiceData* cData)
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
	unsigned int newPadding = newStr.length() % 4; // end 0s for even writes
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
	for (unsigned int i = 0; i < writeStr.length(); i+=1024)
	{
	    if(writeStr.length()-i < 1024)
	        writeLen += write(sockfd, writeStr.c_str()+i, writeStr.length()-i);
	    else
	        writeLen += write(sockfd, writeStr.c_str()+i, 1024);
	}

	if ((writeLen != writeStr.length()) || (newBlockSize != newStr.length()))
	    logger->error("SOCKS", shmea::GString::format("Write Error: %u/%u : %u/%u", writeLen, writeStr.length(), newBlockSize, newStr.length()));
	else
	    logger->verbose("SOCKS", shmea::GString::format("Write Success: %u/%u : %u/%u", writeLen, writeStr.length(), newBlockSize, newStr.length()));

	// write to the sock
	return writeLen;
}

void Sockets::closeConnection(const int& sockfd)
{
	close(sockfd);
}

/*!
 * @brief read lists from connection
 * @details read pending lists from a connection
 * @param origin the connection Connection
 * @return false if the Connection should log out (unable to read), false otherwise
 */
bool Sockets::readLists(Connection* origin)
{
	std::vector<shmea::ServiceData*> srvcList;
	readConnection(origin, origin->sockfd, srvcList);

	if (srvcList.size() == 0)
		return false;

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

		pthread_mutex_lock(inMutex);

		int64_t serviceNum = cData->getServiceNum();
		std::map<int64_t, shmea::ServiceData*>::iterator itr = inboundLists.find(serviceNum);
		if(itr == inboundLists.end())
			inboundLists.insert(std::pair<int64_t, shmea::ServiceData*>(serviceNum, cData));
		else
		{
			logger->warning("SOCKS", shmea::GString::format("ServiceNum colision: %ld !!!!", serviceNum));
			inboundLists[serviceNum] = cData;
		}

		pthread_mutex_unlock(inMutex);
	}
	return true;
}

/*!
 * @brief process lists
 * @details create new services from the lists in the "inbound" map
 * @param cConnection the connection Connection
 */
void Sockets::processLists(GServer* serverInstance, Connection* cConnection)
{
	while (!inboundLists.empty())
	{
		pthread_mutex_lock(inMutex);
		shmea::ServiceData* nextSD = (*inboundLists.begin()).second;
		inboundLists.erase(inboundLists.begin());
		pthread_mutex_unlock(inMutex);
		GNet::Service::ExecuteService(serverInstance, nextSD, cConnection);
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

	pthread_mutex_lock(outMutex);
	shmea::ServiceData* nextOutbound = (*outboundLists.begin()).second;
	outboundLists.erase(outboundLists.begin());
	serverInstance->send(nextOutbound);
	pthread_mutex_unlock(outMutex);
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

	pthread_mutex_lock(outMutex);

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
	pthread_mutex_unlock(outMutex);
}
