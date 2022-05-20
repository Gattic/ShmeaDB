// Confidential, unpublished property of Robert Carneiro

// The access and distribution of this material is limited solely to
// authorized personnel.  The use, disclosure, reproduction,
// modification, transfer, or transmittal of this work for any purpose
// in any form or by any means without the written permission of
// Robert Carneiro is strictly prohibited.
#include "GPointer-test.h"
#include "../../unit-test.h"
#include "../../../Backend/Database/GPointer.h"
#include "../../../Backend/Database/GList.h"
#include "../../../Backend/Database/GString.h"

// This File will have the more advanced functionalities of GPointer
// For simpler tests check GList-test.cpp

// === This is the primary unit testing function:
// void G_assert(const char* fileName, int lineNo, const char* failureMsg, bool expr)

using namespace shmea;

void GPointerUnitTest()
{
	//
	GPointer<GList> p0(new GList());
	p0->addString("derp");
	p0->addString("herp");
	p0->addString("chirp");
	p0->addString("slurp");
	p0->addString("burp");

	G_assert (__FILE__, __LINE__, "==============GList::size() Failed==============", p0->size() == 5);
	G_assert (__FILE__, __LINE__, "==============list0[0] Failed==============", (*p0)[0] == "derp");
	G_assert (__FILE__, __LINE__, "==============list0[1] Failed==============", (*p0)[1] == "herp");
	G_assert (__FILE__, __LINE__, "==============list0[2] Failed==============", (*p0)[2] == "chirp");
	G_assert (__FILE__, __LINE__, "==============list0[3] Failed==============", (*p0)[3] == "slurp");
	G_assert (__FILE__, __LINE__, "==============list0[4] Failed==============", (*p0)[4] == "burp");
}
