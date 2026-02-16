// Confidential, unpublished property of Robert Carneiro
//
// The access and distribution of this material is limited solely to
// authorized personnel.  The use, disclosure, reproduction,
// modification, transfer, or transmittal of this work for any purpose
// in any form or by any means without the written permission of
// Robert Carneiro is strictly prohibited.
#include "binary-payload-test.h"
#include "../../unit-test.h"

#include "../../../Backend/Database/GString.h"
#include "../../../Backend/Database/GList.h"
#include "../../../Backend/Database/Serializable.h"
#include "../../../Backend/Database/ServiceData.h"
#include "../../../Backend/Networking/connection.h"

#include <string.h>
#include <math.h>
#include <vector>

namespace {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Build a buffer of `count` floats with a simple deterministic pattern.
static std::vector<float> make_float_buffer(unsigned int count)
{
	std::vector<float> buf(count);
	for (unsigned int i = 0; i < count; ++i)
		buf[i] = (float)i * 0.125f - 100.0f;
	return buf;
}

// Compare two byte buffers.
static bool buffers_equal(const char* a, const char* b, unsigned int len)
{
	return memcmp(a, b, len) == 0;
}

// Serialize a ServiceData, then deserialize into a fresh ServiceData and return it.
static shmea::ServiceData serialize_roundtrip(const shmea::ServiceData& src)
{
	shmea::GString wire = shmea::Serializable::Serialize(&src);
	shmea::ServiceData dst((GNet::Connection*)NULL, "");
	shmea::Serializable::Deserialize(&dst, wire);
	return dst;
}

// ---------------------------------------------------------------------------
// 1. setBinaryPayload sets type, payload, and size correctly
// ---------------------------------------------------------------------------
static void BinaryPayload_SetBinaryPayload_Basic()
{
	const char raw[] = {0x01, 0x02, 0x03, 0x04, 0x05};
	shmea::ServiceData sd((GNet::Connection*)NULL, "CMD");
	sd.setBinaryPayload(raw, 5);

	ASSERT("type should be TYPE_BINARY", sd.getType() == shmea::ServiceData::TYPE_BINARY);
	ASSERT("payload size should be 5", sd.getBinaryPayloadSize() == 5);
	ASSERT("payload content should match", buffers_equal(sd.getBinaryPayload().c_str(), raw, 5));
}

// ---------------------------------------------------------------------------
// 2. set(command, data, size) convenience overload
// ---------------------------------------------------------------------------
static void BinaryPayload_SetConvenience()
{
	const char raw[] = {(char)0xAA, (char)0xBB, (char)0xCC};
	shmea::ServiceData sd((GNet::Connection*)NULL, "CMD");
	sd.set("MY_KEY", raw, 3);

	ASSERT("type should be TYPE_BINARY", sd.getType() == shmea::ServiceData::TYPE_BINARY);
	ASSERT("serviceKey should be MY_KEY", sd.getServiceKey() == "MY_KEY");
	ASSERT("payload size should be 3", sd.getBinaryPayloadSize() == 3);
	ASSERT("payload content should match", buffers_equal(sd.getBinaryPayload().c_str(), raw, 3));
}

// ---------------------------------------------------------------------------
// 3. Empty binary payload (zero bytes)
// ---------------------------------------------------------------------------
static void BinaryPayload_Empty()
{
	shmea::ServiceData sd((GNet::Connection*)NULL, "CMD");
	sd.setBinaryPayload("", 0);

	ASSERT("type should be TYPE_BINARY", sd.getType() == shmea::ServiceData::TYPE_BINARY);
	ASSERT("payload size should be 0", sd.getBinaryPayloadSize() == 0);
}

// ---------------------------------------------------------------------------
// 4. Binary data containing embedded null bytes
// ---------------------------------------------------------------------------
static void BinaryPayload_NullBytes()
{
	const char raw[] = {0x41, 0x00, 0x42, 0x00, 0x00, 0x43};
	shmea::ServiceData sd((GNet::Connection*)NULL, "CMD");
	sd.setBinaryPayload(raw, 6);

	ASSERT("payload size should be 6", sd.getBinaryPayloadSize() == 6);
	ASSERT("payload should preserve nulls", buffers_equal(sd.getBinaryPayload().c_str(), raw, 6));
}

// ---------------------------------------------------------------------------
// 5. Binary data containing serializer delimiter characters (|, \|, %)
// ---------------------------------------------------------------------------
static void BinaryPayload_DelimiterChars()
{
	// Craft a payload that contains every character the serializer treats specially.
	const char raw[] = {'|', '\\', '|', '%', ',', '\\', '%', '|'};
	shmea::ServiceData sd((GNet::Connection*)NULL, "CMD");
	sd.setBinaryPayload(raw, 8);

	ASSERT("payload size should be 8", sd.getBinaryPayloadSize() == 8);
	ASSERT("payload content should match", buffers_equal(sd.getBinaryPayload().c_str(), raw, 8));
}

// ---------------------------------------------------------------------------
// 6. Copy constructor preserves binary payload
// ---------------------------------------------------------------------------
static void BinaryPayload_CopyConstructor()
{
	const char raw[] = {0x10, 0x20, 0x30};
	shmea::ServiceData src((GNet::Connection*)NULL, "COPY");
	src.setBinaryPayload(raw, 3);
	src.setServiceKey("KEY");
	src.setServiceNum(42);

	shmea::ServiceData dst(src);

	ASSERT("copy type should be TYPE_BINARY", dst.getType() == shmea::ServiceData::TYPE_BINARY);
	ASSERT("copy payload size", dst.getBinaryPayloadSize() == 3);
	ASSERT("copy payload content", buffers_equal(dst.getBinaryPayload().c_str(), raw, 3));
	ASSERT("copy command", dst.getCommand() == "COPY");
	ASSERT("copy serviceKey", dst.getServiceKey() == "KEY");
	ASSERT("copy serviceNum", dst.getServiceNum() == 42);
}

// ---------------------------------------------------------------------------
// 7. Type override: setting binary payload on a TYPE_LIST SD switches type
// ---------------------------------------------------------------------------
static void BinaryPayload_TypeOverride()
{
	shmea::GList list;
	list.addString("hello");
	shmea::ServiceData sd((GNet::Connection*)NULL, "CMD");
	sd.set(list);
	ASSERT("initially TYPE_LIST", sd.getType() == shmea::ServiceData::TYPE_LIST);

	const char raw[] = {(char)0xFF};
	sd.setBinaryPayload(raw, 1);
	ASSERT("after setBinaryPayload should be TYPE_BINARY", sd.getType() == shmea::ServiceData::TYPE_BINARY);
}

// ---------------------------------------------------------------------------
// 8. Serialize/Deserialize round-trip: basic
// ---------------------------------------------------------------------------
static void BinaryPayload_RoundTrip_Basic()
{
	const char raw[] = {(char)0xDE, (char)0xAD, (char)0xBE, (char)0xEF};
	shmea::ServiceData src((GNet::Connection*)NULL, "BIN_CMD");
	src.setBinaryPayload(raw, 4);
	src.setServiceKey("BIN_KEY");
	src.setServiceNum(100);
	src.setResponseServiceNum(101);

	shmea::ServiceData dst = serialize_roundtrip(src);

	ASSERT("RT type", dst.getType() == shmea::ServiceData::TYPE_BINARY);
	ASSERT("RT command", dst.getCommand() == "BIN_CMD");
	ASSERT("RT serviceKey", dst.getServiceKey() == "BIN_KEY");
	ASSERT("RT serviceNum", dst.getServiceNum() == 100);
	ASSERT("RT responseServiceNum", dst.getResponseServiceNum() == 101);
	ASSERT("RT payload size", dst.getBinaryPayloadSize() == 4);
	ASSERT("RT payload content", buffers_equal(dst.getBinaryPayload().c_str(), raw, 4));
}

// ---------------------------------------------------------------------------
// 9. Serialize/Deserialize round-trip with args
// ---------------------------------------------------------------------------
static void BinaryPayload_RoundTrip_WithArgs()
{
	const char raw[] = {0x01, 0x02, 0x03};
	shmea::ServiceData src((GNet::Connection*)NULL, "CMD");
	src.setBinaryPayload(raw, 3);

	shmea::GList args;
	args.addString("alpha");
	args.addInt(77);
	args.addFloat(3.14f);
	src.setArgList(args);

	shmea::ServiceData dst = serialize_roundtrip(src);

	ASSERT("RT+args type", dst.getType() == shmea::ServiceData::TYPE_BINARY);
	ASSERT("RT+args payload size", dst.getBinaryPayloadSize() == 3);
	ASSERT("RT+args payload content", buffers_equal(dst.getBinaryPayload().c_str(), raw, 3));
	ASSERT("RT+args argList size", dst.getArgList().size() == 3);
	ASSERT("RT+args arg[0]", dst.getArgList().getString(0) == "alpha");
	ASSERT("RT+args arg[1]", dst.getArgList().getInt(1) == 77);
	// Float comparison with tolerance
	float diff = dst.getArgList().getFloat(2) - 3.14f;
	ASSERT("RT+args arg[2] float", diff > -0.001f && diff < 0.001f);
}

// ---------------------------------------------------------------------------
// 10. Serialize/Deserialize round-trip: args with escape chars + binary payload
//     with escape chars. Both should survive independently.
// ---------------------------------------------------------------------------
static void BinaryPayload_RoundTrip_EscapeCharsInBoth()
{
	// Binary payload containing delimiter bytes
	const char raw[] = {'|', '\\', '|', '%', 0x00, (char)0xFF};
	shmea::ServiceData src((GNet::Connection*)NULL, "ESC");
	src.setBinaryPayload(raw, 6);

	shmea::GList args;
	args.addString("pipe|in|arg");
	args.addString("backslash\\here");
	src.setArgList(args);

	shmea::ServiceData dst = serialize_roundtrip(src);

	ASSERT("ESC type", dst.getType() == shmea::ServiceData::TYPE_BINARY);
	ASSERT("ESC payload size", dst.getBinaryPayloadSize() == 6);
	ASSERT("ESC payload content", buffers_equal(dst.getBinaryPayload().c_str(), raw, 6));
	ASSERT("ESC arg[0]", dst.getArgList().getString(0) == "pipe|in|arg");
	ASSERT("ESC arg[1]", dst.getArgList().getString(1) == "backslash\\here");
}

// ---------------------------------------------------------------------------
// 11. Serialize/Deserialize round-trip: empty binary payload
// ---------------------------------------------------------------------------
static void BinaryPayload_RoundTrip_Empty()
{
	shmea::ServiceData src((GNet::Connection*)NULL, "EMPTY");
	src.setBinaryPayload("", 0);
	src.setServiceNum(50);

	shmea::ServiceData dst = serialize_roundtrip(src);

	ASSERT("empty RT type", dst.getType() == shmea::ServiceData::TYPE_BINARY);
	ASSERT("empty RT payload size", dst.getBinaryPayloadSize() == 0);
	ASSERT("empty RT command", dst.getCommand() == "EMPTY");
	ASSERT("empty RT serviceNum", dst.getServiceNum() == 50);
}

// ---------------------------------------------------------------------------
// 12. Float array round-trip: simulate DDP gradient buffer
// ---------------------------------------------------------------------------
static void BinaryPayload_RoundTrip_FloatArray()
{
	const unsigned int count = 2048;
	std::vector<float> original = make_float_buffer(count);
	const unsigned int byteLen = count * sizeof(float);

	shmea::ServiceData src((GNet::Connection*)NULL, "GRAD");
	src.setBinaryPayload((const char*)&original[0], byteLen);

	shmea::ServiceData dst = serialize_roundtrip(src);

	ASSERT("float RT type", dst.getType() == shmea::ServiceData::TYPE_BINARY);
	ASSERT("float RT payload size", dst.getBinaryPayloadSize() == byteLen);

	const float* recovered = (const float*)dst.getBinaryPayload().c_str();
	bool allMatch = true;
	for (unsigned int i = 0; i < count; ++i)
	{
		if (recovered[i] != original[i])
		{
			allMatch = false;
			break;
		}
	}
	ASSERT("float RT all values match", allMatch);
}

// ---------------------------------------------------------------------------
// 13. All 256 byte values survive the round-trip
// ---------------------------------------------------------------------------
static void BinaryPayload_RoundTrip_AllByteValues()
{
	char raw[256];
	for (int i = 0; i < 256; ++i)
		raw[i] = (char)(unsigned char)i;

	shmea::ServiceData src((GNet::Connection*)NULL, "BYTES");
	src.setBinaryPayload(raw, 256);

	shmea::ServiceData dst = serialize_roundtrip(src);

	ASSERT("256 RT type", dst.getType() == shmea::ServiceData::TYPE_BINARY);
	ASSERT("256 RT payload size", dst.getBinaryPayloadSize() == 256);
	ASSERT("256 RT all bytes match", buffers_equal(dst.getBinaryPayload().c_str(), raw, 256));
}

// ---------------------------------------------------------------------------
// 14. Large binary payload round-trip (1 MB simulating real gradient buffer)
// ---------------------------------------------------------------------------
static void BinaryPayload_RoundTrip_LargePayload()
{
	const unsigned int byteLen = 1024 * 1024; // 1 MB
	std::vector<char> raw(byteLen);
	for (unsigned int i = 0; i < byteLen; ++i)
		raw[i] = (char)(unsigned char)(i & 0xFF);

	shmea::ServiceData src((GNet::Connection*)NULL, "LARGE");
	src.setBinaryPayload(&raw[0], byteLen);

	shmea::ServiceData dst = serialize_roundtrip(src);

	ASSERT("large RT type", dst.getType() == shmea::ServiceData::TYPE_BINARY);
	ASSERT("large RT payload size", dst.getBinaryPayloadSize() == byteLen);
	ASSERT("large RT payload content", buffers_equal(dst.getBinaryPayload().c_str(), &raw[0], byteLen));
}

// ---------------------------------------------------------------------------
// 15. Binary payload does not corrupt other ServiceData fields
// ---------------------------------------------------------------------------
static void BinaryPayload_FieldIntegrity()
{
	const char raw[] = {0x01, 0x02};
	shmea::ServiceData src((GNet::Connection*)NULL, "FIELD_CMD");
	src.setBinaryPayload(raw, 2);
	src.setServiceKey("FIELD_KEY");
	src.setServiceNum(999);
	src.setResponseServiceNum(1000);
	src.setSID("UTSID0000000");

	shmea::GList args;
	args.addString("x");
	args.addInt(42);
	args.addLong(123456789LL);
	src.setArgList(args);

	shmea::ServiceData dst = serialize_roundtrip(src);

	ASSERT("field SID", dst.getSID() == "UTSID0000000");
	ASSERT("field command", dst.getCommand() == "FIELD_CMD");
	ASSERT("field serviceKey", dst.getServiceKey() == "FIELD_KEY");
	ASSERT("field serviceNum", dst.getServiceNum() == 999);
	ASSERT("field responseServiceNum", dst.getResponseServiceNum() == 1000);
	ASSERT("field argList size", dst.getArgList().size() == 3);
	ASSERT("field arg[0]", dst.getArgList().getString(0) == "x");
	ASSERT("field arg[1]", dst.getArgList().getInt(1) == 42);
	ASSERT("field arg[2]", dst.getArgList().getLong(2) == 123456789LL);
	ASSERT("field payload size", dst.getBinaryPayloadSize() == 2);
	ASSERT("field payload content", buffers_equal(dst.getBinaryPayload().c_str(), raw, 2));
}

// ---------------------------------------------------------------------------
// 16. Single-byte payload (boundary: smallest non-empty payload)
// ---------------------------------------------------------------------------
static void BinaryPayload_RoundTrip_SingleByte()
{
	const char raw[] = {0x42};
	shmea::ServiceData src((GNet::Connection*)NULL, "ONE");
	src.setBinaryPayload(raw, 1);

	shmea::ServiceData dst = serialize_roundtrip(src);

	ASSERT("1-byte RT type", dst.getType() == shmea::ServiceData::TYPE_BINARY);
	ASSERT("1-byte RT payload size", dst.getBinaryPayloadSize() == 1);
	ASSERT("1-byte RT content", (unsigned char)dst.getBinaryPayload().c_str()[0] == 0x42);
}

// ---------------------------------------------------------------------------
// 17. Payload entirely of null bytes
// ---------------------------------------------------------------------------
static void BinaryPayload_RoundTrip_AllNulls()
{
	char raw[64];
	memset(raw, 0, 64);

	shmea::ServiceData src((GNet::Connection*)NULL, "NULLS");
	src.setBinaryPayload(raw, 64);

	shmea::ServiceData dst = serialize_roundtrip(src);

	ASSERT("nulls RT type", dst.getType() == shmea::ServiceData::TYPE_BINARY);
	ASSERT("nulls RT payload size", dst.getBinaryPayloadSize() == 64);

	bool allZero = true;
	const char* out = dst.getBinaryPayload().c_str();
	for (int i = 0; i < 64; ++i)
	{
		if (out[i] != 0)
		{
			allZero = false;
			break;
		}
	}
	ASSERT("nulls RT all zeros", allZero);
}

// ---------------------------------------------------------------------------
// 18. TYPE_BINARY with no args (argList empty) round-trips cleanly
// ---------------------------------------------------------------------------
static void BinaryPayload_RoundTrip_NoArgs()
{
	const char raw[] = {(char)0xCA, (char)0xFE};
	shmea::ServiceData src((GNet::Connection*)NULL, "NOARGS");
	src.setBinaryPayload(raw, 2);
	// Intentionally do NOT set argList.

	shmea::ServiceData dst = serialize_roundtrip(src);

	ASSERT("noargs RT type", dst.getType() == shmea::ServiceData::TYPE_BINARY);
	ASSERT("noargs RT payload size", dst.getBinaryPayloadSize() == 2);
	ASSERT("noargs RT payload", buffers_equal(dst.getBinaryPayload().c_str(), raw, 2));
	ASSERT("noargs RT argList empty", dst.getArgList().size() == 0);
}

// ---------------------------------------------------------------------------
// 19. Repeated serialize/deserialize does not accumulate drift
// ---------------------------------------------------------------------------
static void BinaryPayload_RoundTrip_DoubleHop()
{
	const char raw[] = {0x11, 0x22, 0x33, 0x44, 0x55};
	shmea::ServiceData src((GNet::Connection*)NULL, "HOP");
	src.setBinaryPayload(raw, 5);
	src.setServiceNum(10);

	shmea::GList args;
	args.addString("hop");
	src.setArgList(args);

	// First hop
	shmea::ServiceData mid = serialize_roundtrip(src);
	// Second hop
	shmea::ServiceData dst = serialize_roundtrip(mid);

	ASSERT("double-hop type", dst.getType() == shmea::ServiceData::TYPE_BINARY);
	ASSERT("double-hop payload size", dst.getBinaryPayloadSize() == 5);
	ASSERT("double-hop payload content", buffers_equal(dst.getBinaryPayload().c_str(), raw, 5));
	ASSERT("double-hop command", dst.getCommand() == "HOP");
	ASSERT("double-hop serviceNum", dst.getServiceNum() == 10);
	ASSERT("double-hop arg[0]", dst.getArgList().getString(0) == "hop");
}

// ---------------------------------------------------------------------------
// 20. Payload that looks like serialized GList metadata (type+size prefix bytes)
//     should not confuse the deserializer.
// ---------------------------------------------------------------------------
static void BinaryPayload_RoundTrip_AdversarialContent()
{
	// Craft bytes that mimic a GList serialization header:
	// INT_TYPE(4 bytes) + size(4 bytes) + "data" + "|"
	// This should be treated as opaque binary, not parsed.
	const char raw[] = {
		0x00, 0x00, 0x00, 0x01,  // looks like type=1
		0x00, 0x00, 0x00, 0x04,  // looks like size=4
		'd', 'a', 't', 'a',     // looks like content
		'|',                     // looks like delimiter
		'\\', '|'               // looks like end delimiter
	};
	shmea::ServiceData src((GNet::Connection*)NULL, "ADV");
	src.setBinaryPayload(raw, 15);

	shmea::ServiceData dst = serialize_roundtrip(src);

	ASSERT("adversarial RT type", dst.getType() == shmea::ServiceData::TYPE_BINARY);
	ASSERT("adversarial RT payload size", dst.getBinaryPayloadSize() == 15);
	ASSERT("adversarial RT payload content", buffers_equal(dst.getBinaryPayload().c_str(), raw, 15));
}

} // namespace

void BinaryPayloadUnitTest()
{
	BinaryPayload_SetBinaryPayload_Basic();
	BinaryPayload_SetConvenience();
	BinaryPayload_Empty();
	BinaryPayload_NullBytes();
	BinaryPayload_DelimiterChars();
	BinaryPayload_CopyConstructor();
	BinaryPayload_TypeOverride();
	BinaryPayload_RoundTrip_Basic();
	BinaryPayload_RoundTrip_WithArgs();
	BinaryPayload_RoundTrip_EscapeCharsInBoth();
	BinaryPayload_RoundTrip_Empty();
	BinaryPayload_RoundTrip_FloatArray();
	BinaryPayload_RoundTrip_AllByteValues();
	BinaryPayload_RoundTrip_LargePayload();
	BinaryPayload_FieldIntegrity();
	BinaryPayload_RoundTrip_SingleByte();
	BinaryPayload_RoundTrip_AllNulls();
	BinaryPayload_RoundTrip_NoArgs();
	BinaryPayload_RoundTrip_DoubleHop();
	BinaryPayload_RoundTrip_AdversarialContent();
}
