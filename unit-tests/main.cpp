// Confidential, unpublished property of Robert Carneiro

// The access and distribution of this material is limited solely to
// authorized personnel.  The use, disclosure, reproduction,
// modification, transfer, or transmittal of this work for any purpose
// in any form or by any means without the written permission of
// Robert Carneiro is strictly prohibited.
#include "main.h"
#include "Backend/Database/GObjects-test.h"

int main(int argc, char* argv[])
{
	GObjectsUnitTest();
	pthread_exit(EXIT_SUCCESS);
}
