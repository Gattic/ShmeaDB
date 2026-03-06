// Confidential, unpublished property of Robert Carneiro
//
// The access and distribution of this material is limited solely to
// authorized personnel.  The use, disclosure, reproduction,
// modification, transfer, or transmittal of this work for any purpose
// in any form or by any means without the written permission of
// Robert Carneiro is strictly prohibited.
#include "connection-test.h"
#include "../../unit-test.h"

#include "../../../Backend/Networking/connection.h"

#include <string>

static void Connection_ValidName_Boundaries()
{
	ASSERT("Empty name invalid", !GNet::Connection::validName(""));
	ASSERT("Length 1 name valid", GNet::Connection::validName("a"));

	// 40 chars max
	std::string forty(40, 'a');
	std::string fortyOne(41, 'a');
	ASSERT("Length 40 name valid", GNet::Connection::validName(forty.c_str()));
	ASSERT("Length 41 name invalid", !GNet::Connection::validName(fortyOne.c_str()));

	ASSERT("Whitespace invalid", !GNet::Connection::validName("has space"));
	ASSERT("Pipe invalid (not in allowlist)", !GNet::Connection::validName("bad|name"));
	ASSERT("Common allowed punctuation valid", GNet::Connection::validName("AZaz09_-+=!@#$%^&*()"));
}

static void Connection_Key_Generation_IsNumericAndInRange()
{
	// generateKey() is 6 digits (may include leading zeros -> numeric value can be 0..999999)
	for (int i = 0; i < 100; ++i)
	{
		int64_t k = GNet::Connection::generateKey();
		ASSERT("Key should be non-negative", k >= 0);
		ASSERT("Key should be <= 999999", k <= 999999);
	}
}

static void Connection_Counters_ClampAtZero()
{
	GNet::Connection c(-1, GNet::Connection::CLIENT_TYPE, "local", "0");
	c.setCloseOnFinish(false);

	ASSERT("inFlight starts at 0", c.getInFlightServices() == 0);
	c.decInFlight();
	ASSERT("decInFlight clamps at 0", c.getInFlightServices() == 0);
	c.incInFlight();
	ASSERT("incInFlight increments", c.getInFlightServices() == 1);
	c.decInFlight();
	ASSERT("decInFlight decrements", c.getInFlightServices() == 0);

	ASSERT("pendingSends starts at 0", c.getPendingSends() == 0);
	c.decPendingSends();
	ASSERT("decPendingSends clamps at 0", c.getPendingSends() == 0);
	c.incPendingSends();
	ASSERT("incPendingSends increments", c.getPendingSends() == 1);
	c.decPendingSends();
	ASSERT("decPendingSends decrements", c.getPendingSends() == 0);
}

static void Connection_LastSeen_Updates()
{
	GNet::Connection c(-1, GNet::Connection::CLIENT_TYPE, "local", "0");
	c.setCloseOnFinish(false);

	int64_t before = c.getLastSeenSec();
	g_sleep_ms(10);
	c.noteSeen();
	int64_t after = c.getLastSeenSec();
	ASSERT("noteSeen() should not go backwards", after >= before);
}

void ConnectionUnitTest()
{
	Connection_ValidName_Boundaries();
	Connection_Key_Generation_IsNumericAndInRange();
	Connection_Counters_ClampAtZero();
	Connection_LastSeen_Updates();
}

