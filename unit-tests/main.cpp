// Confidential, unpublished property of Robert Carneiro

// The access and distribution of this material is limited solely to
// authorized personnel.  The use, disclosure, reproduction,
// modification, transfer, or transmittal of this work for any purpose
// in any form or by any means without the written permission of
// Robert Carneiro is strictly prohibited.
#include "main.h"
#include "Backend/Database/GType-test.h"
#include "Backend/Database/GString-test.h"
#include "Backend/Database/GPointer-test.h"
#include "Backend/Database/GList-test.h"
#include "Backend/Database/GList-edge-test.h"
#include "Backend/Database/GTable-test.h"
#include "Backend/Database/GObjects-test.h"
#include "Backend/Networking/crypt-test.h"
#include "Backend/Networking/connection-test.h"
#include "Backend/Networking/udp-test.h"
#include "Backend/Networking/protocol-fuzz-test.h"
#include "Backend/Networking/socket-integration-test.h"
#include "Backend/Networking/socket-advanced-test.h"
#include "Backend/Database/ServiceData-test.h"
#include "Backend/Database/GVector-test.h"
#include "Backend/Database/image-test.h"
#include "Backend/Plotter/plotter-test.h"

int main(int argc, char* argv[])
{
	if (argc == 1)
	{
	    GTypeUnitTest();
	    GStringUnitTest();
	    GVectorUnitTest();
	    GPointerUnitTest();
	    GListUnitTest();
	    GListEdgeUnitTest();
	    GTableUnitTest();
	    //GObjectsUnitTest();
	    CryptUnitTest();
	    ConnectionUnitTest();
	    UDPUnitTest();
	    ProtocolFuzzUnitTest();
	    SocketIntegrationUnitTest();
	    SocketAdvancedUnitTest();
	    ImageUnitTest();
	    ServiceDataUnitTest();
	    shmea::testCluster();
	    shmea::testHistogram();
	    shmea::testCandlestickChart();
	    shmea::testLineScatter();
	    shmea::testMultiCluster();
	    shmea::testArrows();
	    shmea::testOriginAxes();
	    shmea::testLabeledHistogram();
	}
	else if (argc > 1)
	{
	    if (strcmp(argv[1], "db") == 0)
	    {
		GTypeUnitTest();
		GStringUnitTest();
		GVectorUnitTest();
		GPointerUnitTest();
		GListUnitTest();
		GTableUnitTest();
		GListEdgeUnitTest();
		ServiceDataUnitTest();
	    }
	    else if (strcmp(argv[1], "gnet") == 0)
	    {
		CryptUnitTest();
		ConnectionUnitTest();
		ProtocolFuzzUnitTest();
		SocketIntegrationUnitTest();
		SocketAdvancedUnitTest();
	    }
	    else if (strcmp(argv[1], "udp") == 0)
		UDPUnitTest();
	    else if (strcmp(argv[1], "proto") == 0)
		ProtocolFuzzUnitTest();
	    else if (strcmp(argv[1], "images") == 0)
	    {
		ImageUnitTest();
		shmea::testCluster();
		shmea::testHistogram();
		shmea::testCandlestickChart();
		shmea::testLineScatter();
		shmea::testMultiCluster();
		shmea::testArrows();
		shmea::testOriginAxes();
		shmea::testLabeledHistogram();
	    }
	    else
		printf("Invalid test: %s\n", argv[1]);
	}

	printf("========================\n");
	printf("| Unit Tests Completed |\n");
	printf("========================\n");

	// IMPORTANT:
	// `pthread_exit()` keeps the process alive while other threads exist.
	// For test executables (and GUI apps), that can look like a "hang forever"
	// if libraries (e.g. GLib) have background thread pools.
	return EXIT_SUCCESS;
}
