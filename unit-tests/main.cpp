// Confidential, unpublished property of Robert Carneiro

// The access and distribution of this material is limited solely to
// authorized personnel.  The use, disclosure, reproduction,
// modification, transfer, or transmittal of this work for any purpose
// in any form or by any means without the written permission of
// Robert Carneiro is strictly prohibited.
#include "main.h"
#include "Backend/Database/GType-test.h"
#include "Backend/Database/GString-test.h"
#include "Backend/Database/GList-test.h"
#include "Backend/Database/GTable-test.h"
#include "Backend/Database/GObjects-test.h"

int main(int argc, char* argv[])
{
	GTypeUnitTest();
	GStringUnitTest();
	GStringUnitTest();
	GListUnitTest();
	GTableUnitTest();
	GObjectsUnitTest();

	printf("========================\n");
	printf("| Unit Tests Completed |\n");
	printf("========================\n");

	pthread_exit(EXIT_SUCCESS);
}
