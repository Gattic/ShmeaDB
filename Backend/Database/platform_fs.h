#pragma once
#include <vector>
#include <string>

#ifdef _WIN32
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include <direct.h>     // _mkdir
  #include <sys/stat.h>   // _stat
  #include <io.h>         // _findfirst, _findnext
  #include <errno.h>

  inline bool shmea_is_dir(const char* p)
  {
      struct _stat info;
      if (_stat(p, &info) != 0) return false;
      return (info.st_mode & _S_IFDIR) != 0;
  }

  inline int shmea_mkdir(const char* p)
  {
      return _mkdir(p); // 0 = ok, -1 = fail (errno set)
  }

  inline std::vector<std::string> shmea_list_dir(const std::string& folder)
  {
      std::vector<std::string> out;

      std::string pattern = folder;
      if (!pattern.empty() && pattern.back() != '\\' && pattern.back() != '/')
          pattern += "\\*";
      else
          pattern += "*";

      struct _finddata_t fd;
      intptr_t h = _findfirst(pattern.c_str(), &fd);
      if (h == -1)
          return out;

      do {
          const char* name = fd.name;
          if (!name || name[0] == '\0') continue;
          if (name[0] == '.') continue; // skips ".", "..", and hidden dotfiles

          out.push_back(name);
      } while (_findnext(h, &fd) == 0);

      _findclose(h);
      return out;
  }

#else
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <unistd.h>
  #include <dirent.h>
  #include <errno.h>

  inline bool shmea_is_dir(const char* p)
  {
      struct stat info;
      if (stat(p, &info) != 0) return false;
      return S_ISDIR(info.st_mode);
  }

  inline int shmea_mkdir(const char* p)
  {
      return ::mkdir(p, 0777); // perms obey umask
  }

  inline std::vector<std::string> shmea_list_dir(const std::string& folder)
  {
      std::vector<std::string> out;

      DIR* dir = opendir(folder.c_str());
      if (!dir)
          return out;

      struct dirent* ent = NULL;
      while ((ent = readdir(dir)) != NULL)
      {
          const char* name = ent->d_name;
          if (!name || name[0] == '\0') continue;
          if (name[0] == '.') continue; // skips ".", "..", and dotfiles

          out.push_back(name);
      }

      closedir(dir);
      return out;
  }
#endif
