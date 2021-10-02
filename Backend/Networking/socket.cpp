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
	PORT = "45019";
	inMutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	outMutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));

	pthread_mutex_init(inMutex, NULL);
	pthread_mutex_init(outMutex, NULL);
}

Sockets::Sockets()
{
	initSockets();
}

Sockets::Sockets(const shmea::GString& newPORT)
{
	initSockets();
	PORT = newPORT;
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

int Sockets::openClientConnection(const shmea::GString& serverIP)
{
	struct addrinfo* result;
	struct addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	int status = getaddrinfo(serverIP.c_str(), PORT.c_str(), &hints, &result);
	if (status < 0)
	{
		printf("[SOCKS] Get client addr info fail\n");
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
			printf("[SOCKS] Could not open client socket\n");
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
			printf("[SOCKS] Could not connect to the server!\n");
			return -1; // continue;
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
		printf("[SOCKS] Could not open server socket\n");
		return -1;
	}

	int optval = 1;
	int sockopts = SO_REUSEADDR;
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
		printf("[SOCKS] Get server addr info fail\n");
		return -1;
	}

	status = bind(sockfd, result->ai_addr, result->ai_addrlen);
	if (status < 0)
	{
		printf("[SOCKS] Could not bind server!\n");
		return -1;
	}

	listen(sockfd, 5);
	freeaddrinfo(result);

	return sockfd;
}

shmea::GString Sockets::reader(const int& sockfd)
{
	char buffer[1025];
	bzero(buffer, 1025);
	unsigned int tSize = 0;
	char* eText = (char*)malloc(sizeof(char) * tSize);
	if (!eText)
	{
		printf("[READER] Error: 0\n");
		return "";
	}

	do
	{
		int bytesRead = read(sockfd, buffer, 1024);
		if (bytesRead > 0)
		{
			int oldTSize = tSize;
			tSize += bytesRead;
			char* pText = (char*)realloc(eText, sizeof(char) * tSize);

			if (pText)
			{
				eText = pText;
				memcpy(&eText[oldTSize], buffer, bytesRead);
			}
			else
			{
				printf("[READER] Error: 1\n");
				tSize = 0;
				return "";
			}
		}
		else
		{
			printf("[READER] Error: 2\n");
			free(eText);
			tSize = 0;
			return "";
		}

		// We only write int64_t but we do it int by int
	} while (((tSize % sizeof(int)) == 0) && ((tSize / sizeof(int)) % 2 == 1));

	// Make sure we are the correct big/little endian
	int64_t newEBlock = 0;
	int lCounter = 0;
	int64_t* newEText = (int64_t*)malloc(sizeof(char) * tSize);
	if(!newEText)
	{
		printf("[READER] Error: 3\n");
		tSize = 0;
		return "";
	}
	
	for (unsigned int i = 0; i < tSize / sizeof(int); ++i)
	{
		int64_t cIntBlock = ntohl(((unsigned int*)eText)[i]);
		if (i % 2 == 0)
			newEBlock = cIntBlock;
		else
		{
			newEBlock += cIntBlock * 0x100000000ll;
			newEText[lCounter] = newEBlock;
			newEBlock = 0;
			++lCounter;
		}
	}

	free(eText);
	return shmea::GString((const char*)newEText, tSize);
}

void Sockets::readConnection(Connection* origin, const int& sockfd, std::vector<const shmea::ServiceData*>& srvcList)
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

void Sockets::readConnectionHelper(Connection* origin, const int& sockfd, std::vector<const shmea::ServiceData*>& srvcList)
{
	if (origin == NULL)
		return;

	int balance = 0;
	shmea::GString cOverflow = origin->overflow;
	int64_t key = origin->getKey();

	do
	{
		// get the things to read
		shmea::GString eText = "";
		if(balance != 1)
		{
			eText = reader(sockfd);
		}

		// overflow+eText
		if (balance != 0)
		{
			eText = cOverflow + eText;
		}

		//if (eText.length() == 0)
		if (eText.length() == 0)
			return;

		/*printf("Key Read: %lld\n", key);
		for(int i=0;i<eText.length()/8;++i)
			printf("eTextRead[%d]: 0x%016llX\n", i, *(int64_t*)eText.substr(i*sizeof(int64_t), sizeof(int64_t)).c_str());*/

		// decrypt
		Crypt crypt;//TODO: MOVE THIS TO SERIALIZE
		crypt.decrypt((int64_t*)eText.c_str(), key, eText.length()/8);

		if (crypt.error)
		{
			printf("[CRYPT] Error: %d\n", crypt.error);
			return;
		}

		// starving
		if (crypt.sizeCurrent < crypt.sizeClaimed)
		{
			balance = -1;

			cOverflow = eText;
		}
		else if (crypt.sizeCurrent == crypt.sizeClaimed)
		{
			/*printf("READ-dText[%d]: %s\n", crypt.sizeClaimed, crypt.dText);
			if(crypt.dText[crypt.sizeClaimed-1] == 0)
			for(unsigned int rCounter=0;rCounter<crypt.sizeClaimed;++rCounter)
			{
				printf("READ[%u]: 0x%02X:%c\n", rCounter, crypt.dText[rCounter], crypt.dText[rCounter]);
				if(crypt.dText[rCounter] == 0x7C)
					printf("-------------------------------\n");
			}*/

			// set the text from the crypt object & add it to the data
			shmea::ServiceData* cData = new shmea::ServiceData(origin);
			//printf("eTextLen-PRE-SER: %u\n", eText.length()/8);
			shmea::GString cStr = crypt.dText;
			shmea::Serializable::Deserialize(cData, cStr);
			srvcList.push_back(cData); // minus the key

			if (eText.length()/8 == crypt.sizeClaimed)
				balance = 0;
			else if (eText.length()/8 > crypt.sizeClaimed)
			{
				balance = 1;

				unsigned int cOverflowLen = (eText.length()/8) - crypt.sizeClaimed;
				cOverflow = eText.substr(
					crypt.sizeClaimed*sizeof(int64_t), cOverflowLen*sizeof(int64_t));
			}
		}
	} while (balance != 0);

	origin->overflow = cOverflow;
}

int Sockets::writeConnection(const Connection* cConnection, const int& sockfd,
							 const shmea::ServiceData* cData)
{
	int64_t key = DEFAULT_KEY;
	//printf("==============================================================\n");

	if (cConnection != NULL)
		key = cConnection->getKey();

	// Add the version and message type to the front of every packet
	// writeList.insertString(0, version.getString());

	// Convert to packet format
	shmea::GString rawData = shmea::Serializable::Serialize(cData);
	if (rawData.length() == 0)
	{
		printf("[WRITER] Error: 0\n");
		return -1;
	}

	// Encrypt
	Crypt crypt;//TODO: MOVE THIS TO SERIALIZE
	crypt.encrypt(rawData.c_str(), key, rawData.length());

	if (crypt.error)
	{
		printf("[CRYPT] Error: %d\n", crypt.error);
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

	shmea::GString writeStr = "";
	for (unsigned int i = 0; i < crypt.sizeClaimed * 2; ++i)
	{
		unsigned int writeVal = // This will be nice with GVector
			htonl(*((unsigned int*)(crypt.eText.substr(i*sizeof(unsigned int), sizeof(unsigned int)).c_str())));
		writeStr += shmea::GString((const char*)&writeVal, sizeof(unsigned int));
	}

	unsigned int writeLen = write(sockfd, writeStr.c_str(), writeStr.length());
	if (writeLen != sizeof(int64_t) * crypt.sizeClaimed)
		printf("[SOCKS] Write error: %u/%lld\n", writeLen, sizeof(int64_t) * crypt.sizeClaimed);

	//printf("==============================================================\n");

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
	std::vector<const shmea::ServiceData*> srvcList;
	readConnection(origin, origin->sockfd, srvcList);

	if (srvcList.size() > 0)
	{
		// loop through the srvcList
		for (unsigned int i = 0; i < srvcList.size(); ++i)
		{
			// get the data from the data list
			const shmea::ServiceData* cData = srvcList.front();
			srvcList.erase(srvcList.begin());

			// Check the version
			/*shmea::GString clientVersion = cData.getString(0);
			cData.remove(0);*/
			/*if (version != clientVersion)
				return false;*/

			pthread_mutex_lock(inMutex);
			inboundLists.push(cData);
			pthread_mutex_unlock(inMutex);
		}
		return true;
	}

	return false;
}

/*!
 * @brief process lists
 * @details create new services from the lists in the "inbound" queue
 * @param cConnection the connection Connection
 */
void Sockets::processLists(GServer* serverInstance, Connection* cConnection)
{
	while (!inboundLists.empty())
	{
		pthread_mutex_lock(inMutex);
		const shmea::ServiceData* nextSD = inboundLists.front();
		inboundLists.pop();
		pthread_mutex_unlock(inMutex);
		GNet::Service::ExecuteService(serverInstance, nextSD, cConnection);
	}
}

/*!
 * @brief write lists
 * @details write lists in the "outbound" queue to the socket
 * @param cConnection the connection Connection
 */
void Sockets::writeLists(GServer* serverInstance)
{
	if (!serverInstance)
		return;

	if (!anyOutboundLists())
		return;

	pthread_mutex_lock(outMutex);
	std::pair<Connection*, const shmea::ServiceData*> nextOutbound = outboundLists.front();
	outboundLists.pop();
	serverInstance->send(nextOutbound.second);
	pthread_mutex_unlock(outMutex);
	/*Connection* cConnection = nextOutbound.first;
	const shmea::ServiceData* nextCommand = nextOutbound.second;
	int bytesWritten =
		writeConnection(cConnection, cConnection->sockfd, nextCommand);

	return !(bytesWritten < 0);*/
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
void Sockets::addResponseList(GServer* serverInstance, Connection* cConnection,
							  const shmea::ServiceData* cData)
{
	if (!cConnection)
		return;

	pthread_mutex_lock(outMutex);
	outboundLists.push(std::make_pair(cConnection, cData));
	serverInstance->wakeWriter();
	pthread_mutex_unlock(outMutex);
}
