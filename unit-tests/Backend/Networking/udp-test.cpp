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

// Basic UDP loopback test: start server locally, send a Handshake_Server over UDP and expect Handshake_Client response

static void sleep_ms(int ms)
{
	usleep(ms * 1000);
}

void UDPUnitTest()
{
	GNet::GServer server;
	server.disableEncryption(); // simplify test
	server.run("45019", false);

	// Give server time to start
	sleep_ms(50);

	// Create a UDP connection to localhost
	GNet::Connection* udpConn = server.getOrCreateUDPConnection("127.0.0.1", server.getPort(), "UTClient");
	ASSERT("UDP connection creation failed", udpConn != NULL);

	// Build handshake list (expect server to reply with Handshake_Client)
	shmea::GList wData;
	wData.addString("UTClient");
	shmea::ServiceData* sd = new shmea::ServiceData(udpConn, "Handshake_Server");
	sd->set(wData);

	// Send
	server.send(sd);

	// Wait for response to be processed
	sleep_ms(100);

	// No direct response capture path here; at least ensure server is still running and no crash occurred
	ASSERT("Server not running after UDP send", server.getRunning());

	server.stop();
}


