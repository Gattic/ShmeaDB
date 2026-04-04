// Confidential, unpublished property of Robert Carneiro
//
// The access and distribution of this material is limited solely to
// authorized personnel.  The use, disclosure, reproduction,
// modification, transfer, or transmittal of this work for any purpose
// in any form or by any means without the written permission of
// Robert Carneiro is strictly prohibited.
#include "protocol-fuzz-test.h"
#include "../../unit-test.h"

#include "../../../Backend/Database/GList.h"
#include "../../../Backend/Database/GString.h"
#include "../../../Backend/Database/Serializable.h"
#include "../../../Backend/Database/ServiceData.h"
#include "../../../Backend/Networking/connection.h"
#include "../../../Backend/Networking/socket.h"

#include <stdint.h>
#include <string.h>
#include "../../../Backend/Core/platform.h"

namespace {
static const uint32_t FRAME_HEADER_BYTES = 8;               // [blockSize(4)][padding(4)]
static const uint32_t MAX_FRAME_BYTES = 16 * 1024 * 1024;   // must match protocol cap in socket.cpp

struct PRNG
{
	uint32_t s;
	explicit PRNG(uint32_t seed) : s(seed) {}
	uint32_t next()
	{
		// LCG (deterministic, portable, good enough for tests)
		s = s * 1664525u + 1013904223u;
		return s;
	}
	uint32_t uniform(uint32_t n)
	{
		if (n == 0)
			return 0;
		return next() % n;
	}
	uint8_t byte()
	{
		return (uint8_t)(next() & 0xFFu);
	}
};

static bool write_all_bytes(socket_t fd, const char* buf, size_t len)
{
	size_t off = 0;
	while (off < len)
	{
		int rc = (int)::send(fd, buf + off, len - off, G_MSG_NOSIGNAL);
		if (rc > 0)
		{
			off += (size_t)rc;
			continue;
		}
		if (rc == 0)
			return false;
		if (G_LAST_SOCK_ERROR() == G_EINTR)
			continue;
		return false;
	}
	return true;
}

static shmea::GString u32_be(uint32_t v)
{
	uint32_t be = htonl(v);
	return shmea::GString((const char*)&be, (int)sizeof(be));
}

static shmea::GString build_frame(const shmea::GString& payload, uint32_t padding)
{
	// padding is currently unused in production code; keep it 0 for correctness tests
	shmea::GString frame = "";
	uint32_t blockSize = FRAME_HEADER_BYTES + (uint32_t)payload.length() + padding;
	frame += u32_be(blockSize);
	frame += u32_be(padding);
	frame += payload;
	if (padding > 0)
	{
		for (uint32_t i = 0; i < padding; ++i)
			frame += '\0';
	}
	return frame;
}

static shmea::GString build_serialized_service(
	const shmea::GString& sid,
	int64_t svcNum,
	int64_t respNum,
	int type,
	const shmea::GString& cmd,
	const shmea::GString& skey,
	const shmea::GList& args,
	const shmea::GList* repList)
{
	// Construct a ServiceData only as a serialization helper; do not depend on its random SID.
	shmea::ServiceData tmp((GNet::Connection*)NULL, cmd);
	tmp.setSID(sid);
	tmp.setServiceNum(svcNum);
	tmp.setResponseServiceNum(respNum);
	tmp.setType(type);
	tmp.setServiceKey(skey);
	tmp.setArgList(args);
	if (repList)
		tmp.setList(*repList);
	return shmea::Serializable::Serialize(&tmp);
}

static void parse_from_socketpair(const shmea::GString& bytes, std::vector< shmea::GPointer<shmea::ServiceData> >& out)
{
	socket_t fds[2];
	int rc = g_socketpair(fds);
	ASSERT("socketpair() failed", rc == 0);

	// Reader side parses using Sockets::readConnectionHelper()
	GNet::Connection origin(fds[1], GNet::Connection::SERVER_TYPE, "local");
	origin.disableEncryption();
	origin.setCloseOnFinish(false);

	// Write bytes into the writer socket
	bool ok = write_all_bytes(fds[0], bytes.c_str(), (size_t)bytes.length());
	ASSERT("write_all_bytes() failed", ok);

	// Parse
	GNet::Sockets socks;
	(void)socks.readConnectionHelper(&origin, origin.sockfd, out);

	G_CLOSE_SOCKET(fds[0]);
	G_CLOSE_SOCKET(fds[1]);
}

static void Protocol_Frame_RoundTrip_Single()
{
	// One valid unencrypted frame should deserialize to exactly one ServiceData
	shmea::GList args;
	args.addString("A");
	args.addString("B|C"); // exercises escaping
	args.addInt(123);

	shmea::GList rep;
	rep.addString("hello");
	rep.addString("world");

	shmea::GString payload = build_serialized_service(
		"UTSID0000000", 100, 101, shmea::ServiceData::TYPE_LIST, "UT_Command", "UT_Key", args, &rep);
	shmea::GString frame = build_frame(payload, 0);

	std::vector< shmea::GPointer<shmea::ServiceData> > out;
	parse_from_socketpair(frame, out);

	ASSERT("Expected exactly one service from valid frame", out.size() == 1);
	ASSERT("ServiceData command mismatch", out[0] && out[0]->getCommand() == "UT_Command");
	ASSERT("ServiceData serviceKey mismatch", out[0] && out[0]->getServiceKey() == "UT_Key");
	ASSERT("ServiceData SID mismatch", out[0] && out[0]->getSID() == "UTSID0000000");
	ASSERT("ServiceData serviceNum mismatch", out[0] && out[0]->getServiceNum() == 100);
	ASSERT("ServiceData respNum mismatch", out[0] && out[0]->getResponseServiceNum() == 101);
	ASSERT("ServiceData type mismatch", out[0] && out[0]->getType() == shmea::ServiceData::TYPE_LIST);
	ASSERT("ServiceData argList size mismatch", out[0] && out[0]->getArgList().size() == 3);
	ASSERT("ServiceData argList[1] escaping mismatch", out[0] && out[0]->getArgList().getString(1) == "B|C");
	ASSERT("ServiceData repList size mismatch", out[0] && out[0]->getList().size() == 2);
	ASSERT("ServiceData repList[0] mismatch", out[0] && out[0]->getList().getString(0) == "hello");
}

static void Protocol_Frame_RoundTrip_MultipleFrames()
{
	// Multiple frames back-to-back in one read must decode all of them
	shmea::GList args;
	args.addString("x");

	shmea::GString payload1 = build_serialized_service(
		"UTSID0000000", 1, 2, shmea::ServiceData::TYPE_ACK, "C1", "K1", args, NULL);
	shmea::GString payload2 = build_serialized_service(
		"UTSID0000000", 3, 4, shmea::ServiceData::TYPE_ACK, "C2", "K2", args, NULL);

	shmea::GString bytes = build_frame(payload1, 0) + build_frame(payload2, 0);

	std::vector< shmea::GPointer<shmea::ServiceData> > out;
	parse_from_socketpair(bytes, out);

	ASSERT("Expected two services from two frames", out.size() == 2);
	ASSERT("First command mismatch", out[0] && out[0]->getCommand() == "C1");
	ASSERT("Second command mismatch", out[1] && out[1]->getCommand() == "C2");
}

static void Protocol_Frame_PartialDelivery()
{
	// A frame delivered in pieces should not produce a ServiceData until complete.
	shmea::GList args;
	args.addString("partial");

	shmea::GString payload = build_serialized_service(
		"UTSID0000000", 55, 56, shmea::ServiceData::TYPE_ACK, "PART", "SK", args, NULL);
	shmea::GString frame = build_frame(payload, 0);

	socket_t fds[2];
	int rc = g_socketpair(fds);
	ASSERT("socketpair() failed", rc == 0);

	GNet::Connection origin(fds[1], GNet::Connection::SERVER_TYPE, "local");
	origin.disableEncryption();
	origin.setCloseOnFinish(false);

	GNet::Sockets socks;
	std::vector< shmea::GPointer<shmea::ServiceData> > out;

	// Write only a prefix (header + a little payload)
	unsigned int cut = (unsigned int)(FRAME_HEADER_BYTES + 3);
	if (cut > (unsigned int)frame.length())
		cut = (unsigned int)frame.length();
	bool ok = write_all_bytes(fds[0], frame.c_str(), cut);
	ASSERT("write prefix failed", ok);
	socks.readConnectionHelper(&origin, origin.sockfd, out);
	ASSERT("Partial frame should not decode", out.size() == 0);

	// Write the remaining bytes
	ok = write_all_bytes(fds[0], frame.c_str() + cut, (size_t)(frame.length() - cut));
	ASSERT("write suffix failed", ok);
	socks.readConnectionHelper(&origin, origin.sockfd, out);

	ASSERT("Complete frame should decode exactly one", out.size() == 1);
	ASSERT("Decoded command mismatch after partial delivery", out[0] && out[0]->getCommand() == "PART");

	G_CLOSE_SOCKET(fds[0]);
	G_CLOSE_SOCKET(fds[1]);
}

static void Protocol_Frame_InvalidBlockSize_DoesNotCrash()
{
	// blockSize < header bytes is invalid; parser should not crash.
	shmea::GString bad = u32_be(4) + u32_be(0) + shmea::GString("xxxx", 4);
	std::vector< shmea::GPointer<shmea::ServiceData> > out;
	parse_from_socketpair(bad, out);
	ASSERT("Invalid blockSize should not decode any service", out.size() == 0);
}

static void Protocol_Frame_TooLarge_DoesNotCrash()
{
	// blockSize > MAX_FRAME_BYTES should be rejected without OOM or crash.
	uint32_t tooBig = MAX_FRAME_BYTES + 1;
	shmea::GString bad = u32_be(tooBig) + u32_be(0) + shmea::GString("x", 1);
	std::vector< shmea::GPointer<shmea::ServiceData> > out;
	parse_from_socketpair(bad, out);
	ASSERT("Oversized frame should not decode any service", out.size() == 0);
}

static void Protocol_Fuzz_RandomBytes_NoCrash()
{
	// Deterministic fuzz: feed random bytes into parser; ensure no crash/hang.
	PRNG rng(0xC0FFEEu);
	for (int i = 0; i < 250; ++i)
	{
		socket_t fds[2];
		int rc = g_socketpair(fds);
		ASSERT("socketpair() failed", rc == 0);

		GNet::Connection origin(fds[1], GNet::Connection::SERVER_TYPE, "local");
		origin.disableEncryption();
		origin.setCloseOnFinish(false);

		// Create a bounded random blob (avoid gigantic overflows in tests)
		unsigned int n = 1 + (unsigned int)rng.uniform(2048);
		std::string blob;
		blob.resize(n);
		for (unsigned int j = 0; j < n; ++j)
			blob[j] = (char)rng.byte();

		// Occasionally prefix with a plausible header to increase coverage of framing code
		if ((rng.uniform(4) == 0) && blob.size() >= FRAME_HEADER_BYTES)
		{
			uint32_t claimed = (uint32_t)rng.uniform(4096);
			if (claimed < FRAME_HEADER_BYTES)
				claimed = FRAME_HEADER_BYTES;
			uint32_t padding = 0;
			// overwrite first 8 bytes as header
			uint32_t beClaimed = htonl(claimed);
			uint32_t bePadding = htonl(padding);
			memcpy(&blob[0], &beClaimed, sizeof(beClaimed));
			memcpy(&blob[4], &bePadding, sizeof(bePadding));
		}

		bool ok = write_all_bytes(fds[0], blob.data(), blob.size());
		ASSERT("fuzz write_all_bytes failed", ok);

		GNet::Sockets socks;
		std::vector< shmea::GPointer<shmea::ServiceData> > out;
		socks.readConnectionHelper(&origin, origin.sockfd, out);

		G_CLOSE_SOCKET(fds[0]);
		G_CLOSE_SOCKET(fds[1]);
	}

	// If we reached here, we didn't crash.
	ASSERT("Deterministic fuzz completed", true);
}
} // namespace

void ProtocolFuzzUnitTest()
{
	Protocol_Frame_RoundTrip_Single();
	Protocol_Frame_RoundTrip_MultipleFrames();
	Protocol_Frame_PartialDelivery();
	Protocol_Frame_InvalidBlockSize_DoesNotCrash();
	Protocol_Frame_TooLarge_DoesNotCrash();
	Protocol_Fuzz_RandomBytes_NoCrash();
}

