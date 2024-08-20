// Confidential, unpublished property of Robert Carneiro

// The access and distribution of this material is limited solely to
// authorized personnel.  The use, disclosure, reproduction,
// modification, transfer, or transmittal of this work for any purpose
// in any form or by any means without the written permission of
// Robert Carneiro is strictly prohibited.
#include "GTable-test.h"
#include "../../unit-test.h"
#include "../../../Backend/Database/GTable.h"
#include "../../../Backend/Database/GString.h"
#include "../../../Backend/Database/GObject.h"

// This File will have the more advanced functionalities of GTable
// For simpler tests check GList-test.cpp

// === This is the primary unit testing function:
// void G_assert(const char* fileName, int lineNo, const char* failureMsg, bool expr)

void GTableUnitTest()
{
	shmea::GString fname( "AAPLtestTable.csv" );
	shmea::GString outputFile( "outputTest.csv" );
	shmea::GTable testTable( fname, ',', shmea::GTable::TYPE_FILE );

	G_assert(__FILE__, __LINE__, "==============GTable::getNumCols() Failed==============",  testTable.numberOfCols() == 6);
	G_assert(__FILE__, __LINE__, "==============GTable::getNumRows() Failed==============", testTable.numberOfRows() == 500 );

	//testTable.print();
	testTable.save( outputFile );

	shmea::GTable readTable( outputFile, ',', shmea::GTable::TYPE_FILE );
	G_assert(__FILE__, __LINE__, "==============GTable::getNumCols() Comparison Failed==============",  readTable.numberOfCols() == testTable.numberOfCols() );
	G_assert(__FILE__, __LINE__, "==============GTable::getNumRows() Comparison Failed==============", readTable.numberOfRows() == testTable.numberOfRows() );
}
