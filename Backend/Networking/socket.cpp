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
#include "../Database/GList.h"
#include "../Database/Serializable.h"
#include "connection.h"
#include "crypt.h"
#include "main.h"
#include "service.h"

using namespace GNet;

const std::string Sockets::ANYADDR = "0.0.0.0";
const std::string Sockets::LOCALHOST = "127.0.0.1";

void Sockets::initSockets()
{
	PORT = "45019";
	overflow = NULL;
	overflowLen = 0;
	inMutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	outMutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));

	pthread_mutex_init(inMutex, NULL);
	pthread_mutex_init(outMutex, NULL);
}

Sockets::Sockets()
{
	initSockets();
}

Sockets::Sockets(const std::string& newPORT)
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

const std::string Sockets::getPort()
{
	return PORT;
}

int Sockets::openClientConnection(const std::string& serverIP)
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
	std::string clientIP = fromIP;

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

int64_t* Sockets::reader(const int& sockfd, unsigned int& tSize)
{
	char buffer[1025];
	bzero(buffer, 1025);
	tSize = 0;
	char* eText = (char*)malloc(sizeof(char) * tSize);
	if (!eText)
		return NULL;

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
				return NULL;
		}
		else
		{
			free(eText);
			return NULL;
		}

		// We only write int64_t but we do it int by int
	} while (((tSize % sizeof(int)) == 0) && ((tSize / sizeof(int)) % 2 == 1));

	int64_t newEBlock = 0;
	int lCounter = 0;
	int64_t* newEText = (int64_t*)malloc(sizeof(char) * tSize);
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

	return newEText;
}

void Sockets::readConnection(Connection* cConnection, const int& sockfd, const std::string& cIP,
							 std::vector<shmea::GList>& itemList)
{
	readConnectionHelper(cConnection, sockfd, cIP, itemList);

	// remove the empty data lists
	for (unsigned int i = 0; i < itemList.size(); ++i)
	{
		if (itemList[i].size() <= 0)
		{
			itemList.erase(itemList.begin() + i);
			--i;
		}
	}
}

void Sockets::readConnectionHelper(Connection* cConnection, const int& sockfd,
								   const std::string& cIP, std::vector<shmea::GList>& itemList)
{
	int balance = 0;
	int64_t* cOverflow = overflow;
	unsigned int cOverflowLen = overflowLen;
	int64_t key = DEFAULT_KEY;

	// we would rather use the Connection versions instead
	if (cConnection != NULL)
	{
		cOverflow = cConnection->overflow;
		cOverflowLen = cConnection->overflowLen;
		key = cConnection->getKey();
	}

	do
	{
		// get the things to read
		unsigned int eTextLen = 0;
		int64_t* eText = balance == 1 ? NULL : reader(sockfd, eTextLen);

		// error in reader, whatevz
		if (eText == NULL)
			eText = (int64_t*)malloc(sizeof(int64_t) * 0);
		else
			eTextLen /= 8;

		// overflow+eText
		if (balance != 0)
		{
			// prepare a temp array
			unsigned int eTextLen2 = cOverflowLen + eTextLen;
			int64_t* eText2 = (int64_t*)malloc(sizeof(int64_t) * eTextLen2);
			if (!eText2)
				return;

			// overflow+eText
			memcpy(eText2, cOverflow, sizeof(int64_t) * cOverflowLen);
			memcpy(&eText2[cOverflowLen], eText, sizeof(int64_t) * eTextLen);

			// move eText3 over to eText
			eText = (int64_t*)malloc(sizeof(int64_t) * eTextLen2);
			if (!eText)
				return;
			memcpy(eText, eText2, sizeof(int64_t) * eTextLen2);
			eTextLen = eTextLen2;

			if (eText2)
				free(eText2);
		}

		if (eTextLen == 0)
			return;

		/*printf("Key Read: %lld\n", key);
		for(int i=0;i<eTextLen;++i)
			printf("eTextRead[%d]: 0x%016llX\n", i, eText[i]);*/

		// decrypt
		Crypt* crypt = new Crypt();
		crypt->decrypt(eText, key, eTextLen);

		if (crypt->error)
		{
			printf("[CRYPT] Error: %d\n", crypt->error);
			if (crypt)
				delete crypt;
			return;
		}

		// starving
		if (crypt->linesRead < crypt->size)
		{
			balance = -1;

			// overflow
			if (cOverflow)
				free(cOverflow);

			cOverflowLen = crypt->linesRead;
			cOverflow = (int64_t*)malloc(sizeof(int64_t) * cOverflowLen);
			memcpy(cOverflow, eText, sizeof(int64_t) * cOverflowLen);
		}
		else if (crypt->linesRead == crypt->size)
		{
			// set the text from the crypt object & add it to the data
			itemList.push_back(shmea::Serializable::DeserializeHelper(
				crypt->dText, crypt->size - 1)); // minus the key

			if (eTextLen == crypt->size)
				balance = 0;
			else if (eTextLen > crypt->size)
			{
				balance = 1;

				// overflow
				if (cOverflow)
					free(cOverflow);
				cOverflowLen = eTextLen - crypt->size;
				cOverflow = (int64_t*)malloc(sizeof(int64_t) * cOverflowLen);
				memcpy(cOverflow, &eText[crypt->size], sizeof(int64_t) * cOverflowLen);
			}
		}

		// delete it after we are done
		if (crypt)
			delete crypt;

		// free the eText
		if (eText)
			free(eText);

	} while (balance != 0);

	if (cConnection != NULL)
	{
		cConnection->overflow = cOverflow;
		cConnection->overflowLen = cOverflowLen;
	}
}

int Sockets::writeConnection(const Connection* cConnection, const int& sockfd,
							 const shmea::GList& cList, int messageType)
{
	int64_t key = DEFAULT_KEY;

	if (cConnection != NULL)
		key = cConnection->getKey();

	// add the version and message type to the front of every packet
	shmea::GList writeList = cList;
	writeList.insertInt(0, messageType);
	// writeList.insertString(0, version.getString());

	// convert to packet format
	char* writeData = (char*)malloc(0);
	unsigned int writeDataSize = shmea::Serializable::Serialize(writeList, &writeData);
	if (writeDataSize <= 0)
		return -1;

	// encrypt
	Crypt* crypt = new Crypt();
	crypt->encrypt(writeData, key, writeDataSize);

	if (crypt->error)
	{
		printf("[CRYPT] Error: %d\n", crypt->error);
		if (crypt)
			delete crypt;
		return -1;
	}

	/*printf("Key Write: %lld\n", key);
	for(int i=0;i<crypt->size;++i)
		printf("eTextWrite[%d]: 0x%016llX\n", i, crypt->eText[i]);*/

	unsigned int writeLen = 0;
	for (unsigned int i = 0; i < crypt->size * 2; ++i)
	{
		unsigned int writeVal = htonl(*(((unsigned int*)(crypt->eText)) + i));
		writeLen += write(sockfd, &writeVal, sizeof(unsigned int));
	}

	if (writeLen != sizeof(int64_t) * crypt->size)
		printf("[SOCKS] Write error: %u/%llu\n", writeLen, sizeof(int64_t) * crypt->size);

	// delete it after we are done
	if (crypt)
		delete crypt;

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
 * @param cConnection the connection Connection
 * @return false if the Connection should log out (unable to read), false otherwise
 */
bool Sockets::readLists(Connection* cConnection)
{
	std::vector<shmea::GList> itemList;
	readConnection(cConnection, cConnection->sockfd, cConnection->getIP(), itemList);

	if (itemList.size() > 0)
	{
		// loop through the itemList
		for (unsigned int i = 0; i < itemList.size(); ++i)
		{
			// get the data from the data list
			shmea::GList cList = itemList.front();
			itemList.erase(itemList.begin());

			// Check the version
			/*std::string clientVersion = cList.getString(0);
			cList.remove(0);*/
			/*if (version != clientVersion)
				return false;*/
			// check the message type as well
			int messageType = cList.getInt(0);
			cList.remove(0);
			// do something with message type here!!
			// printf("[NET] message type: %d\n", messageType);
			/*if (messageType == GNet::RESPONSE_TYPE)
				outboundLists.push(cList);
			else*/
			pthread_mutex_lock(inMutex);
			inboundLists.push(cList);
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
		shmea::GList nextCommand = inboundLists.front();
		inboundLists.pop();
		pthread_mutex_unlock(inMutex);
		GNet::Service::ExecuteService(serverInstance, nextCommand, cConnection);
	}
}

/*!
 * @brief write lists
 * @details write lists in the "outbound" queue to the socket
 * @param cConnection the connection Connection
 */
void Sockets::writeLists(GServer* serverInstance)
{
	if (!anyOutboundLists())
		return;

	pthread_mutex_lock(outMutex);
	std::pair<Connection*, shmea::GList> nextOutbound = outboundLists.front();
	outboundLists.pop();
	serverInstance->NewService(nextOutbound.second, nextOutbound.first);
	pthread_mutex_unlock(outMutex);
	/*Connection* cConnection = nextOutbound.first;
	shmea::GList nextCommand = nextOutbound.second;
	int bytesWritten =
		writeConnection(cConnection, cConnection->sockfd, nextCommand,
	GNet::GServer::RESPONSE_TYPE);

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
							  const shmea::GList& cList)
{
	if (!cConnection)
		return;

	pthread_mutex_lock(outMutex);
	outboundLists.push(std::make_pair(cConnection, cList));
	serverInstance->wakeWriter();
	pthread_mutex_unlock(outMutex);
}
