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
#include "socket.h"
#include "../Database/GString.h"
#include "../Database/Serializable.h"
#include "connection.h"
#include "crypt.h"
#include "main.h"
#include "service.h"
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <vector>

using namespace GNet;

const shmea::GString Sockets::ANYADDR = "0.0.0.0";
const shmea::GString Sockets::LOCALHOST = "127.0.0.1";

namespace {
static const uint32_t FRAME_HEADER_BYTES = 8;               // [blockSize(4)][padding(4)]
static const uint32_t MAX_FRAME_BYTES = 16 * 1024 * 1024;   // hard cap to avoid OOM/DoS

static void set_send_timeout(int fd, int seconds, shmea::GPointer<shmea::GLogger> logger, const char* where)
{
	if (fd < 0)
		return;
	struct timeval tv;
	tv.tv_sec = seconds;
	tv.tv_usec = 0;
	if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0)
	{
		if (logger)
			logger->warning("SOCKS", shmea::GString::format("setsockopt(SO_SNDTIMEO) failed (%s)", where ? where : "?"));
	}
}

static bool read_u32_be(const shmea::GString& s, size_t offset, uint32_t& out)
{
	if (offset + sizeof(uint32_t) > (size_t)s.length())
		return false;
	uint32_t tmp = 0;
	memcpy(&tmp, s.c_str() + offset, sizeof(uint32_t));
	out = ntohl(tmp);
	return true;
}

static void append_u32_be(shmea::GString& out, uint32_t v)
{
	uint32_t be = htonl(v);
	out += shmea::GString((const char*)&be, sizeof(be));
}

static int write_all(int fd, const char* buf, size_t len)
{
	size_t written = 0;
	while (written < len)
	{
		// Avoid SIGPIPE on peer disconnect (Linux).
		ssize_t rc = ::send(fd, buf + written, len - written, MSG_NOSIGNAL);
		if (rc > 0)
		{
			written += (size_t)rc;
			continue;
		}

		if (rc == 0)
			break;

		if (errno == EINTR)
			continue;

		// For this protocol layer, treat non-blocking "try again" as an error; caller can retry later.
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return -1;

		return -1;
	}
	// Treat partial writes as an error for this framing protocol.
	if (written != len)
		return -1;
	return (int)written;
}

static bool decrypt_into_service_data(Sockets* self, Connection* origin, const shmea::GString& payload, shmea::GPointer<shmea::ServiceData>& outSD)
{
	if (!origin)
		return false;

	if ((payload.length() % 8) != 0)
	{
		if (self && self->logger)
			self->logger->error("CRYPT", "Encrypted payload not multiple of 8 bytes");
		return false;
	}

	std::vector<int64_t> blocks((size_t)payload.length() / 8);
	if (!blocks.empty())
		memcpy(blocks.data(), payload.c_str(), (size_t)payload.length());

	Crypt crypt; // TODO: MOVE THIS TO SERIALIZE
	crypt.decrypt(blocks.data(), origin->getKey(), (unsigned int)blocks.size());
	if (crypt.error)
	{
		if (self && self->logger)
			self->logger->error("CRYPT", shmea::GString::format("Readside Error: %d", crypt.error));
		return false;
	}

	shmea::ServiceData* cData = new shmea::ServiceData(origin, "");
	shmea::GString cStr = crypt.dText;
	shmea::Serializable::Deserialize(cData, cStr);
	cData->setTimesent(crypt.getTimesent());
	outSD = shmea::GPointer<shmea::ServiceData>(cData);
	return true;
}

static shmea::GString obs_conn_kv(const Connection* c)
{
	if (!c)
		return "conn=null";
	const char* proto = (c->getProtocol() == Connection::PROTO_UDP) ? "udp" : "tcp";
	return shmea::GString::format("peer=%s:%s name=%s proto=%s enc=%d fd=%d",
		c->getIP().c_str(), c->getPort().c_str(), c->getName().c_str(), proto, (int)c->isEncrypted(), c->sockfd);
}

static shmea::GString obs_sd_kv(const shmea::ServiceData* sd)
{
	if (!sd)
		return "sid=? svc=? resp=? cmd=? sk=? type=?";
	return shmea::GString::format("sid=%s svc=%ld resp=%ld cmd=%s sk=%s type=%d argc=%u",
		sd->getSID().c_str(),
		(long)sd->getServiceNum(),
		(long)sd->getResponseServiceNum(),
		sd->getCommand().c_str(),
		sd->getServiceKey().c_str(),
		sd->getType(),
		(unsigned int)sd->getArgList().size());
}
} // namespace

void Sockets::maybeLogMetrics(const char* where)
{
	if (!logger)
		return;
	static time_t last = 0;
	time_t now = time(NULL);
	if (now == (time_t)-1)
		return;
	if (last != 0 && (now - last) < 10)
		return;
	last = now;

	unsigned int inSz = 0;
	unsigned int outSz = 0;
	pthread_mutex_lock(inMutex);
	inSz = (unsigned int)inboundLists.size();
	pthread_mutex_unlock(inMutex);
	pthread_mutex_lock(outMutex);
	outSz = (unsigned int)outboundLists.size();
	pthread_mutex_unlock(outMutex);

	logger->info("OBS", shmea::GString::format("event=metrics where=%s inbound_q=%u outbound_q=%u",
		where ? where : "?", inSz, outSz));
}

void Sockets::initSockets()
{
	//logger->setPrintLevel(shmea::GLogger::LOG_INFO);
	PORT = "45019";
	inMutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	outMutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	udpfd = -1;
	// Basic backpressure caps to prevent unbounded memory growth under load/abuse.
	// These are global across all connections (queue keys are per-connection).
	inboundQueueMax = 8192;
	outboundQueueMax = 8192;

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
	struct addrinfo* result = NULL;
	struct addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	int status = getaddrinfo(serverIP.c_str(), serverPort.c_str(), &hints, &result);
	if (status != 0 || !result)
	{
		logger->error("SOCKS", shmea::GString::format("Get client addr info fail: %s", gai_strerror(status)));
		return -1;
	}

	int sockfd = -1;
	for (struct addrinfo* rp = result; rp != NULL; rp = rp->ai_next)
	{
		sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sockfd < 0)
		{
			continue;
		}

		/*int optval=1;
		int sockopts=SO_REUSEADDR;
		#ifdef (SO_REUSEPORT)
			sockopts|=SO_REUSEPORT;
		#endif
		setsockopt(sockfd, SOL_SOCKET, sockopts, &optval, sizeof(optval));*/

		int optval = 1;
		if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) < 0)
			logger->warning("SOCKS", "setsockopt(SO_KEEPALIVE) failed (client)");
		if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)) < 0)
			logger->warning("SOCKS", "setsockopt(TCP_NODELAY) failed (client)");
		set_send_timeout(sockfd, 3, logger, "client");

		// Use a finite connect timeout so shutdown can't hang indefinitely.
		int flags = fcntl(sockfd, F_GETFL, 0);
		if (flags >= 0)
			(void)fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

		status = connect(sockfd, rp->ai_addr, rp->ai_addrlen);
		if (status == 0)
		{
			// connected immediately
			if (flags >= 0)
				(void)fcntl(sockfd, F_SETFL, flags); // restore blocking
			break;
		}

		if (status < 0 && errno == EINPROGRESS)
		{
			fd_set wfds;
			FD_ZERO(&wfds);
			FD_SET(sockfd, &wfds);
			struct timeval tv;
			tv.tv_sec = 3;
			tv.tv_usec = 0;

			int sel;
			do {
				sel = select(sockfd + 1, NULL, &wfds, NULL, &tv);
			} while (sel < 0 && errno == EINTR);

			if (sel > 0 && FD_ISSET(sockfd, &wfds))
			{
				int soerr = 0;
				socklen_t slen = sizeof(soerr);
				if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &soerr, &slen) == 0 && soerr == 0)
				{
					if (flags >= 0)
						(void)fcntl(sockfd, F_SETFL, flags); // restore blocking
					break; // success
				}
			}
		}

		close(sockfd);
		sockfd = -1;
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
	// IMPORTANT: setsockopt() takes a single optname at a time.
	// Some options also require a different protocol level (e.g. TCP_NODELAY).
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
		logger->warning("SOCKS", "setsockopt(SO_REUSEADDR) failed");
#ifdef SO_REUSEPORT
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) < 0)
		logger->warning("SOCKS", "setsockopt(SO_REUSEPORT) failed");
#endif
	if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) < 0)
		logger->warning("SOCKS", "setsockopt(SO_KEEPALIVE) failed");
	if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)) < 0)
		logger->warning("SOCKS", "setsockopt(TCP_NODELAY) failed");

	struct addrinfo* result;
	struct addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	int status = getaddrinfo(ANYADDR.c_str(), PORT.c_str(), &hints, &result);
	if (status != 0 || !result)
	{
		logger->error("SOCKS", shmea::GString::format("Get server addr info fail: %s", gai_strerror(status)));
		close(sockfd);
		return -1;
	}

	bool bound = false;
	for (struct addrinfo* rp = result; rp != NULL; rp = rp->ai_next)
	{
		status = bind(sockfd, rp->ai_addr, rp->ai_addrlen);
		if (status == 0)
		{
			bound = true;
			break;
		}
	}

	freeaddrinfo(result);

	if (!bound)
	{
		logger->error("SOCKS", "Could not bind server!");
		close(sockfd);
		return -1;
	}

	if (listen(sockfd, 64) < 0)
	{
		logger->error("SOCKS", "listen() failed");
		close(sockfd);
		return -1;
	}

	return sockfd;
}

int Sockets::openUDPServerSocket()
{
	int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (s < 0)
	{
		logger->error("SOCKS", "Could not open UDP socket");
		return -1;
	}

	int optval = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
		logger->warning("SOCKS", "setsockopt(SO_REUSEADDR) failed (UDP)");
#ifdef SO_REUSEPORT
	if (setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) < 0)
		logger->warning("SOCKS", "setsockopt(SO_REUSEPORT) failed (UDP)");
#endif

	struct addrinfo* result;
	struct addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;

	int status = getaddrinfo(ANYADDR.c_str(), PORT.c_str(), &hints, &result);
	if (status != 0 || !result)
	{
		logger->error("SOCKS", shmea::GString::format("Get UDP addr info fail: %s", gai_strerror(status)));
		close(s);
		return -1;
	}

	bool bound = false;
	for (struct addrinfo* rp = result; rp != NULL; rp = rp->ai_next)
	{
		status = bind(s, rp->ai_addr, rp->ai_addrlen);
		if (status == 0)
		{
			bound = true;
			break;
		}
	}
	freeaddrinfo(result);
	if (!bound)
	{
		logger->error("SOCKS", "Could not bind UDP socket");
		close(s);
		return -1;
	}

	udpfd = s;
	return s;
}

void Sockets::readConnection(Connection* origin, const int& sockfd, std::vector<shmea::GPointer<shmea::ServiceData> >& srvcList)
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

int Sockets::readConnectionHelper(Connection* origin, const int& sockfd, std::vector<shmea::GPointer<shmea::ServiceData> >& srvcList)
{
	if (origin == NULL)
		return -2;

	// Accumulate raw network-order data and parse as many complete frames as available
	shmea::GString raw = origin->overflow;
	origin->overflow = "";
	const size_t rawBeforeRead = (size_t)raw.length();

	char buffer[4096];
	bool readAny = false;
	bool peerClosed = false;
	for (;;)
	{
		// Never block inside the protocol parser; drain what is available.
		ssize_t bytesRead = ::recv(sockfd, buffer, sizeof(buffer), MSG_DONTWAIT);
		if (bytesRead > 0)
		{
			readAny = true;
			raw += shmea::GString(buffer, (int)bytesRead);
			continue;
		}
		if (bytesRead == 0)
		{
			// peer closed (EOF). Keep any already-buffered bytes; caller will decide whether to logout.
			peerClosed = true;
			break;
		}

		// bytesRead < 0
		if (errno == EINTR)
			continue;
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			break;

		// Preserve existing overflow on permanent errors
		origin->overflow = raw;
		logger->error("SOCKS", "[READER] recv() failed");
		return -2;
	}

	bool parsedAny = false;
	while ((uint32_t)raw.length() >= FRAME_HEADER_BYTES)
	{
		uint32_t blockSize = 0;
		uint32_t padding = 0;
		if (!read_u32_be(raw, 0, blockSize) || !read_u32_be(raw, 4, padding))
			break;

		if (blockSize < FRAME_HEADER_BYTES)
		{
			logger->error("SOCKS", shmea::GString::format("Invalid blockSize: %u", blockSize));
			origin->overflow = "";
			return -2;
		}

		if (blockSize > MAX_FRAME_BYTES)
		{
			logger->error("SOCKS", shmea::GString::format("Frame too large: %u", blockSize));
			origin->overflow = "";
			return -2;
		}

		if (raw.length() < blockSize)
			break; // wait for more bytes

		uint32_t payloadWithPaddingLen = blockSize - FRAME_HEADER_BYTES;
		if (padding > payloadWithPaddingLen)
		{
			logger->error("SOCKS", shmea::GString::format("Invalid padding: %u (frame=%u)", padding, blockSize));
			origin->overflow = "";
			return -2;
		}

		shmea::GString payload = raw.substr(FRAME_HEADER_BYTES, payloadWithPaddingLen);
		if (padding > 0 && (uint32_t)payload.length() >= padding)
			payload = payload.substr(0, payload.length() - padding);

		if (origin->isEncrypted())
		{
			shmea::GPointer<shmea::ServiceData> cData;
			if (!decrypt_into_service_data(this, origin, payload, cData))
			{
				origin->overflow = "";
				return -2;
			}
			srvcList.push_back(cData);
		}
		else
		{
			shmea::GPointer<shmea::ServiceData> cData(new shmea::ServiceData(origin, ""));
			shmea::Serializable::Deserialize(cData.get(), payload);
			srvcList.push_back(cData);
		}
		parsedAny = true;

		// Consume this frame from the raw buffer
		raw = raw.substr(blockSize);
	}

	// Save any leftover raw bytes for the next read
	origin->overflow = raw;

	// Status semantics:
	// - If we parsed at least one frame, caller should keep the connection and process work.
	// - If peer closed (EOF) and we did not parse any frames, caller should logout connection.
	// - If no new bytes were read and no frames parsed, treat as "no data right now".
	if (parsedAny)
		return 1;
	if (peerClosed)
		return -1;
	if (!readAny && (size_t)origin->overflow.length() == rawBeforeRead)
		return 0;
	// Progress (read bytes and/or buffered state changed), but not enough to parse a frame yet.
	return 1;
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

	if (logger)
		logger->debug("OBS", "event=send_prepare " + obs_sd_kv(cData) + " " + obs_conn_kv(cConnection));

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

	shmea::GString payload = "";
	if(cConnection->isEncrypted())
	    payload = crypt.eText;
	else
	    payload = rawData;

	// Payload bytes are sent as-is. Only the header fields are network-order.
	uint32_t padding = 0;
	uint32_t blockSize = FRAME_HEADER_BYTES + (uint32_t)payload.length() + padding;
	if (blockSize > MAX_FRAME_BYTES)
	{
		logger->error("SOCKS", shmea::GString::format("Refusing to send frame too large: %u", blockSize));
		return -1;
	}

	shmea::GString frame = "";
	append_u32_be(frame, blockSize);
	append_u32_be(frame, padding);
	frame += payload;
	// (padding is currently always 0)

	unsigned int writeLen = 0;
	if (cConnection && cConnection->getProtocol() == Connection::PROTO_UDP)
	{
		// Send full datagram via UDP
		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(atoi(cConnection->getPort().c_str()));
		inet_pton(AF_INET, cConnection->getIP().c_str(), &addr.sin_addr);
		int sent = sendto(sockfd, frame.c_str(), frame.length(), 0, (struct sockaddr*)&addr, sizeof(addr));
		if (sent >= 0)
		{
			if ((unsigned int)sent == frame.length())
				writeLen = (unsigned int)sent;
			else
				writeLen = 0;
		}
	}
	else
	{
		int rc = write_all(sockfd, frame.c_str(), (size_t)frame.length());
		if (rc >= 0)
			writeLen = (unsigned int)rc;
	}

	if ((writeLen != frame.length()) || (blockSize != (uint32_t)frame.length()))
	    logger->error("SOCKS", shmea::GString::format("Write Error: %u/%u : %u/%u", writeLen, frame.length(), blockSize, frame.length()));
	else
	    logger->verbose("SOCKS", shmea::GString::format("Write Success: %u/%u : %u/%u", writeLen, frame.length(), blockSize, frame.length()));

	// write to the sock
	if (writeLen != frame.length())
	{
		if (logger)
			logger->error("OBS", "event=send_fail " + obs_sd_kv(cData) + " " + obs_conn_kv(cConnection) +
				shmea::GString::format(" bytes=%u frame=%u", writeLen, (unsigned int)frame.length()));
		return -1;
	}

	if (logger)
		logger->debug("OBS", "event=send_ok " + obs_sd_kv(cData) + " " + obs_conn_kv(cConnection) +
			shmea::GString::format(" bytes=%u", writeLen));
	return (int)writeLen;
}

void Sockets::closeConnection(const int& sockfd)
{
	close(sockfd);
}

void Sockets::closeSockets()
{
	// Only close the shared UDP socket here. TCP sockets are owned by Connection::finish().
	if (udpfd >= 0)
	{
		close(udpfd);
		udpfd = -1;
	}
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
        return false;

    std::vector<shmea::GPointer<shmea::ServiceData> > srvcList;
    int rc = readConnectionHelper(origin, origin->sockfd, srvcList);

	// Explicitly interpret read status:
	// - -2: fatal protocol/I/O error => logout
	// - -1: peer closed (EOF) => logout (unless we parsed frames, in which case rc==1)
	// -  0: no data right now => keep connection
	// -  1: progress and/or frames parsed => keep connection
	if (rc < 0)
		return false;
	if (srvcList.size() == 0)
		return true;

	// loop through the srvcList
	for (unsigned int i = 0; i < srvcList.size(); ++i)
	{
		// get the data from the data list
		shmea::GPointer<shmea::ServiceData> cData = srvcList[i];
		if (!cData)
			continue;

		if (logger)
			logger->debug("OBS", "event=recv_service " + obs_sd_kv(cData.get()) + " " + obs_conn_kv(origin));

		// Check the version
		/*shmea::GString clientVersion = cData.getString(0);
		cData.remove(0);*/
		/*if (version != clientVersion)
			return false;*/

		pthread_mutex_lock(inMutex);
		if (inboundLists.size() >= (size_t)inboundQueueMax)
		{
			size_t qsz = inboundLists.size();
			pthread_mutex_unlock(inMutex);
			if (logger)
				logger->warning(
					"OBS",
					shmea::GString::format("event=backpressure_drop where=inbound q=%u qmax=%u ",
						(unsigned int)qsz, (unsigned int)inboundQueueMax) +
						obs_sd_kv(cData.get()) + " " + obs_conn_kv(origin));
			// Tell caller to logout this connection (it is producing faster than we can consume).
			return false;
		}

		int64_t serviceNum = cData->getServiceNum();
		Sockets::QueueKey key(origin, serviceNum);
		std::map<Sockets::QueueKey, shmea::GPointer<shmea::ServiceData>, Sockets::QueueKeyLess>::iterator itr = inboundLists.find(key);
		if(itr == inboundLists.end())
			inboundLists.insert(std::pair<Sockets::QueueKey, shmea::GPointer<shmea::ServiceData> >(key, cData));
		else
		{
			// Never overwrite an existing queued message. Overwriting silently corrupts request/response
			// ordering and can cross-wire responses. Treat as a protocol/ID-generation error and drop.
			const shmea::GPointer<shmea::ServiceData>& existing = itr->second;
			if (logger)
			{
				logger->error(
					"SOCKS",
					"Queue collision: dropping inbound (per-conn) " +
						shmea::GString::format("serviceNum=%ld ", (long)serviceNum) +
						"new[" + obs_sd_kv(cData.get()) + "] existing[" + obs_sd_kv(existing.get()) + "] " +
						obs_conn_kv(origin));
			}
		}

		pthread_mutex_unlock(inMutex);
	}
	maybeLogMetrics("readLists");
	return true;
}

bool Sockets::readUDPDatagram(GServer* serverInstance)
{
	if (udpfd < 0)
		return false;

	char buffer[4096];
	struct sockaddr_in from;
	socklen_t fromlen = sizeof(from);
	int bytes = recvfrom(udpfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&from, &fromlen);
	if (bytes <= 0)
		return false;

	char fromIP[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &from.sin_addr, fromIP, INET_ADDRSTRLEN);
	shmea::GString ip = fromIP;
	shmea::GString port = shmea::GString::format("%d", ntohs(from.sin_port));

	// Reuse a server-managed per-peer UDP Connection keyed by source ip:port.
	// Creating a new Connection per datagram causes needless churn and breaks any attempt at per-peer state.
	if (!serverInstance)
		return false;

	Connection* tempConn = NULL;
	const shmea::GString peerKey = ip + ":" + port;

	pthread_mutex_lock(serverInstance->clientMutex);
	{
		std::map<shmea::GString, std::vector<int> >::iterator itr = serverInstance->clientCLookUp.find(peerKey);
		if (itr != serverInstance->clientCLookUp.end())
		{
			std::vector<int>& idxs = itr->second;
			for (unsigned int i = 0; i < idxs.size(); ++i)
			{
				Connection* c = serverInstance->clientC[idxs[i]];
				if (!c)
					continue;
				if (c->getProtocol() != Connection::PROTO_UDP)
					continue;
				if (c->getIP() == ip && c->getPort() == port && c->sockfd == udpfd)
				{
					tempConn = c;
					break;
				}
			}
		}

		if (!tempConn)
		{
			tempConn = new Connection(udpfd, Connection::CLIENT_TYPE, ip, port);
			tempConn->setProtocol(Connection::PROTO_UDP);
			tempConn->setCloseOnFinish(false);
			if (!serverInstance->isEncryptedByDefault())
				tempConn->disableEncryption();

			if (serverInstance->clientCLookUp.find(peerKey) == serverInstance->clientCLookUp.end())
				serverInstance->clientCLookUp.insert(std::pair<shmea::GString, std::vector<int> >(peerKey, std::vector<int>()));

			serverInstance->clientC.push_back(tempConn);
			serverInstance->clientCLookUp[peerKey].push_back(serverInstance->clientC.size() - 1);
		}

		// Track last-seen time for safe idle pruning.
		if (tempConn)
			tempConn->noteSeen();
	}
	pthread_mutex_unlock(serverInstance->clientMutex);

	// UDP datagram carries already network-order encapsulated frame per existing protocol
	std::vector<shmea::GPointer<shmea::ServiceData> > srvcList;
	// Mirror TCP framing: [blockSize(4)][padding(4)][payload bytes...]
	if ((unsigned int)bytes < FRAME_HEADER_BYTES)
	{
		return false;
	}
	// Create a temporary string with the datagram
	shmea::GString chunk(buffer, bytes);

	if (tempConn->isEncrypted())
	{
		uint32_t blockSize = 0;
		uint32_t padding = 0;
		if (!read_u32_be(chunk, 0, blockSize) || !read_u32_be(chunk, 4, padding))
		{
			return false;
		}
		if (blockSize > (uint32_t)bytes)
		{
			logger->warning("SOCKS", shmea::GString::format("UDP truncated frame: %u > %u", blockSize, bytes));
			return false;
		}
		if (blockSize < FRAME_HEADER_BYTES || blockSize > MAX_FRAME_BYTES)
		{
			return false;
		}
		uint32_t payloadWithPaddingLen = blockSize - FRAME_HEADER_BYTES;
		if (padding > payloadWithPaddingLen)
		{
			return false;
		}
		shmea::GString payload = chunk.substr(FRAME_HEADER_BYTES, payloadWithPaddingLen);
		if (padding > 0 && (uint32_t)payload.length() >= padding)
			payload = payload.substr(0, payload.length() - padding);

		shmea::GPointer<shmea::ServiceData> cData;
		if (!decrypt_into_service_data(this, tempConn, payload, cData))
			return false;
		srvcList.push_back(cData);
	}
	else
	{
		uint32_t blockSize = 0;
		uint32_t padding = 0;
		if (!read_u32_be(chunk, 0, blockSize) || !read_u32_be(chunk, 4, padding))
		{
			return false;
		}
		if (blockSize > (uint32_t)bytes)
		{
			logger->warning("SOCKS", shmea::GString::format("UDP truncated frame: %u > %u", blockSize, bytes));
			return false;
		}
		if (blockSize < FRAME_HEADER_BYTES || blockSize > MAX_FRAME_BYTES)
		{
			return false;
		}
		uint32_t payloadWithPaddingLen = blockSize - FRAME_HEADER_BYTES;
		if (padding > payloadWithPaddingLen)
		{
			return false;
		}
		shmea::GString payload = chunk.substr(FRAME_HEADER_BYTES, payloadWithPaddingLen);
		if (padding > 0 && (uint32_t)payload.length() >= padding)
			payload = payload.substr(0, payload.length() - padding);

		shmea::GPointer<shmea::ServiceData> cData(new shmea::ServiceData(tempConn, ""));
		shmea::Serializable::Deserialize(cData.get(), payload);
		srvcList.push_back(cData);
	}

	for (unsigned int i = 0; i < srvcList.size(); ++i)
	{
		if (logger && srvcList[i])
			logger->debug("OBS", "event=recv_service " + obs_sd_kv(srvcList[i].get()) + " " + obs_conn_kv(tempConn));
		pthread_mutex_lock(inMutex);
		if (inboundLists.size() >= (size_t)inboundQueueMax)
		{
			size_t qsz = inboundLists.size();
			pthread_mutex_unlock(inMutex);
			if (logger)
				logger->warning(
					"OBS",
					shmea::GString::format("event=backpressure_drop where=inbound_udp q=%u qmax=%u ",
						(unsigned int)qsz, (unsigned int)inboundQueueMax) +
						obs_sd_kv(srvcList[i].get()) + " " + obs_conn_kv(tempConn));
			// Drop this datagram's contents (UDP has no reliable backpressure).
			continue;
		}
		int64_t serviceNum = srvcList[i]->getServiceNum();
		Sockets::QueueKey key(tempConn, serviceNum);
		std::map<Sockets::QueueKey, shmea::GPointer<shmea::ServiceData>, Sockets::QueueKeyLess>::iterator itr = inboundLists.find(key);
		if (itr == inboundLists.end())
			inboundLists.insert(std::pair<Sockets::QueueKey, shmea::GPointer<shmea::ServiceData> >(key, srvcList[i]));
		else
		{
			// Do not overwrite an existing message on collision; drop the new one.
			const shmea::GPointer<shmea::ServiceData>& existing = itr->second;
			if (logger)
			{
				logger->error(
					"SOCKS",
					"Queue collision: dropping inbound(udp) (per-conn) " +
						shmea::GString::format("serviceNum=%ld ", (long)serviceNum) +
						"new[" + obs_sd_kv(srvcList[i].get()) + "] existing[" + obs_sd_kv(existing.get()) + "] " +
						obs_conn_kv(tempConn));
			}
		}
		pthread_mutex_unlock(inMutex);
	}

	maybeLogMetrics("readUDPDatagram");
	return true;
}

/*!
 * @brief process lists
 * @details create new services from the lists in the "inbound" map
 * @param cConnection the connection Connection
 */
void Sockets::processLists(GServer* serverInstance)
{
	for (;;)
	{
		pthread_mutex_lock(inMutex);
		if (inboundLists.empty())
		{
			pthread_mutex_unlock(inMutex);
			break;
		}
		shmea::GPointer<shmea::ServiceData> nextSD = (*inboundLists.begin()).second;
		inboundLists.erase(inboundLists.begin());
		pthread_mutex_unlock(inMutex);
		if (logger && nextSD)
			logger->debug("OBS", "event=dispatch_service " + obs_sd_kv(nextSD.get()) + " " + obs_conn_kv(nextSD->getConnection()));
		if (serverInstance)
			serverInstance->enqueueService(nextSD, nextSD ? nextSD->getConnection() : NULL);
	}
	maybeLogMetrics("processLists");
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

	pthread_mutex_lock(outMutex);
	if (outboundLists.empty())
	{
		pthread_mutex_unlock(outMutex);
		return;
	}
	shmea::GPointer<shmea::ServiceData> nextOutbound = (*outboundLists.begin()).second;
	outboundLists.erase(outboundLists.begin());
	pthread_mutex_unlock(outMutex);

	if (logger && nextOutbound)
		logger->debug("OBS", "event=dequeue_send " + obs_sd_kv(nextOutbound.get()) + " " + obs_conn_kv(nextOutbound->getConnection()));
	serverInstance->send(nextOutbound);
	maybeLogMetrics("writeLists");
}

/*!
 * @brief any inbound data?
 * @details are there any inbound data?
 * @return true if any inbound unprocessed service lists, false otherwise
 */
bool Sockets::anyInboundLists()
{
	pthread_mutex_lock(inMutex);
	bool any = !inboundLists.empty();
	pthread_mutex_unlock(inMutex);
	return any;
}

/*!
 * @brief any outbound data?
 * @details are there any outbound data?
 * @return true if any outbound unprocessed service lists, false otherwise
 */
bool Sockets::anyOutboundLists()
{
	pthread_mutex_lock(outMutex);
	bool any = !outboundLists.empty();
	pthread_mutex_unlock(outMutex);
	return any;
}
void Sockets::addResponseList(GServer* serverInstance, Connection* cConnection, shmea::GPointer<shmea::ServiceData> cData)
{
	if (!cConnection)
		return;

	if (!cData)
		return;

	pthread_mutex_lock(outMutex);
	if (outboundLists.size() >= (size_t)outboundQueueMax)
	{
		size_t qsz = outboundLists.size();
		pthread_mutex_unlock(outMutex);
		if (logger)
			logger->warning(
				"OBS",
				shmea::GString::format("event=backpressure_drop where=outbound q=%u qmax=%u ",
					(unsigned int)qsz, (unsigned int)outboundQueueMax) +
					obs_sd_kv(cData.get()) + " " + obs_conn_kv(cConnection));
		// Request logout from the server thread; this may be called from a worker thread.
		if (serverInstance)
			serverInstance->requestLogout(cConnection);
		return;
	}

	int64_t serviceNum = cData->getServiceNum();
	Sockets::QueueKey key(cConnection, serviceNum);
	std::map<Sockets::QueueKey, shmea::GPointer<shmea::ServiceData>, Sockets::QueueKeyLess>::iterator itr = outboundLists.find(key);
	if(itr == outboundLists.end())
	{
		// Track a pending send so UDP peers can be pruned safely only when idle.
		cConnection->incPendingSends();
		outboundLists.insert(std::pair<Sockets::QueueKey, shmea::GPointer<shmea::ServiceData> >(key, cData));
	}
	else
	{
		// Do not overwrite an existing queued outbound message. Drop and log loudly.
		const shmea::GPointer<shmea::ServiceData>& existing = itr->second;
		if (logger)
		{
			logger->error(
				"SOCKS",
				"Queue collision: dropping outbound (per-conn) " +
					shmea::GString::format("serviceNum=%ld ", (long)serviceNum) +
					"new[" + obs_sd_kv(cData.get()) + "] existing[" + obs_sd_kv(existing.get()) + "] " +
					obs_conn_kv(cConnection));
		}
	}
	pthread_mutex_unlock(outMutex);

	if (logger)
		logger->debug("OBS", "event=enqueue_send " + obs_sd_kv(cData.get()) + " " + obs_conn_kv(cConnection));

	// Wake the writer thread after releasing the queue lock.
	// (wakeWriter() now takes `writersMutex` internally.)
	if (serverInstance)
		serverInstance->wakeWriter();
	maybeLogMetrics("addResponseList");
}

void Sockets::purgeConnection(Connection* c)
{
	if (!c)
		return;

	// Remove queued inbound messages for this connection.
	pthread_mutex_lock(inMutex);
	for (std::map<Sockets::QueueKey, shmea::GPointer<shmea::ServiceData>, Sockets::QueueKeyLess>::iterator it = inboundLists.begin();
		 it != inboundLists.end();)
	{
		if (it->first.conn == c)
			inboundLists.erase(it++);
		else
			++it;
	}
	pthread_mutex_unlock(inMutex);

	// Remove queued outbound messages and release pending-send bookkeeping for each dropped message.
	pthread_mutex_lock(outMutex);
	for (std::map<Sockets::QueueKey, shmea::GPointer<shmea::ServiceData>, Sockets::QueueKeyLess>::iterator it = outboundLists.begin();
		 it != outboundLists.end();)
	{
		if (it->first.conn == c)
		{
			// This outbound message will never be sent; undo the pending-send refcount.
			c->decPendingSends();
			outboundLists.erase(it++);
		}
		else
		{
			++it;
		}
	}
	pthread_mutex_unlock(outMutex);

	if (logger)
		logger->info("OBS", "event=purge_queues " + obs_conn_kv(c));
}
