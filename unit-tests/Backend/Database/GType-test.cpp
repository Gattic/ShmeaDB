// Confidential, unpublished property of Robert Carneiro

// The access and distribution of this material is limited solely to
// authorized personnel.  The use, disclosure, reproduction,
// modification, transfer, or transmittal of this work for any purpose
// in any form or by any means without the written permission of
// Robert Carneiro is strictly prohibited.
#include "GType-test.h"
#include "../../unit-test.h"
#include "../../../Backend/Database/GType.h"

// === This is the primary unit testing function:
// void G_assert(const char* fileName, int lineNo, const char* failureMsg, bool expr)

void GTypeUnitTest()
{
	shmea::GType gInt = 123;
	shmea::GType gInt2 = 123;
	shmea::GType gInt3 = 93453;
	shmea::GType gInt4 = 1;
	G_assert (__FILE__, __LINE__, "==============GType-int Failed==============", gInt.getType() == shmea::GType::INT_TYPE);
	G_assert (__FILE__, __LINE__, "==============GType-int Failed==============", gInt == 123);
	G_assert (__FILE__, __LINE__, "==============GType-int Failed==============", gInt != 25);
	G_assert (__FILE__, __LINE__, "==============GType-int Failed==============", gInt == gInt2);
	G_assert (__FILE__, __LINE__, "==============GType-int Failed==============", gInt != gInt3);

	shmea::GType gFloat = 14.3f;
	shmea::GType gFloat2 = 831.3f;
	shmea::GType gFloat3 = 1.0f;
	shmea::GType gFloat4 = 14.3f;
	G_assert (__FILE__, __LINE__, "==============GType-float Failed==============", gFloat.getType() == shmea::GType::FLOAT_TYPE);
	G_assert (__FILE__, __LINE__, "==============GType-float Failed==============", gFloat == 14.3f);
	G_assert (__FILE__, __LINE__, "==============GType-float Failed==============", gFloat != 25.0);
	G_assert (__FILE__, __LINE__, "==============GType-float Failed==============", gFloat == gFloat4);
	G_assert (__FILE__, __LINE__, "==============GType-float Failed==============", gFloat != gFloat3);
	G_assert (__FILE__, __LINE__, "==============GType-float-int Failed==============", gFloat3 == 1);

	shmea::GType gDouble = 14.3;
	shmea::GType gDouble2 = 654.856;
	shmea::GType gDouble3 = 1.43557;
	shmea::GType gDouble4 = 1.43557;
	G_assert (__FILE__, __LINE__, "==============GType-double Failed==============", gDouble.getType() == shmea::GType::DOUBLE_TYPE);
	G_assert (__FILE__, __LINE__, "==============GType-double Failed==============", gDouble == 14.3f);
	G_assert (__FILE__, __LINE__, "==============GType-double Failed==============", gDouble != 25);
	G_assert (__FILE__, __LINE__, "==============GType-double Failed==============", gDouble3 == gDouble4);
	G_assert (__FILE__, __LINE__, "==============GType-double Failed==============", gDouble != gDouble3);
	G_assert (__FILE__, __LINE__, "==============GType-float-double Failed==============", gFloat != gDouble); // Different precision! Use an EPSILON value for comparison then make another test case

	shmea::GType gChar = 'a';
	shmea::GType gChar2 = '&';
	G_assert (__FILE__, __LINE__, "==============GType-int Failed==============", gChar.getType() == shmea::GType::CHAR_TYPE);
	G_assert (__FILE__, __LINE__, "==============GType-int Failed==============", gChar == 'a');
	G_assert (__FILE__, __LINE__, "==============GType-int Failed==============", gChar2 != 'B');
	G_assert (__FILE__, __LINE__, "==============GType-int Failed==============", gChar != gChar2);
}
