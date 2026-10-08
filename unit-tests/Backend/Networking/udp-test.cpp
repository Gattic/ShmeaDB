// Confidential, unpublished property of Robert Carneiro

// The access and distribution of this material is limited solely to
// authorized personnel.  The use, disclosure, reproduction,
// modification, transfer, or transmittal of this work for any purpose
// in any form or by any means without the written permission of
// Robert Carneiro is strictly prohibited.
#include "udp-test.h"
#include "../../unit-test.h"
#include "../../../Backend/Database/GString.h"
#include "../../../Backend/Database/GList.h"
#include "../../../Backend/Database/Serializable.h"
#include "../../../Backend/Database/ServiceData.h"
#include "../../../Backend/Networking/main.h"
#include "../../../Backend/Networking/socket.h"

// Strengthened UDP loopback test:
// Start server locally, send a framed Handshake_Server over UDP, and assert we receive a framed Handshake_Client reply.

#include <errno.h>
#include <stdint.h>
#include <string>
#include "../../../Backend/Core/platform.h"

// Basic UDP loopback test: start server locally, send a Handshake_Server over UDP and expect Handshake_Client response

static void sleep_ms(int ms)
{
	g_sleep_ms(ms);
}

static uint32_t read_u32_be(const char* p)
{
	uint32_t tmp = 0;
	memcpy(&tmp, p, sizeof(uint32_t));
	return ntohl(tmp);
}

static shmea::GString u32_be(uint32_t v)
{
	uint32_t be = htonl(v);
	return shmea::GString((const char*)&be, (int)sizeof(be));
}

static shmea::GString build_frame(const shmea::GString& payload)
{
	const uint32_t header = 8;
	const uint32_t padding = 0;
	uint32_t blockSize = header + (uint32_t)payload.length() + padding;
	return u32_be(blockSize) + u32_be(padding) + payload;
}

void UDPUnitTest()
{
	// Make the key generation deterministic for this test.
	srand(12345);

	GNet::GServer server;
	server.disableEncryption(); // simplify test
	server.run("45019", false);

	// Give server time to start
	sleep_ms(50);

	// Create a UDP client socket bound to localhost:ephemeral
	socket_t cfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	ASSERT("socket(AF_INET,SOCK_DGRAM) failed", cfd != INVALID_SOCKET_VALUE);

	struct sockaddr_in caddr;
	memset(&caddr, 0, sizeof(caddr));
	caddr.sin_family = AF_INET;
	caddr.sin_port = htons(0);
	inet_pton(AF_INET, "127.0.0.1", &caddr.sin_addr);
	int brc = bind(cfd, (struct sockaddr*)&caddr, sizeof(caddr));
	ASSERT("bind UDP client failed", brc == 0);

	// Build a Handshake_Server request payload (framed)
	shmea::GList wData;
	wData.addString("UTClient");
	shmea::ServiceData req((GNet::Connection*)NULL, "Handshake_Server");
	req.set(wData);
	shmea::GString payload = shmea::Serializable::Serialize(&req);
	shmea::GString frame = build_frame(payload);

	struct sockaddr_in saddr;
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(atoi(server.getPort().c_str()));
	inet_pton(AF_INET, "127.0.0.1", &saddr.sin_addr);

	int sent = (int)sendto(cfd, frame.c_str(), frame.length(), 0, (struct sockaddr*)&saddr, sizeof(saddr));
	ASSERT("sendto failed", sent == (int)frame.length());

	// Wait up to 1s for a response datagram
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(cfd, &rfds);
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	int sel = select((int)(cfd + 1), &rfds, NULL, NULL, &tv);
	ASSERT("select() should indicate readable", sel == 1);

	char buf[4096];
	struct sockaddr_in from;
	socklen_t fromlen = sizeof(from);
	int n = recvfrom(cfd, buf, sizeof(buf), 0, (struct sockaddr*)&from, &fromlen);
	ASSERT("recvfrom() should succeed", n > 0);
	ASSERT("UDP response must have header", n >= 8);

	uint32_t blockSize = read_u32_be(buf);
	uint32_t padding = read_u32_be(buf + 4);
	ASSERT("blockSize >= header", blockSize >= 8);
	ASSERT("blockSize <= received bytes", blockSize <= (uint32_t)n);
	ASSERT("padding <= payloadWithPadding", padding <= (blockSize - 8));

	uint32_t payloadLen = (blockSize - 8) - padding;
	shmea::GString respPayload(buf + 8, (int)payloadLen);

	shmea::ServiceData resp((GNet::Connection*)NULL, "");
	shmea::Serializable::Deserialize(&resp, respPayload);

	ASSERT("Response command should be Handshake_Client", resp.getCommand() == "Handshake_Client");
	ASSERT("Response type should be list", resp.getType() == shmea::ServiceData::TYPE_LIST);
	ASSERT("Response list should have 2 items", resp.getList().size() == 2);
	ASSERT("Response list[0] name echoed", resp.getList().getString(0) == "UTClient");
	int64_t k = resp.getList().getLong(1);
	ASSERT("Response key is within 6-digit range", (k >= 0) && (k <= 999999));

	G_CLOSE_SOCKET(cfd);

	server.stop();
}


