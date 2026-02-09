// Confidential, unpublished property of Robert Carneiro
//
// The access and distribution of this material is limited solely to
// authorized personnel.  The use, disclosure, reproduction,
// modification, transfer, or transmittal of this work for any purpose
// in any form or by any means without the written permission of
// Robert Carneiro is strictly prohibited.
#include "socket-advanced-test.h"
#include "../../unit-test.h"

#include "../../../Backend/Database/GList.h"
#include "../../../Backend/Database/GString.h"
#include "../../../Backend/Database/Serializable.h"
#include "../../../Backend/Database/ServiceData.h"
#include "../../../Backend/Networking/connection.h"
#include "../../../Backend/Networking/socket.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdint.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

namespace {
static const uint32_t FRAME_HEADER_BYTES = 8;               // [blockSize(4)][padding(4)]
static const uint32_t MAX_FRAME_BYTES = 16 * 1024 * 1024;   // must match socket.cpp

static void MakeSocketpair(int fds[2])
{
	int rc = ::socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
	ASSERT("socketpair failed", rc == 0);
}

static void ClosePair(int fds[2])
{
	if (fds[0] >= 0)
		::close(fds[0]);
	if (fds[1] >= 0)
		::close(fds[1]);
	fds[0] = -1;
	fds[1] = -1;
}

static bool write_all_bytes(int fd, const char* buf, size_t len)
{
	size_t off = 0;
	while (off < len)
	{
		ssize_t rc = ::send(fd, buf + off, len - off, MSG_NOSIGNAL);
		if (rc > 0)
		{
			off += (size_t)rc;
			continue;
		}
		if (rc == 0)
			return false;
		if (errno == EINTR)
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
	shmea::GString frame = "";
	uint32_t blockSize = FRAME_HEADER_BYTES + (uint32_t)payload.length() + padding;
	frame += u32_be(blockSize);
	frame += u32_be(padding);
	frame += payload;
	for (uint32_t i = 0; i < padding; ++i)
		frame += '\0';
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

static void Socket_Overflow_Reassembly_PartialHeaderThenPayload()
{
	int fds[2] = {-1, -1};
	MakeSocketpair(fds);

	GNet::Connection origin(fds[1], GNet::Connection::SERVER_TYPE, "local", "0");
	origin.disableEncryption();
	origin.setCloseOnFinish(false);

	shmea::GList args;
	args.addString("x");
	shmea::GString payload = build_serialized_service("UTSID0000000", 10, 11, shmea::ServiceData::TYPE_ACK, "OVR", "", args, NULL);
	shmea::GString frame = build_frame(payload, 0);

	GNet::Sockets socks;
	std::vector< shmea::GPointer<shmea::ServiceData> > out;

	// Write only 2 bytes of header.
	ASSERT("frame must be >=2 bytes", frame.length() >= 2);
	bool ok = write_all_bytes(fds[0], frame.c_str(), 2);
	ASSERT("write prefix failed", ok);

	int rc = socks.readConnectionHelper(&origin, origin.sockfd, out);
	ASSERT("Partial header should still count as progress", rc == 1);
	ASSERT("Partial header should not decode", out.size() == 0);
	ASSERT("Overflow should buffer partial bytes", origin.overflow.length() == 2);

	// Write the rest; should decode now.
	ok = write_all_bytes(fds[0], frame.c_str() + 2, (size_t)(frame.length() - 2));
	ASSERT("write suffix failed", ok);
	rc = socks.readConnectionHelper(&origin, origin.sockfd, out);
	ASSERT("Complete frame should decode", rc == 1);
	ASSERT("Expected one decoded message", out.size() == 1);
	ASSERT("Decoded command mismatch", out[0] && out[0]->getCommand() == "OVR");
	ASSERT("Overflow should be empty after full parse", origin.overflow.length() == 0);

	ClosePair(fds);
}

static void Socket_MultiFrames_WithTrailingPartial_PreservesOverflow()
{
	int fds[2] = {-1, -1};
	MakeSocketpair(fds);

	GNet::Connection origin(fds[1], GNet::Connection::SERVER_TYPE, "local", "0");
	origin.disableEncryption();
	origin.setCloseOnFinish(false);

	shmea::GList args;
	args.addString("a");
	shmea::GString p1 = build_serialized_service("UTSID0000000", 1, 2, shmea::ServiceData::TYPE_ACK, "A", "", args, NULL);
	shmea::GString p2 = build_serialized_service("UTSID0000000", 3, 4, shmea::ServiceData::TYPE_ACK, "B", "", args, NULL);
	shmea::GString p3 = build_serialized_service("UTSID0000000", 5, 6, shmea::ServiceData::TYPE_ACK, "C", "", args, NULL);
	shmea::GString f1 = build_frame(p1, 0);
	shmea::GString f2 = build_frame(p2, 0);
	shmea::GString f3 = build_frame(p3, 0);

	// Write 2 full frames + a partial 3rd frame.
	unsigned int cut = 5; // arbitrary small cut
	ASSERT("f3 must be longer than cut", (unsigned int)f3.length() > cut);
	shmea::GString bytes = f1 + f2 + f3.substr(0, cut);

	bool ok = write_all_bytes(fds[0], bytes.c_str(), (size_t)bytes.length());
	ASSERT("write bytes failed", ok);

	GNet::Sockets socks;
	std::vector< shmea::GPointer<shmea::ServiceData> > out;
	int rc = socks.readConnectionHelper(&origin, origin.sockfd, out);
	ASSERT("readConnectionHelper should progress", rc == 1);
	ASSERT("Should decode exactly 2 frames", out.size() == 2);
	ASSERT("Decoded[0] command", out[0] && out[0]->getCommand() == "A");
	ASSERT("Decoded[1] command", out[1] && out[1]->getCommand() == "B");
	ASSERT("Overflow should hold partial tail", origin.overflow.length() == (int)cut);

	// Now send remainder of third frame.
	ok = write_all_bytes(fds[0], f3.c_str() + cut, (size_t)(f3.length() - cut));
	ASSERT("write tail failed", ok);

	rc = socks.readConnectionHelper(&origin, origin.sockfd, out);
	ASSERT("readConnectionHelper should progress", rc == 1);
	ASSERT("Now should have 3 decoded total", out.size() == 3);
	ASSERT("Decoded[2] command", out[2] && out[2]->getCommand() == "C");
	ASSERT("Overflow should be empty", origin.overflow.length() == 0);

	ClosePair(fds);
}

static void Socket_Padding_StripsCorrectly()
{
	int fds[2] = {-1, -1};
	MakeSocketpair(fds);

	GNet::Connection origin(fds[1], GNet::Connection::SERVER_TYPE, "local", "0");
	origin.disableEncryption();
	origin.setCloseOnFinish(false);

	shmea::GList args;
	args.addString("pad");
	shmea::GString payload = build_serialized_service("UTSID0000000", 7, 8, shmea::ServiceData::TYPE_ACK, "PAD", "", args, NULL);

	// Add non-zero padding; parser should strip it.
	shmea::GString frame = build_frame(payload, 5);
	bool ok = write_all_bytes(fds[0], frame.c_str(), (size_t)frame.length());
	ASSERT("write padded frame failed", ok);

	GNet::Sockets socks;
	std::vector< shmea::GPointer<shmea::ServiceData> > out;
	int rc = socks.readConnectionHelper(&origin, origin.sockfd, out);
	ASSERT("readConnectionHelper should progress", rc == 1);
	ASSERT("Should decode exactly one", out.size() == 1);
	ASSERT("Decoded command mismatch", out[0] && out[0]->getCommand() == "PAD");
	ASSERT("Decoded svcNum mismatch", out[0] && out[0]->getServiceNum() == 7);

	ClosePair(fds);
}

static void Socket_InvalidPadding_IsFatalAndClearsOverflow()
{
	int fds[2] = {-1, -1};
	MakeSocketpair(fds);

	GNet::Connection origin(fds[1], GNet::Connection::SERVER_TYPE, "local", "0");
	origin.disableEncryption();
	origin.setCloseOnFinish(false);

	// blockSize = header + 1 payload byte; padding=2 => invalid (padding > payloadWithPaddingLen)
	shmea::GString bad = u32_be(FRAME_HEADER_BYTES + 1) + u32_be(2) + shmea::GString("x", 1);
	bool ok = write_all_bytes(fds[0], bad.c_str(), (size_t)bad.length());
	ASSERT("write bad frame failed", ok);

	GNet::Sockets socks;
	std::vector< shmea::GPointer<shmea::ServiceData> > out;
	int rc = socks.readConnectionHelper(&origin, origin.sockfd, out);
	ASSERT("Invalid padding should be fatal (-2)", rc == -2);
	ASSERT("No decode", out.size() == 0);
	ASSERT("Overflow cleared on fatal protocol error", origin.overflow.length() == 0);

	ClosePair(fds);
}

static void Socket_ClaimedTooLarge_IsRejected_EvenWithoutPayload()
{
	int fds[2] = {-1, -1};
	MakeSocketpair(fds);

	GNet::Connection origin(fds[1], GNet::Connection::SERVER_TYPE, "local", "0");
	origin.disableEncryption();
	origin.setCloseOnFinish(false);

	uint32_t tooBig = MAX_FRAME_BYTES + 1;
	shmea::GString hdr = u32_be(tooBig) + u32_be(0);
	bool ok = write_all_bytes(fds[0], hdr.c_str(), (size_t)hdr.length());
	ASSERT("write oversized header failed", ok);

	GNet::Sockets socks;
	std::vector< shmea::GPointer<shmea::ServiceData> > out;
	int rc = socks.readConnectionHelper(&origin, origin.sockfd, out);
	ASSERT("Oversized frame claim should be fatal (-2)", rc == -2);
	ASSERT("No decode", out.size() == 0);
	ASSERT("Overflow cleared", origin.overflow.length() == 0);

	ClosePair(fds);
}

static void Socket_EncryptedPayload_NotMultipleOf8_IsFatal()
{
	int fds[2] = {-1, -1};
	MakeSocketpair(fds);

	GNet::Connection origin(fds[1], GNet::Connection::SERVER_TYPE, "local", "0");
	// Leave encryption enabled (default true)
	origin.setCloseOnFinish(false);
	origin.setKey(123456);

	// Encrypted payload must be multiple of 8; craft a frame with 7-byte payload.
	shmea::GString payload("1234567", 7);
	shmea::GString frame = build_frame(payload, 0);
	bool ok = write_all_bytes(fds[0], frame.c_str(), (size_t)frame.length());
	ASSERT("write bad encrypted frame failed", ok);

	GNet::Sockets socks;
	std::vector< shmea::GPointer<shmea::ServiceData> > out;
	int rc = socks.readConnectionHelper(&origin, origin.sockfd, out);
	ASSERT("Bad encrypted payload length should be fatal (-2)", rc == -2);
	ASSERT("No decode", out.size() == 0);

	ClosePair(fds);
}

static void Socket_Encrypted_WriteRead_RoundTrip()
{
	int fds[2] = {-1, -1};
	MakeSocketpair(fds);

	const int64_t key = 424242;

	// Writer-side connection (encrypts using its key)
	GNet::Connection dest(fds[0], GNet::Connection::SERVER_TYPE, "local", "0");
	dest.setCloseOnFinish(false);
	dest.setKey(key);
	// encryption enabled by default

	// Reader-side origin (decrypts using its key)
	GNet::Connection origin(fds[1], GNet::Connection::SERVER_TYPE, "local", "0");
	origin.setCloseOnFinish(false);
	origin.setKey(key);

	shmea::GList rep;
	rep.addString("secret");
	rep.addInt(9);

	shmea::GList args;
	args.addString("E");
	args.addString("F");

	shmea::ServiceData sd(&dest, "ENC_CMD");
	sd.set(rep);
	sd.setServiceKey("ENC_KEY");
	sd.setArgList(args);

	GNet::Sockets socks;
	int wrote = socks.writeConnection(&dest, dest.sockfd, &sd);
	ASSERT("Encrypted writeConnection should succeed", wrote > 0);

	std::vector< shmea::GPointer<shmea::ServiceData> > out;
	int rc = socks.readConnectionHelper(&origin, origin.sockfd, out);
	ASSERT("Encrypted readConnectionHelper should progress", rc == 1);
	ASSERT("Should decode exactly one", out.size() == 1);
	ASSERT("Decoded command", out[0] && out[0]->getCommand() == "ENC_CMD");
	ASSERT("Decoded serviceKey", out[0] && out[0]->getServiceKey() == "ENC_KEY");
	ASSERT("Decoded type list", out[0] && out[0]->getType() == shmea::ServiceData::TYPE_LIST);
	ASSERT("Decoded repList[0]", out[0] && out[0]->getList().getString(0) == "secret");
	ASSERT("Decoded repList[1]", out[0] && out[0]->getList().getInt(1) == 9);

	ClosePair(fds);
}

static void Socket_OutboundCollision_And_Purge_AdjustsPendingSends()
{
	GNet::Sockets socks;
	GNet::Connection c(-1, GNet::Connection::CLIENT_TYPE, "local", "0");
	c.setCloseOnFinish(false);
	c.disableEncryption();

	// Two different ServiceData objects with the same serviceNum => queue-key collision.
	shmea::ServiceData* a = new shmea::ServiceData(&c, "A");
	a->setServiceNum(42);
	shmea::ServiceData* b = new shmea::ServiceData(&c, "B");
	b->setServiceNum(42);

	ASSERT("pending sends starts at 0", c.getPendingSends() == 0);
	socks.addResponseList(NULL, &c, shmea::GPointer<shmea::ServiceData>(a));
	ASSERT("pending sends increments on first enqueue", c.getPendingSends() == 1);
	socks.addResponseList(NULL, &c, shmea::GPointer<shmea::ServiceData>(b));
	ASSERT("pending sends should NOT increment on collision", c.getPendingSends() == 1);

	// Purge should drop queued outbound and decrement pending send refcount.
	socks.purgeConnection(&c);
	ASSERT("pending sends should be 0 after purge", c.getPendingSends() == 0);
}

static void Socket_InboundPurge_ClearsInboundQueue()
{
	int fds[2] = {-1, -1};
	MakeSocketpair(fds);

	GNet::Connection origin(fds[1], GNet::Connection::SERVER_TYPE, "local", "0");
	origin.disableEncryption();
	origin.setCloseOnFinish(false);

	shmea::GList args;
	args.addString("in");
	shmea::GString payload = build_serialized_service("UTSID0000000", 999, 1000, shmea::ServiceData::TYPE_ACK, "IN", "", args, NULL);
	shmea::GString frame = build_frame(payload, 0);
	bool ok = write_all_bytes(fds[0], frame.c_str(), (size_t)frame.length());
	ASSERT("write inbound frame failed", ok);

	GNet::Sockets socks;
	ASSERT("No inbound lists initially", !socks.anyInboundLists());
	bool keep = socks.readLists(&origin);
	ASSERT("readLists should succeed", keep);
	ASSERT("Should have inbound queued", socks.anyInboundLists());

	socks.purgeConnection(&origin);
	ASSERT("Inbound queue should be cleared after purge", !socks.anyInboundLists());

	ClosePair(fds);
}

static void Socket_InboundBackpressure_TriggersLogoutSignal()
{
	// This test verifies the backpressure cutoff in Sockets::readLists():
	// when inbound queue grows to its cap (8192), readLists returns false.
	//
	// Important: we avoid huge single writes (which can block) by feeding frames in batches.
	int fds[2] = {-1, -1};
	MakeSocketpair(fds);

	GNet::Connection origin(fds[1], GNet::Connection::SERVER_TYPE, "local", "0");
	origin.disableEncryption();
	origin.setCloseOnFinish(false);

	GNet::Sockets socks;

	// Deterministic payload construction (doesn't rely on random SID).
	shmea::GList args;
	args.addString("bp");

	int64_t svc = 1;
	bool okAll = true;
	bool keep = true;

	// Send in 128-frame batches until readLists() trips the inbound cap.
	for (int round = 0; round < 200 && keep; ++round)
	{
		shmea::GString batch = "";
		for (int i = 0; i < 128; ++i)
		{
			shmea::GString payload = build_serialized_service("UTSID0000000", svc, svc + 1, shmea::ServiceData::TYPE_ACK, "BP", "", args, NULL);
			batch += build_frame(payload, 0);
			svc += 2;
		}

		okAll = write_all_bytes(fds[0], batch.c_str(), (size_t)batch.length());
		ASSERT("batch write failed", okAll);

		keep = socks.readLists(&origin);
	}

	ASSERT("Expected backpressure to eventually return false", keep == false);

	ClosePair(fds);
}
} // namespace

void SocketAdvancedUnitTest()
{
	// Keep deterministic RNG impact low for other tests.
	srand(123);

	Socket_Overflow_Reassembly_PartialHeaderThenPayload();
	Socket_MultiFrames_WithTrailingPartial_PreservesOverflow();
	Socket_Padding_StripsCorrectly();
	Socket_InvalidPadding_IsFatalAndClearsOverflow();
	Socket_ClaimedTooLarge_IsRejected_EvenWithoutPayload();

	Socket_EncryptedPayload_NotMultipleOf8_IsFatal();
	Socket_Encrypted_WriteRead_RoundTrip();

	Socket_OutboundCollision_And_Purge_AdjustsPendingSends();
	Socket_InboundPurge_ClearsInboundQueue();
	Socket_InboundBackpressure_TriggersLogoutSignal();
}

