#include "GDir.h"
#include <string.h>

#ifdef _WIN32
    #include <direct.h>
#else
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <unistd.h>
#endif

namespace shmea {

GDir::GDir() : valid(false)
{
#ifdef _WIN32
    hFind     = INVALID_HANDLE_VALUE;
    firstDone = false;
    memset(&findData, 0, sizeof(findData));
#else
    dir   = NULL;
    entry = NULL;
#endif
}

GDir::~GDir()
{
    close();
}

bool GDir::open(const std::string& path)
{
    close();
#ifdef _WIN32
    std::string pattern = path + "\\*";
    hFind = FindFirstFileA(pattern.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE)
        return false;
    firstDone = false;
#else
    dir = opendir(path.c_str());
    if (!dir)
        return false;
    entry = NULL;
#endif
    valid = true;
    return true;
}

bool GDir::next(GDirEntry& out)
{
    if (!valid)
        return false;

#ifdef _WIN32
    for (;;)
    {
        if (!firstDone)
        {
            firstDone = true;
        }
        else
        {
            if (!FindNextFileA(hFind, &findData))
                return false;
        }
        if (strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0)
            continue;
        out.name        = findData.cFileName;
        out.isDirectory = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        return true;
    }
#else
    for (;;)
    {
        entry = readdir(dir);
        if (!entry)
            return false;
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        out.name        = entry->d_name;
        out.isDirectory = (entry->d_type == DT_DIR);
        return true;
    }
#endif
}

void GDir::close()
{
#ifdef _WIN32
    if (hFind != INVALID_HANDLE_VALUE)
    {
        FindClose(hFind);
        hFind = INVALID_HANDLE_VALUE;
    }
    firstDone = false;
#else
    if (dir)
    {
        closedir(dir);
        dir   = NULL;
        entry = NULL;
    }
#endif
    valid = false;
}

bool GDir::isOpen() const
{
    return valid;
}

bool GDir::exists(const std::string& path)
{
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat info;
    return (stat(path.c_str(), &info) == 0) && (info.st_mode & S_IFDIR);
#endif
}

bool GDir::makeDir(const std::string& path)
{
#ifdef _WIN32
    return CreateDirectoryA(path.c_str(), NULL) != 0;
#else
    return mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == 0;
#endif
}

} // namespace shmea
