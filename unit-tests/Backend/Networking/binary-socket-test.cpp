// Confidential, unpublished property of Robert Carneiro
//
// The access and distribution of this material is limited solely to
// authorized personnel.  The use, disclosure, reproduction,
// modification, transfer, or transmittal of this work for any purpose
// in any form or by any means without the written permission of
// Robert Carneiro is strictly prohibited.
#include "binary-socket-test.h"
#include "../../unit-test.h"

#include "../../../Backend/Database/GList.h"
#include "../../../Backend/Database/GString.h"
#include "../../../Backend/Database/Serializable.h"
#include "../../../Backend/Database/ServiceData.h"
#include "../../../Backend/Networking/connection.h"
#include "../../../Backend/Networking/socket.h"

#include "Backend/Core/platform.h"
#include <string.h>
#include <vector>

namespace {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static void MakeSocketpair(socket_t fds[2])
{
	int rc = g_socketpair(fds);
	ASSERT("socketpair failed", rc == 0);
}

static void ClosePair(socket_t fds[2])
{
	if (fds[0] != INVALID_SOCKET_VALUE)
		G_CLOSE_SOCKET(fds[0]);
	if (fds[1] != INVALID_SOCKET_VALUE)
		G_CLOSE_SOCKET(fds[1]);
	fds[0] = INVALID_SOCKET_VALUE;
	fds[1] = INVALID_SOCKET_VALUE;
}

static bool buffers_equal(const char* a, const char* b, unsigned int len)
{
	return memcmp(a, b, len) == 0;
}

// ---------------------------------------------------------------------------
// 1. Unencrypted write/read round-trip for TYPE_BINARY
// ---------------------------------------------------------------------------
static void BinarySocket_WriteRead_Unencrypted()
{
	socket_t fds[2] = {INVALID_SOCKET_VALUE, INVALID_SOCKET_VALUE};
	MakeSocketpair(fds);

	GNet::Connection dest(fds[0], GNet::Connection::SERVER_TYPE, "local", "0");
	dest.disableEncryption();
	dest.setCloseOnFinish(false);

	GNet::Connection origin(fds[1], GNet::Connection::SERVER_TYPE, "local", "0");
	origin.disableEncryption();
	origin.setCloseOnFinish(false);

	const char raw[] = {(char)0xDE, (char)0xAD, (char)0xBE, (char)0xEF, 0x00, (char)0xFF};
	shmea::ServiceData sd(&dest, "BIN_CMD");
	sd.setBinaryPayload(raw, 6);
	sd.setServiceKey("BIN_KEY");

	shmea::GList args;
	args.addString("tag");
	args.addInt(99);
	sd.setArgList(args);

	GNet::Sockets socks;
	int wrote = socks.writeConnection(&dest, dest.sockfd, &sd);
	ASSERT("write binary should succeed", wrote > 0);

	std::vector< shmea::GPointer<shmea::ServiceData> > out;
	int rc = socks.readConnectionHelper(&origin, origin.sockfd, out);
	ASSERT("read should make progress", rc == 1);
	ASSERT("should decode 1 message", out.size() == 1);
	ASSERT("decoded type", out[0] && out[0]->getType() == shmea::ServiceData::TYPE_BINARY);
	ASSERT("decoded command", out[0] && out[0]->getCommand() == "BIN_CMD");
	ASSERT("decoded serviceKey", out[0] && out[0]->getServiceKey() == "BIN_KEY");
	ASSERT("decoded payload size", out[0] && out[0]->getBinaryPayloadSize() == 6);
	ASSERT("decoded payload content", out[0] && buffers_equal(out[0]->getBinaryPayload().c_str(), raw, 6));
	ASSERT("decoded argList size", out[0] && out[0]->getArgList().size() == 2);
	ASSERT("decoded arg[0]", out[0] && out[0]->getArgList().getString(0) == "tag");
	ASSERT("decoded arg[1]", out[0] && out[0]->getArgList().getInt(1) == 99);

	ClosePair(fds);
}

// ---------------------------------------------------------------------------
// 2. Encrypted write/read round-trip for TYPE_BINARY
// ---------------------------------------------------------------------------
static void BinarySocket_WriteRead_Encrypted()
{
	socket_t fds[2] = {INVALID_SOCKET_VALUE, INVALID_SOCKET_VALUE};
	MakeSocketpair(fds);

	const int64_t key = 987654;

	GNet::Connection dest(fds[0], GNet::Connection::SERVER_TYPE, "local", "0");
	dest.setCloseOnFinish(false);
	dest.setKey(key);

	GNet::Connection origin(fds[1], GNet::Connection::SERVER_TYPE, "local", "0");
	origin.setCloseOnFinish(false);
	origin.setKey(key);

	const char raw[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, (char)0x88};
	shmea::ServiceData sd(&dest, "ENC_BIN");
	sd.setBinaryPayload(raw, 8);

	GNet::Sockets socks;
	int wrote = socks.writeConnection(&dest, dest.sockfd, &sd);
	ASSERT("encrypted write should succeed", wrote > 0);

	std::vector< shmea::GPointer<shmea::ServiceData> > out;
	int rc = socks.readConnectionHelper(&origin, origin.sockfd, out);
	ASSERT("encrypted read should progress", rc == 1);
	ASSERT("encrypted should decode 1", out.size() == 1);
	ASSERT("encrypted type", out[0] && out[0]->getType() == shmea::ServiceData::TYPE_BINARY);
	ASSERT("encrypted command", out[0] && out[0]->getCommand() == "ENC_BIN");
	ASSERT("encrypted payload size", out[0] && out[0]->getBinaryPayloadSize() == 8);
	ASSERT("encrypted payload content", out[0] && buffers_equal(out[0]->getBinaryPayload().c_str(), raw, 8));

	ClosePair(fds);
}

// ---------------------------------------------------------------------------
// 3. Multiple TYPE_BINARY frames in a single read
// ---------------------------------------------------------------------------
static void BinarySocket_MultipleFrames()
{
	socket_t fds[2] = {INVALID_SOCKET_VALUE, INVALID_SOCKET_VALUE};
	MakeSocketpair(fds);

	GNet::Connection dest(fds[0], GNet::Connection::SERVER_TYPE, "local", "0");
	dest.disableEncryption();
	dest.setCloseOnFinish(false);

	GNet::Connection origin(fds[1], GNet::Connection::SERVER_TYPE, "local", "0");
	origin.disableEncryption();
	origin.setCloseOnFinish(false);

	GNet::Sockets socks;

	const char raw1[] = {(char)0xAA, (char)0xBB};
	const char raw2[] = {(char)0xCC, (char)0xDD, (char)0xEE};
	const char raw3[] = {(char)0xFF};

	shmea::ServiceData sd1(&dest, "F1");
	sd1.setBinaryPayload(raw1, 2);
	shmea::ServiceData sd2(&dest, "F2");
	sd2.setBinaryPayload(raw2, 3);
	shmea::ServiceData sd3(&dest, "F3");
	sd3.setBinaryPayload(raw3, 1);

	int w1 = socks.writeConnection(&dest, dest.sockfd, &sd1);
	int w2 = socks.writeConnection(&dest, dest.sockfd, &sd2);
	int w3 = socks.writeConnection(&dest, dest.sockfd, &sd3);
	ASSERT("write F1", w1 > 0);
	ASSERT("write F2", w2 > 0);
	ASSERT("write F3", w3 > 0);

	std::vector< shmea::GPointer<shmea::ServiceData> > out;
	int rc = socks.readConnectionHelper(&origin, origin.sockfd, out);
	ASSERT("multi read should progress", rc == 1);
	ASSERT("should decode 3 frames", out.size() == 3);

	ASSERT("F1 command", out[0] && out[0]->getCommand() == "F1");
	ASSERT("F1 type", out[0] && out[0]->getType() == shmea::ServiceData::TYPE_BINARY);
	ASSERT("F1 size", out[0] && out[0]->getBinaryPayloadSize() == 2);
	ASSERT("F1 content", out[0] && buffers_equal(out[0]->getBinaryPayload().c_str(), raw1, 2));

	ASSERT("F2 command", out[1] && out[1]->getCommand() == "F2");
	ASSERT("F2 type", out[1] && out[1]->getType() == shmea::ServiceData::TYPE_BINARY);
	ASSERT("F2 size", out[1] && out[1]->getBinaryPayloadSize() == 3);
	ASSERT("F2 content", out[1] && buffers_equal(out[1]->getBinaryPayload().c_str(), raw2, 3));

	ASSERT("F3 command", out[2] && out[2]->getCommand() == "F3");
	ASSERT("F3 type", out[2] && out[2]->getType() == shmea::ServiceData::TYPE_BINARY);
	ASSERT("F3 size", out[2] && out[2]->getBinaryPayloadSize() == 1);
	ASSERT("F3 content", out[2] && buffers_equal(out[2]->getBinaryPayload().c_str(), raw3, 1));

	ClosePair(fds);
}

// ---------------------------------------------------------------------------
// 4. Large binary payload over socket (simulating gradient buffer ~128 KB)
// ---------------------------------------------------------------------------
static void BinarySocket_LargePayload()
{
	socket_t fds[2] = {INVALID_SOCKET_VALUE, INVALID_SOCKET_VALUE};
	MakeSocketpair(fds);

	// Increase socket buffer to handle the large payload without blocking
	int bufSz = 512 * 1024;
	setsockopt(fds[0], SOL_SOCKET, SO_SNDBUF, G_SETSOCKOPT_VAL(bufSz), sizeof(bufSz));
	setsockopt(fds[1], SOL_SOCKET, SO_RCVBUF, G_SETSOCKOPT_VAL(bufSz), sizeof(bufSz));

	GNet::Connection dest(fds[0], GNet::Connection::SERVER_TYPE, "local", "0");
	dest.disableEncryption();
	dest.setCloseOnFinish(false);

	GNet::Connection origin(fds[1], GNet::Connection::SERVER_TYPE, "local", "0");
	origin.disableEncryption();
	origin.setCloseOnFinish(false);

	// 128 KB of float data (32768 floats)
	const unsigned int floatCount = 32768;
	const unsigned int byteLen = floatCount * sizeof(float);
	std::vector<float> buf(floatCount);
	for (unsigned int i = 0; i < floatCount; ++i)
		buf[i] = (float)i * 0.001f - 16.0f;

	shmea::ServiceData sd(&dest, "GRAD");
	sd.setBinaryPayload((const char*)&buf[0], byteLen);

	GNet::Sockets socks;
	int wrote = socks.writeConnection(&dest, dest.sockfd, &sd);
	ASSERT("large write should succeed", wrote > 0);

	std::vector< shmea::GPointer<shmea::ServiceData> > out;
	int rc = socks.readConnectionHelper(&origin, origin.sockfd, out);
	ASSERT("large read should progress", rc == 1);
	ASSERT("should decode 1 large frame", out.size() == 1);
	ASSERT("large type", out[0] && out[0]->getType() == shmea::ServiceData::TYPE_BINARY);
	ASSERT("large payload size", out[0] && out[0]->getBinaryPayloadSize() == byteLen);

	const float* recovered = (const float*)out[0]->getBinaryPayload().c_str();
	bool allMatch = true;
	for (unsigned int i = 0; i < floatCount; ++i)
	{
		if (recovered[i] != buf[i])
		{
			allMatch = false;
			break;
		}
	}
	ASSERT("large payload float data matches", allMatch);

	ClosePair(fds);
}

// ---------------------------------------------------------------------------
// 5. Mixed message types: TYPE_LIST, TYPE_BINARY, TYPE_ACK in sequence
// ---------------------------------------------------------------------------
static void BinarySocket_MixedTypes()
{
	socket_t fds[2] = {INVALID_SOCKET_VALUE, INVALID_SOCKET_VALUE};
	MakeSocketpair(fds);

	GNet::Connection dest(fds[0], GNet::Connection::SERVER_TYPE, "local", "0");
	dest.disableEncryption();
	dest.setCloseOnFinish(false);

	GNet::Connection origin(fds[1], GNet::Connection::SERVER_TYPE, "local", "0");
	origin.disableEncryption();
	origin.setCloseOnFinish(false);

	GNet::Sockets socks;

	// Message 1: TYPE_LIST
	shmea::GList repList;
	repList.addString("hello");
	repList.addInt(42);
	shmea::ServiceData sdList(&dest, "LIST_CMD");
	sdList.set(repList);

	// Message 2: TYPE_BINARY
	const char raw[] = {(char)0xBE, (char)0xEF, 0x00, (char)0xCA, (char)0xFE};
	shmea::ServiceData sdBin(&dest, "BIN_CMD");
	sdBin.setBinaryPayload(raw, 5);

	// Message 3: TYPE_ACK
	shmea::ServiceData sdAck(&dest, "ACK_CMD");
	sdAck.set("ack_key");

	int w1 = socks.writeConnection(&dest, dest.sockfd, &sdList);
	int w2 = socks.writeConnection(&dest, dest.sockfd, &sdBin);
	int w3 = socks.writeConnection(&dest, dest.sockfd, &sdAck);
	ASSERT("write list", w1 > 0);
	ASSERT("write binary", w2 > 0);
	ASSERT("write ack", w3 > 0);

	std::vector< shmea::GPointer<shmea::ServiceData> > out;
	int rc = socks.readConnectionHelper(&origin, origin.sockfd, out);
	ASSERT("mixed read progress", rc == 1);
	ASSERT("should decode 3 mixed frames", out.size() == 3);

	// Verify LIST
	ASSERT("mixed[0] type is LIST", out[0] && out[0]->getType() == shmea::ServiceData::TYPE_LIST);
	ASSERT("mixed[0] command", out[0] && out[0]->getCommand() == "LIST_CMD");
	ASSERT("mixed[0] list size", out[0] && out[0]->getList().size() == 2);
	ASSERT("mixed[0] list[0]", out[0] && out[0]->getList().getString(0) == "hello");
	ASSERT("mixed[0] list[1]", out[0] && out[0]->getList().getInt(1) == 42);

	// Verify BINARY
	ASSERT("mixed[1] type is BINARY", out[1] && out[1]->getType() == shmea::ServiceData::TYPE_BINARY);
	ASSERT("mixed[1] command", out[1] && out[1]->getCommand() == "BIN_CMD");
	ASSERT("mixed[1] payload size", out[1] && out[1]->getBinaryPayloadSize() == 5);
	ASSERT("mixed[1] payload content", out[1] && buffers_equal(out[1]->getBinaryPayload().c_str(), raw, 5));

	// Verify ACK
	ASSERT("mixed[2] type is ACK", out[2] && out[2]->getType() == shmea::ServiceData::TYPE_ACK);
	ASSERT("mixed[2] command", out[2] && out[2]->getCommand() == "ACK_CMD");
	ASSERT("mixed[2] serviceKey", out[2] && out[2]->getServiceKey() == "ack_key");

	ClosePair(fds);
}

// ---------------------------------------------------------------------------
// 6. Binary payload with null bytes over socket (ensures wire transport is
//    binary-safe end-to-end)
// ---------------------------------------------------------------------------
static void BinarySocket_NullBytesInPayload()
{
	socket_t fds[2] = {INVALID_SOCKET_VALUE, INVALID_SOCKET_VALUE};
	MakeSocketpair(fds);

	GNet::Connection dest(fds[0], GNet::Connection::SERVER_TYPE, "local", "0");
	dest.disableEncryption();
	dest.setCloseOnFinish(false);

	GNet::Connection origin(fds[1], GNet::Connection::SERVER_TYPE, "local", "0");
	origin.disableEncryption();
	origin.setCloseOnFinish(false);

	// Payload is mostly null bytes with a few markers
	char raw[32];
	memset(raw, 0, 32);
	raw[0] = 0x01;
	raw[15] = 0x02;
	raw[31] = 0x03;

	shmea::ServiceData sd(&dest, "NULL_BIN");
	sd.setBinaryPayload(raw, 32);

	GNet::Sockets socks;
	int wrote = socks.writeConnection(&dest, dest.sockfd, &sd);
	ASSERT("null-bytes write", wrote > 0);

	std::vector< shmea::GPointer<shmea::ServiceData> > out;
	int rc = socks.readConnectionHelper(&origin, origin.sockfd, out);
	ASSERT("null-bytes read progress", rc == 1);
	ASSERT("null-bytes decode count", out.size() == 1);
	ASSERT("null-bytes type", out[0] && out[0]->getType() == shmea::ServiceData::TYPE_BINARY);
	ASSERT("null-bytes payload size", out[0] && out[0]->getBinaryPayloadSize() == 32);
	ASSERT("null-bytes payload content", out[0] && buffers_equal(out[0]->getBinaryPayload().c_str(), raw, 32));

	ClosePair(fds);
}

// ---------------------------------------------------------------------------
// 7. Binary payload with all 256 byte values over encrypted socket
// ---------------------------------------------------------------------------
static void BinarySocket_AllByteValues_Encrypted()
{
	socket_t fds[2] = {INVALID_SOCKET_VALUE, INVALID_SOCKET_VALUE};
	MakeSocketpair(fds);

	const int64_t key = 314159;

	GNet::Connection dest(fds[0], GNet::Connection::SERVER_TYPE, "local", "0");
	dest.setCloseOnFinish(false);
	dest.setKey(key);

	GNet::Connection origin(fds[1], GNet::Connection::SERVER_TYPE, "local", "0");
	origin.setCloseOnFinish(false);
	origin.setKey(key);

	char raw[256];
	for (int i = 0; i < 256; ++i)
		raw[i] = (char)(unsigned char)i;

	shmea::ServiceData sd(&dest, "ALL256");
	sd.setBinaryPayload(raw, 256);

	GNet::Sockets socks;
	int wrote = socks.writeConnection(&dest, dest.sockfd, &sd);
	ASSERT("256-byte encrypted write", wrote > 0);

	std::vector< shmea::GPointer<shmea::ServiceData> > out;
	int rc = socks.readConnectionHelper(&origin, origin.sockfd, out);
	ASSERT("256-byte encrypted read progress", rc == 1);
	ASSERT("256-byte encrypted decode count", out.size() == 1);
	ASSERT("256-byte encrypted type", out[0] && out[0]->getType() == shmea::ServiceData::TYPE_BINARY);
	ASSERT("256-byte encrypted payload size", out[0] && out[0]->getBinaryPayloadSize() == 256);
	ASSERT("256-byte encrypted payload content", out[0] && buffers_equal(out[0]->getBinaryPayload().c_str(), raw, 256));

	ClosePair(fds);
}

// ---------------------------------------------------------------------------
// 8. Empty binary payload over socket
// ---------------------------------------------------------------------------
static void BinarySocket_EmptyPayload()
{
	socket_t fds[2] = {INVALID_SOCKET_VALUE, INVALID_SOCKET_VALUE};
	MakeSocketpair(fds);

	GNet::Connection dest(fds[0], GNet::Connection::SERVER_TYPE, "local", "0");
	dest.disableEncryption();
	dest.setCloseOnFinish(false);

	GNet::Connection origin(fds[1], GNet::Connection::SERVER_TYPE, "local", "0");
	origin.disableEncryption();
	origin.setCloseOnFinish(false);

	shmea::ServiceData sd(&dest, "EMPTY_BIN");
	sd.setBinaryPayload("", 0);

	GNet::Sockets socks;
	int wrote = socks.writeConnection(&dest, dest.sockfd, &sd);
	ASSERT("empty binary write", wrote > 0);

	std::vector< shmea::GPointer<shmea::ServiceData> > out;
	int rc = socks.readConnectionHelper(&origin, origin.sockfd, out);
	ASSERT("empty binary read progress", rc == 1);
	ASSERT("empty binary decode count", out.size() == 1);
	ASSERT("empty binary type", out[0] && out[0]->getType() == shmea::ServiceData::TYPE_BINARY);
	ASSERT("empty binary payload size", out[0] && out[0]->getBinaryPayloadSize() == 0);

	ClosePair(fds);
}

// ---------------------------------------------------------------------------
// 9. Binary payload with args containing delimiters over socket
// ---------------------------------------------------------------------------
static void BinarySocket_DelimitersInArgsAndPayload()
{
	socket_t fds[2] = {INVALID_SOCKET_VALUE, INVALID_SOCKET_VALUE};
	MakeSocketpair(fds);

	GNet::Connection dest(fds[0], GNet::Connection::SERVER_TYPE, "local", "0");
	dest.disableEncryption();
	dest.setCloseOnFinish(false);

	GNet::Connection origin(fds[1], GNet::Connection::SERVER_TYPE, "local", "0");
	origin.disableEncryption();
	origin.setCloseOnFinish(false);

	// Payload full of delimiter-like bytes
	const char raw[] = {'|', '|', '\\', '|', '%', ',', 0x00, '|'};
	shmea::ServiceData sd(&dest, "DELIM");
	sd.setBinaryPayload(raw, 8);

	// Args with delimiters too
	shmea::GList args;
	args.addString("a|b");
	args.addString("c\\d");
	args.addString("e%f");
	sd.setArgList(args);

	GNet::Sockets socks;
	int wrote = socks.writeConnection(&dest, dest.sockfd, &sd);
	ASSERT("delimiter write", wrote > 0);

	std::vector< shmea::GPointer<shmea::ServiceData> > out;
	int rc = socks.readConnectionHelper(&origin, origin.sockfd, out);
	ASSERT("delimiter read progress", rc == 1);
	ASSERT("delimiter decode count", out.size() == 1);
	ASSERT("delimiter type", out[0] && out[0]->getType() == shmea::ServiceData::TYPE_BINARY);
	ASSERT("delimiter payload size", out[0] && out[0]->getBinaryPayloadSize() == 8);
	ASSERT("delimiter payload content", out[0] && buffers_equal(out[0]->getBinaryPayload().c_str(), raw, 8));
	ASSERT("delimiter arg[0]", out[0] && out[0]->getArgList().getString(0) == "a|b");
	ASSERT("delimiter arg[1]", out[0] && out[0]->getArgList().getString(1) == "c\\d");
	ASSERT("delimiter arg[2]", out[0] && out[0]->getArgList().getString(2) == "e%f");

	ClosePair(fds);
}

// ---------------------------------------------------------------------------
// 10. Binary followed by binary (no other types) to confirm no state leakage
// ---------------------------------------------------------------------------
static void BinarySocket_BackToBack_Binary()
{
	socket_t fds[2] = {INVALID_SOCKET_VALUE, INVALID_SOCKET_VALUE};
	MakeSocketpair(fds);

	GNet::Connection dest(fds[0], GNet::Connection::SERVER_TYPE, "local", "0");
	dest.disableEncryption();
	dest.setCloseOnFinish(false);

	GNet::Connection origin(fds[1], GNet::Connection::SERVER_TYPE, "local", "0");
	origin.disableEncryption();
	origin.setCloseOnFinish(false);

	GNet::Sockets socks;

	// First binary message: small
	const char raw1[] = {0x01};
	shmea::ServiceData sd1(&dest, "B2B_1");
	sd1.setBinaryPayload(raw1, 1);

	// Second binary message: larger, different content
	const char raw2[] = {0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B};
	shmea::ServiceData sd2(&dest, "B2B_2");
	sd2.setBinaryPayload(raw2, 10);

	int w1 = socks.writeConnection(&dest, dest.sockfd, &sd1);
	int w2 = socks.writeConnection(&dest, dest.sockfd, &sd2);
	ASSERT("b2b write 1", w1 > 0);
	ASSERT("b2b write 2", w2 > 0);

	std::vector< shmea::GPointer<shmea::ServiceData> > out;
	int rc = socks.readConnectionHelper(&origin, origin.sockfd, out);
	ASSERT("b2b read progress", rc == 1);
	ASSERT("b2b decode count", out.size() == 2);

	ASSERT("b2b[0] command", out[0] && out[0]->getCommand() == "B2B_1");
	ASSERT("b2b[0] type", out[0] && out[0]->getType() == shmea::ServiceData::TYPE_BINARY);
	ASSERT("b2b[0] size", out[0] && out[0]->getBinaryPayloadSize() == 1);
	ASSERT("b2b[0] content", out[0] && (unsigned char)out[0]->getBinaryPayload().c_str()[0] == 0x01);

	ASSERT("b2b[1] command", out[1] && out[1]->getCommand() == "B2B_2");
	ASSERT("b2b[1] type", out[1] && out[1]->getType() == shmea::ServiceData::TYPE_BINARY);
	ASSERT("b2b[1] size", out[1] && out[1]->getBinaryPayloadSize() == 10);
	ASSERT("b2b[1] content", out[1] && buffers_equal(out[1]->getBinaryPayload().c_str(), raw2, 10));

	ClosePair(fds);
}

} // namespace

void BinarySocketUnitTest()
{
	BinarySocket_WriteRead_Unencrypted();
	BinarySocket_WriteRead_Encrypted();
	BinarySocket_MultipleFrames();
	BinarySocket_LargePayload();
	BinarySocket_MixedTypes();
	BinarySocket_NullBytesInPayload();
	BinarySocket_AllByteValues_Encrypted();
	BinarySocket_EmptyPayload();
	BinarySocket_DelimitersInArgsAndPayload();
	BinarySocket_BackToBack_Binary();
}
