// Confidential, unpublished property of Robert Carneiro

// The access and distribution of this material is limited solely to
// authorized personnel.  The use, disclosure, reproduction,
// modification, transfer, or transmittal of this work for any purpose
// in any form or by any means without the written permission of
// Robert Carneiro is strictly prohibited.
#include "crypt-test.h"
#include "../../unit-test.h"
#include "../../../Backend/Database/GString.h"
#include "../../../Backend/Database/GList.h"
#include "../../../Backend/Database/Serializable.h"
#include "../../../Backend/Database/ServiceData.h"
#include "../../../Backend/Networking/crypt.h"

// === This is the primary unit testing function:
// void G_assert(const char* fileName, int lineNo, const char* failureMsg, bool expr)

void CryptUnitTest()
{
	// Test Encryption with Serialization
	shmea::GTable decimalData = shmea::GTable("testTable.csv", ',', shmea::GTable::TYPE_FILE);
	shmea::GList mcArgs;
	mcArgs.addString("MONTE-CARLO");

	GNet::Connection* cConnection = NULL;
	shmea::GPointer<shmea::ServiceData> mcData(new shmea::ServiceData(cConnection, "GUI_Callback"));
	mcData->set(decimalData);
	mcData->assignServiceNum(); // We usually call this in writeConnection
	mcData->setArgList(mcArgs);

	// Serialize
	shmea::GString serializedStr = shmea::Serializable::Serialize(mcData);

	// Encrypt
	int64_t key = 123;
	GNet::Crypt crypt;
	crypt.encrypt(serializedStr.c_str(), key, serializedStr.length());
	G_assert (__FILE__, __LINE__, "==============Crypt-encrpyt Failed==============", crypt.error == 0);

	// Decrypt
	GNet::Crypt cryptUndo;
	cryptUndo.decrypt((int64_t*)crypt.eText.c_str(), key, crypt.eText.length()/8);
	G_assert (__FILE__, __LINE__, "==============Crypt-decrpyt Failed==============", cryptUndo.error == 0);
	G_assert (__FILE__, __LINE__, "==============Crypt-decrpyt Failed==============", crypt.sizeCurrent == crypt.sizeClaimed);

	// Deserialize
	shmea::GPointer<shmea::ServiceData> deserializedCD(new shmea::ServiceData(cConnection, shmea::GString("ServiceNameHere")));
	shmea::Serializable::Deserialize(deserializedCD, cryptUndo.dText);
	shmea::GTable deserializedTable = deserializedCD->getTable();
	//deserializedTable.print();




}
