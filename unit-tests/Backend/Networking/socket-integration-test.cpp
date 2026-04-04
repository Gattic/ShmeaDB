// Confidential, unpublished property of Robert Carneiro
//
// The access and distribution of this material is limited solely to
// authorized personnel.  The use, disclosure, reproduction,
// modification, transfer, or transmittal of this work for any purpose
// in any form or by any means without the written permission of
// Robert Carneiro is strictly prohibited.
#include "socket-integration-test.h"
#include "../../unit-test.h"

#include "../../../Backend/Database/GList.h"
#include "../../../Backend/Database/Serializable.h"
#include "../../../Backend/Database/ServiceData.h"
#include "../../../Backend/Networking/connection.h"
#include "../../../Backend/Networking/socket.h"

#include "../../../Backend/Core/platform.h"
#include <vector>

namespace {
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

static void Socket_ReadHelper_ReturnCodes()
{
	socket_t fds[2] = {INVALID_SOCKET_VALUE, INVALID_SOCKET_VALUE};
	MakeSocketpair(fds);

	GNet::Connection origin(fds[1], GNet::Connection::SERVER_TYPE, "local", "0");
	origin.disableEncryption();
	origin.setCloseOnFinish(false);

	GNet::Sockets socks;
	std::vector< shmea::GPointer<shmea::ServiceData> > out;

	// No data yet => 0
	int rc = socks.readConnectionHelper(&origin, origin.sockfd, out);
	ASSERT("readConnectionHelper on empty socket should return 0", rc == 0);
	ASSERT("no data should decode nothing", out.size() == 0);

	// Close peer => EOF => -1 (no frames)
	G_CLOSE_SOCKET(fds[0]);
	fds[0] = INVALID_SOCKET_VALUE;
	rc = socks.readConnectionHelper(&origin, origin.sockfd, out);
	ASSERT("EOF should return -1", rc == -1);

	ClosePair(fds);
}

static void Socket_WriteRead_RoundTrip_Unencrypted()
{
	socket_t fds[2] = {INVALID_SOCKET_VALUE, INVALID_SOCKET_VALUE};
	MakeSocketpair(fds);

	// Writer-side connection (TCP)
	GNet::Connection dest(fds[0], GNet::Connection::SERVER_TYPE, "local", "0");
	dest.disableEncryption();
	dest.setCloseOnFinish(false);

	// Reader-side origin (TCP)
	GNet::Connection origin(fds[1], GNet::Connection::SERVER_TYPE, "local", "0");
	origin.disableEncryption();
	origin.setCloseOnFinish(false);

	shmea::GList rep;
	rep.addString("hello");
	rep.addInt(7);

	shmea::GList args;
	args.addString("A");
	args.addString("B|C"); // escaping in serialization
	args.addLong(99);

	shmea::ServiceData sd(&dest, "UT_CMD");
	sd.set(rep);            // makes TYPE_LIST
	sd.setServiceKey("UT_KEY");
	sd.setArgList(args);

	GNet::Sockets socks;
	int wrote = socks.writeConnection(&dest, dest.sockfd, &sd);
	ASSERT("writeConnection should succeed", wrote > 0);
	ASSERT("writeConnection should assign serviceNum", sd.getServiceNum() > 0);

	std::vector< shmea::GPointer<shmea::ServiceData> > out;
	int rc = socks.readConnectionHelper(&origin, origin.sockfd, out);
	ASSERT("readConnectionHelper should make progress", rc == 1);
	ASSERT("Expected 1 decoded ServiceData", out.size() == 1);
	ASSERT("Decoded command mismatch", out[0] && out[0]->getCommand() == "UT_CMD");
	ASSERT("Decoded serviceKey mismatch", out[0] && out[0]->getServiceKey() == "UT_KEY");
	ASSERT("Decoded serviceNum should match sent", out[0] && out[0]->getServiceNum() == sd.getServiceNum());
	ASSERT("Decoded type should be list", out[0] && out[0]->getType() == shmea::ServiceData::TYPE_LIST);
	ASSERT("Decoded repList size", out[0] && out[0]->getList().size() == 2);
	ASSERT("Decoded repList[0]", out[0] && out[0]->getList().getString(0) == "hello");
	ASSERT("Decoded repList[1]", out[0] && out[0]->getList().getInt(1) == 7);
	ASSERT("Decoded argList size", out[0] && out[0]->getArgList().size() == 3);
	ASSERT("Decoded argList[1] escaping", out[0] && out[0]->getArgList().getString(1) == "B|C");
	ASSERT("Decoded connection should be reader origin", out[0] && out[0]->getConnection() == &origin);

	ClosePair(fds);
}

static void Socket_WriteRead_MultipleFrames()
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

	shmea::ServiceData a(&dest, "A");
	a.set(shmea::GList());
	shmea::ServiceData b(&dest, "B");
	b.set(shmea::GList());

	int wa = socks.writeConnection(&dest, dest.sockfd, &a);
	int wb = socks.writeConnection(&dest, dest.sockfd, &b);
	ASSERT("write A ok", wa > 0);
	ASSERT("write B ok", wb > 0);

	std::vector< shmea::GPointer<shmea::ServiceData> > out;
	int rc = socks.readConnectionHelper(&origin, origin.sockfd, out);
	ASSERT("readConnectionHelper should progress", rc == 1);
	ASSERT("Should decode 2 frames", out.size() == 2);
	ASSERT("First command", out[0] && out[0]->getCommand() == "A");
	ASSERT("Second command", out[1] && out[1]->getCommand() == "B");

	ClosePair(fds);
}
} // namespace

void SocketIntegrationUnitTest()
{
	Socket_ReadHelper_ReturnCodes();
	Socket_WriteRead_RoundTrip_Unencrypted();
	Socket_WriteRead_MultipleFrames();
}

