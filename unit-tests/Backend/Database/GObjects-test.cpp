// Confidential, unpublished property of Robert Carneiro

// The access and distribution of this material is limited solely to
// authorized personnel.  The use, disclosure, reproduction,
// modification, transfer, or transmittal of this work for any purpose
// in any form or by any means without the written permission of
// Robert Carneiro is strictly prohibited.
#include "GObjects-test.h"
#include "../../unit-test.h"
#include "../../../Backend/Database/GObject.h"
#include "../../../Backend/Database/GString.h"
#include "../../../Backend/Database/Serializable.h"

// === This is the primary unit testing function:
// void G_assert(const char* fileName, int lineNo, const char* failureMsg, bool expr)

void GObjectsUnitTest()
{
	shmea::GString str = "Test123!";
	G_assert (__FILE__, __LINE__, "==============GString Failed==============", strcmp(str, "Test123!") == 0);
	/*printf("GStr: %s\n", str.c_str());
	for(unsigned int i=0;i<str.length();++i)
		printf("GStr[%d]: 0x%02X:%c\n", i, str[i], str[i]);*/
	str = "abc" + str;
	G_assert (__FILE__, __LINE__, "==============GString::operator+ Failed==============", str == "abcTest123!");
	str += "?!";
	G_assert (__FILE__, __LINE__, "==============GString::operator+= Failed==============", str == "abcTest123!?!");
	str = str + "!";
	G_assert (__FILE__, __LINE__, "==============GString::operator+ Failed==============", str == "abcTest123!?!!");
	G_assert (__FILE__, __LINE__, "==============GString::substr Failed==============", str.substr(3) == "Test123!?!!");
	G_assert (__FILE__, __LINE__, "==============GString::substr Failed==============", str.substr(3, 4) == "Test");

	shmea::GList list0;
	list0.addString("derp");
	list0.addString("herp");
	list0.addString("chirp");
	list0.addString("slurp");
	list0.addString("burp");

	G_assert (__FILE__, __LINE__, "==============GList::size() Failed==============", list0.size() == 5);
	G_assert (__FILE__, __LINE__, "==============list0[0] Failed==============", strcmp(list0[0], "derp") == 0);
	G_assert (__FILE__, __LINE__, "==============list0[1] Failed==============", strcmp(list0[1], "herp") == 0);
	G_assert (__FILE__, __LINE__, "==============list0[2] Failed==============", strcmp(list0[2], "chirp") == 0);
	G_assert (__FILE__, __LINE__, "==============list0[3] Failed==============", strcmp(list0[3], "slurp") == 0);
	G_assert (__FILE__, __LINE__, "==============list0[4] Failed==============", strcmp(list0[4], "burp") == 0);

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

	shmea::GString cStr = shmea::Serializable::Serialize(cObj);
	shmea::GObject newObj;
	shmea::Serializable::Deserialize(newObj, cStr);
	printf("========================\n");
	printf("| Unit Tests Completed |\n");
	printf("========================\n");
}
