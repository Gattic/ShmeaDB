// Confidential, unpublished property of Robert Carneiro
//
// The access and distribution of this material is limited solely to
// authorized personnel.  The use, disclosure, reproduction,
// modification, transfer, or transmittal of this work for any purpose
// in any form or by any means without the written permission of
// Robert Carneiro is strictly prohibited.
#include "GList-edge-test.h"
#include "../../unit-test.h"

#include "../../../Backend/Database/GList.h"
#include "../../../Backend/Database/GString.h"
#include "../../../Backend/Database/GType.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Backend/Core/platform.h"

namespace {
static shmea::GString write_temp_file(const char* contents)
{
#ifdef _WIN32
	char tmpDir[MAX_PATH];
	char path[MAX_PATH];
	GetTempPathA(MAX_PATH, tmpDir);
	GetTempFileNameA(tmpDir, "shmea", 0, path);
	FILE* f = fopen(path, "wb");
	ASSERT("fopen failed", f != NULL);
	fwrite(contents, 1, strlen(contents), f);
	fclose(f);
	return shmea::GString(path);
#else
	char path[] = "/tmp/shmeadb_ut_words_XXXXXX";
	int fd = mkstemp(path);
	ASSERT("mkstemp failed", fd >= 0);
	size_t n = strlen(contents);
	ssize_t w = write(fd, contents, n);
	(void)w;
	close(fd);
	return shmea::GString(path);
#endif
}

static void remove_file(const shmea::GString& p)
{
	if (p.length() > 0)
		remove(p.c_str());
}

static void GList_LoadWords_EmptyAndMissing()
{
	shmea::GList a;
	a.loadWords("");
	ASSERT("Empty filename should not load", a.size() == 0);

	shmea::GList b;
#ifdef _WIN32
	b.loadWords("C:/nonexistent_shmeadb_test_42b4e0c9.txt");
#else
	b.loadWords("/tmp/this_file_should_not_exist_42b4e0c9.txt");
#endif
	ASSERT("Missing file should not load", b.size() == 0);
}

static void GList_LoadWords_CRLF_And_Punctuation()
{
	shmea::GString p = write_temp_file("Hello, WORLD!\r\nthis\tis  a-test\n");
	shmea::GList w;
	w.loadWords(p);
	remove_file(p);

	ASSERT("Should tokenize into words", w.size() >= 4);
	ASSERT("hello normalized", w.getString(0) == "hello");
	ASSERT("world normalized", w.getString(1) == "world");
	ASSERT("this normalized", w.getString(2) == "this");
	ASSERT("is normalized", w.getString(3) == "is");
}

static void GList_Standardize_AllSame_NoOp()
{
	shmea::GList l;
	l.addInt(5);
	l.addInt(5);
	l.addInt(5);

	l.standardize();

	ASSERT("Type preserved when xRange==0", l.getType(0) == shmea::GType::INT_TYPE);
	ASSERT("Value preserved when xRange==0", l.getInt(0) == 5);
}

static void GList_Standardize_ScalesToCenteredRange()
{
	shmea::GList l;
	l.addInt(0);
	l.addInt(10);

	l.standardize();

	ASSERT("standardize converts numeric cells to float", l.getType(0) == shmea::GType::FLOAT_TYPE);
	ASSERT("standardize converts numeric cells to float", l.getType(1) == shmea::GType::FLOAT_TYPE);

	float a = l.getFloat(0);
	float b = l.getFloat(1);
	ASSERT("min maps near -0.5", (a > -0.5001f) && (a < -0.4999f));
	ASSERT("max maps near +0.5", (b > 0.4999f) && (b < 0.5001f));
}
} // namespace

void GListEdgeUnitTest()
{
	GList_LoadWords_EmptyAndMissing();
	GList_LoadWords_CRLF_And_Punctuation();
	GList_Standardize_AllSame_NoOp();
	GList_Standardize_ScalesToCenteredRange();
}

