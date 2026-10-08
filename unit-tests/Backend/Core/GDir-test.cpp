#include "GDir-test.h"
#include "../../../Backend/Core/GDir.h"
#include "../../unit-test.h"
#include <stdio.h>
#include <string>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

using namespace shmea;

void GDirUnitTest()
{
    printf("=== GDir Unit Tests ===\n");

    // Test exists() on a known directory
    {
        bool found = GDir::exists("datasets");
        ASSERT("datasets directory exists", found);
    }

    // Test open() + next() iterates entries
    {
        GDir dir;
        bool opened = dir.open("datasets");
        ASSERT("GDir opens datasets/", opened);
        ASSERT("GDir isOpen after open", dir.isOpen());

        int count = 0;
        GDirEntry ent;
        while (dir.next(ent))
        {
            ASSERT("Entry name is not empty", !ent.name.empty());
            ++count;
        }
        ASSERT("datasets/ has at least one entry", count > 0);
    }

    // Test makeDir() + exists()
    {
        std::string testDir = "datasets/gdir_test_tmp";
        bool made = GDir::makeDir(testDir);
        ASSERT("makeDir creates directory", made);
        ASSERT("exists() finds new directory", GDir::exists(testDir));
        // Cleanup
#ifdef _WIN32
        RemoveDirectoryA(testDir.c_str());
#else
        rmdir(testDir.c_str());
#endif
    }

    printf("GDir tests passed.\n");
}
