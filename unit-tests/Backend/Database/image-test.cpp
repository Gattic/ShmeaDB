// Confidential, unpublished property of Robert Carneiro

// The access and distribution of this material is limited solely to
// authorized personnel.  The use, disclosure, reproduction,
// modification, transfer, or transmittal of this work for any purpose
// in any form or by any means without the written permission of
// Robert Carneiro is strictly prohibited.
#include "image-test.h"
#include "../../unit-test.h"
#include "../../../Backend/Database/image.h"
#include "../../../Backend/Database/png-helper.h"
#include <fstream>

// This File will have the more advanced functionalities of GPointer
// For simpler tests check GList-test.cpp

// === This is the primary unit testing function:
// void G_assert(const char* fileName, int lineNo, const char* failureMsg, bool expr)

using namespace shmea;

void ImageUnitTest()
{
	// Save then load a png to test
	printf("------\n");
	printf("PNG Unit Tests\n");
	printf("------\n");
	PNGHelper::createTestPNG("hello/out-png-test.png");

	// Check if file exists
	std::ifstream file("hello/out-png-test.png");
	G_assert(__FILE__, __LINE__, "image.save failed", file.good());
	
	Image image;
	PNGHelper::LoadPNG(image, "hello/out-png-test.png");

	const unsigned width = 1200; // taken from png-helper.h createTestPNG fnc
	const unsigned height = 800; // taken from png-helper.h createTestPNG fnc
	G_assert(__FILE__, __LINE__, "image.width failed", image.getWidth() == width);
	G_assert(__FILE__, __LINE__, "image.height failed", image.getHeight() == height);
}
