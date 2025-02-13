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
	//
	// Test basic GTable functions
	//
	shmea::GString sampleData( "First,Last,Age\nMickey,Mouse,100\nDonald,Duck,99\n" );
	shmea::GTable sampleDataTable( sampleData, ',', shmea::GTable::TYPE_STRING );
	G_assert(__FILE__, __LINE__, "==============GTable::getNumCols() Failed==============",sampleDataTable.numberOfCols() == 3 );
	G_assert(__FILE__, __LINE__, "==============GTable::getNumRows() Failed==============", sampleDataTable.numberOfRows() == 2 );	

	shmea::GTable copyTable;
	copyTable = sampleDataTable;
	G_assert(__FILE__, __LINE__, "==============GTable::getNumCols() Failed==============", sampleDataTable.numberOfCols() == copyTable.numberOfCols() );
	G_assert(__FILE__, __LINE__, "==============GTable::getNumRows() Failed==============", sampleDataTable.numberOfRows() == copyTable.numberOfRows() );		
	
	shmea::GString header( "First");
	shmea::GList copytableheaders = copyTable[ header ];
	G_assert(__FILE__, __LINE__, "==============GTable::[header] Count Failed==============", copytableheaders.size() == 2 );

	shmea::GList copytabledata = copyTable[1];
	G_assert(__FILE__, __LINE__, "==============GTable::[header] Count Failed==============", copytabledata.size() == 3 );

	//
	// Test file I/O
	//
	shmea::GString fname( "AAPLtestTable.csv" );
	shmea::GString outputFile( "outputTest.csv" );
	shmea::GTable testTable( fname, ',', shmea::GTable::TYPE_FILE );

	G_assert(__FILE__, __LINE__, "==============GTable::getNumCols() Failed==============", testTable.numberOfCols() == 6);
	G_assert(__FILE__, __LINE__, "==============GTable::getNumRows() Failed==============", testTable.numberOfRows() == 500 );

	testTable.save( outputFile );

	shmea::GTable readTable( outputFile, ',', shmea::GTable::TYPE_FILE );
	G_assert(__FILE__, __LINE__, "==============GTable::getNumCols() Comparison Failed==============",  readTable.numberOfCols() == testTable.numberOfCols() );
	G_assert(__FILE__, __LINE__, "==============GTable::getNumRows() Comparison Failed==============", readTable.numberOfRows() == testTable.numberOfRows() );

	// Test  case to handle Invalid Header Access

	shmea::GString invalidHeader("NoHeader");
	shmea::GList invalidHeaderResult = sampleDataTable[invalidHeader];

	// Verify that accessing a non-existent header returns an empty list or handles it gracefully
	G_assert(__FILE__, __LINE__, "==============GTable::Invalid Header Access Failed==============", invalidHeaderResult.size() == 0);


	return;
}
