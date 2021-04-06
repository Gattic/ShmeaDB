// Confidential, unpublished property of Robert Carneiro

// The access and distribution of this material is limited solely to
// authorized personnel.  The use, disclosure, reproduction,
// modification, transfer, or transmittal of this work for any purpose
// in any form or by any means without the written permission of
// Robert Carneiro is strictly prohibited.
#include "GList-test.h"
#include "../../unit-test.h"
#include "../../../Backend/Database/GString.h"
#include "../../../Backend/Database/GList.h"
#include "../../../Backend/Database/GTable.h"
#include "../../../Backend/Database/GObject.h"
#include "../../../Backend/Database/Serializable.h"

// === This is the primary unit testing function:
// void G_assert(const char* fileName, int lineNo, const char* failureMsg, bool expr)

void GListUnitTest()
{
	shmea::GList list0;
	list0.addString("derp");
	list0.addString("herp");
	list0.addString("chirp");
	list0.addString("slurp");
	list0.addString("burp");

	G_assert (__FILE__, __LINE__, "==============GList::size() Failed==============", list0.size() == 5);
	G_assert (__FILE__, __LINE__, "==============list0[0] Failed==============", list0[0] == "derp");
	G_assert (__FILE__, __LINE__, "==============list0[1] Failed==============", list0[1] == "herp");
	G_assert (__FILE__, __LINE__, "==============list0[2] Failed==============", list0[2] == "chirp");
	G_assert (__FILE__, __LINE__, "==============list0[3] Failed==============", list0[3] == "slurp");
	G_assert (__FILE__, __LINE__, "==============list0[4] Failed==============", list0[4] == "burp");

	shmea::GList list0Copy = list0;
	G_assert (__FILE__, __LINE__, "==============GList::size() Failed==============", list0Copy.size() == 5);
	G_assert (__FILE__, __LINE__, "==============list0Copy[0] Failed==============", list0Copy[0] == "derp");
	G_assert (__FILE__, __LINE__, "==============list0Copy[1] Failed==============", list0Copy[1] == "herp");
	G_assert (__FILE__, __LINE__, "==============list0Copy[2] Failed==============", list0Copy[2] == "chirp");
	G_assert (__FILE__, __LINE__, "==============list0Copy[3] Failed==============", list0Copy[3] == "slurp");
	G_assert (__FILE__, __LINE__, "==============list0Copy[4] Failed==============", list0Copy[4] == "burp");

	list0Copy.clear();
	G_assert (__FILE__, __LINE__, "==============GList::copy-clear-size() Failed==============", list0Copy.size() == 0);
	G_assert (__FILE__, __LINE__, "==============GList::clear-size() Failed==============", list0.size() == 5);
	G_assert (__FILE__, __LINE__, "==============list0[0] Failed==============", list0[0] == "derp");
	G_assert (__FILE__, __LINE__, "==============list0[1] Failed==============", list0[1] == "herp");
	G_assert (__FILE__, __LINE__, "==============list0[2] Failed==============", list0[2] == "chirp");
	G_assert (__FILE__, __LINE__, "==============list0[3] Failed==============", list0[3] == "slurp");
	G_assert (__FILE__, __LINE__, "==============list0[4] Failed==============", list0[4] == "burp");


	shmea::GString serializedList0 = shmea::Serializable::Serialize(list0);
	shmea::GList deserializedList0;
	shmea::Serializable::Deserialize(deserializedList0, serializedList0);

	G_assert (__FILE__, __LINE__, "==============Serialize-GList::size() Failed==============", deserializedList0.size() == 5);
	G_assert (__FILE__, __LINE__, "==============deserializedList0[0] Failed==============", deserializedList0[0] == "derp");
	G_assert (__FILE__, __LINE__, "==============deserializedList0[0].getType() Failed==============", deserializedList0[0].getType() == shmea::GType::STRING_TYPE);
	G_assert (__FILE__, __LINE__, "==============deserializedList0[0].size() Failed==============", deserializedList0[0].size() == 4);
	G_assert (__FILE__, __LINE__, "==============deserializedList0[1] Failed==============", deserializedList0[1] == "herp");
	G_assert (__FILE__, __LINE__, "==============deserializedList0[2] Failed==============", deserializedList0[2] == "chirp");
	G_assert (__FILE__, __LINE__, "==============deserializedList0[3] Failed==============", deserializedList0[3] == "slurp");
	G_assert (__FILE__, __LINE__, "==============deserializedList0[4] Failed==============", deserializedList0[4] == "burp");

	shmea::GList list1;
	list1.addString("who");
	list1.addString("what");
	list1.addString("when");
	list1.addString("where");
	list1.addString("why");

	shmea::GList list2;
	list2.addInt(0);
	list2.addInt(1);
	list2.addInt(2);
	list2.addInt(3);
	list2.addInt(4);

	shmea::GList list3;
	list3.addInt(10);
	list3.addInt(9);
	list3.addInt(8);
	list3.addInt(6);
	list3.addInt(6);

	shmea::GTable table0;
	table0.addRow(list0);
	table0.addRow(list1);
	G_assert (__FILE__, __LINE__, "==============GTable::getNumRows() Failed==============", table0.numberOfRows() == 2);
	G_assert (__FILE__, __LINE__, "==============GTable::getNumCols() Failed==============", table0.numberOfCols() == 5);

	G_assert (__FILE__, __LINE__, "==============table0[0].getType() Failed==============", table0[0][0].getType() == shmea::GType::STRING_TYPE);
	G_assert (__FILE__, __LINE__, "==============table0[0].size() Failed==============", table0[0][0].size() == 4);
	G_assert (__FILE__, __LINE__, "==============table0[0][0] Failed==============", table0[0][0] == "derp");
	G_assert (__FILE__, __LINE__, "==============table0[0][1] Failed==============", table0[0][1] == "herp");
	G_assert (__FILE__, __LINE__, "==============table0[0][2] Failed==============", table0[0][2] == "chirp");
	G_assert (__FILE__, __LINE__, "==============table0[0][3] Failed==============", table0[0][3] == "slurp");
	G_assert (__FILE__, __LINE__, "==============table0[0][4] Failed==============", table0[0][4] == "burp");

	G_assert (__FILE__, __LINE__, "==============table0[1][0] Failed==============", table0[1][0] == "who");
	G_assert (__FILE__, __LINE__, "==============table0[1][1] Failed==============", table0[1][1] == "what");
	G_assert (__FILE__, __LINE__, "==============table0[1][2] Failed==============", table0[1][2] == "when");
	G_assert (__FILE__, __LINE__, "==============table0[1][3] Failed==============", table0[1][3] == "where");
	G_assert (__FILE__, __LINE__, "==============table0[1][4] Failed==============", table0[1][4] == "why");

	shmea::GTable table1;
	table1.addRow(list2);
	table1.addRow(list3);

	shmea::GTable table2;
	table2.addRow(list3);
	table2.addRow(list1);

	shmea::GObject cObj;
	cObj.setMembers(table0);
	cObj.addTable(table1);
	cObj.addTable(table2);

	shmea::GString serializedStr = shmea::Serializable::Serialize(cObj);
	shmea::GObject deserializedObj;
	shmea::Serializable::Deserialize(deserializedObj, serializedStr);

	G_assert (__FILE__, __LINE__, "==============GObject-Deserialize-GList::size() Failed==============", deserializedObj.getMembers()[0].size() == 5);
	G_assert (__FILE__, __LINE__, "=============GObject-Deserialize-=deserializedObj.getMembers()[0][0] Failed==============", deserializedObj.getMembers()[0][0] == "derp");
	G_assert (__FILE__, __LINE__, "=============GObject-Deserialize-=deserializedObj.getMembers()[0][1] Failed==============", deserializedObj.getMembers()[0][1] == "herp");
	G_assert (__FILE__, __LINE__, "=============GObject-Deserialize-=deserializedObj.getMembers()[0][2] Failed==============", deserializedObj.getMembers()[0][2] == "chirp");
	G_assert (__FILE__, __LINE__, "=============GObject-Deserialize-=deserializedObj.getMembers()[0][3] Failed==============", deserializedObj.getMembers()[0][3] == "slurp");
	G_assert (__FILE__, __LINE__, "=============GObject-Deserialize-=deserializedObj.getMembers()[0][4] Failed==============", deserializedObj.getMembers()[0][4] == "burp");
}
