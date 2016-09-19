/*
    Initial author: (https://github.com/)Convery for Ayria.se
    License: LGPL 3.0
    Started: 2016-04-13
    Notes:
        A basic system for file operations.
        MSVC makes use of C++17 TS.
*/

#include "Filesystem.h"
#include <algorithm>
#include <fstream>
#include <thread>
#include <mutex>

// MSVC++ 14.0
#if _MSC_VER >= 1900
#include <experimental\filesystem>

uint32_t Filesystem::Modified(const char *Filepath)
{
    // Sanity checking to avoid time-wastage.
    if (!Fileexists(Filepath)) return 0;

    auto Filetime = std::experimental::filesystem::last_write_time(Filepath);
    return uint32_t(decltype(Filetime)::clock::to_time_t(Filetime));
}
size_t Filesystem::Filesize(const char *Filepath)
{
    return size_t(std::experimental::filesystem::file_size(Filepath));
}
bool Filesystem::Fileexists(const char *Filepath)
{
    return std::experimental::filesystem::exists(Filepath);
}
bool Filesystem::Createdir(const char *Path)
{
    return std::experimental::filesystem::create_directory(Path);
}
#else

size_t Filesystem::Filesize(const char *Filepath)
{
    std::ifstream Filehandle(Filepath, std::ios::binary);
    std::streamsize Size = 0;

    Filehandle.seekg(0, std::ios::end);
    Size = Filehandle.tellg();
    Filehandle.seekg(0, std::ios::beg);

    if (Size == -1)
        return 0;
    else
        return size_t(Size);
}
bool Filesystem::Fileexists(const char *Filepath)
{
    return Filesize(Filepath) != 0;
}
#ifdef _WIN32
#include <direct.h>
#include <Windows.h>
#undef min
#undef max

uint32_t Filesystem::Modified(const char *Filepath)
{
    HANDLE FileHandle = CreateFileA(Filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    FILETIME ModifiedTime;

    GetFileTime(FileHandle, NULL, NULL, &ModifiedTime);
    CloseHandle(FileHandle);

    return uint32_t((*(uint64_t *)&ModifiedTime / 10000000 - 11644473600LL));
}
bool Filesystem::Createdir(const char *Path)
{
    return _mkdir(Path);
}
#else
#include <sys/types.h>
#include <sys/stat.h>

uint32_t Filesystem::Modified(const char *Filepath)
{
    struct stat Attribute;
    stat(Filepath, &Attribute);
    return uint32_t(Attribute.st_mtime);
}
bool Filesystem::Createdir(const char *Path)
{
    return mkdir(Path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0;
}
#endif
#endif 

bool Filesystem::Writefile(const char *Filepath, const void *Databuffer, const size_t Datalength, const bool Append)
{
    bool Result;
    std::ofstream Filewriter(Filepath, std::ios::binary | (Append ? std::ios::app : 0));
    Result = !!Filewriter.write((const char *)Databuffer, Datalength);
    Filewriter.close();
    return Result;
}
bool Filesystem::Writefile(const char *Filepath, const std::string &Databuffer, const bool Append)
{
    return Writefile(Filepath, Databuffer.data(), Databuffer.size(), Append);
}
bool Filesystem::Readfile(const char *Filepath, void *Databuffer, size_t *Datalength)
{
    // Sanity checking to avoid time-wastage.
    if (!Fileexists(Filepath)) return false;

    // Open the file.
    size_t Readsize = std::min(Filesize(Filepath), *Datalength);
    std::ifstream Filereader(Filepath, std::ios::binary);

    // Read the file into the buffer.
    bool Result;
    Result = !!Filereader.read((char *)Databuffer, Readsize);

    // Set the read length and cleanup.
    *Datalength = Readsize;
    Filereader.close();
    return Result;
}
bool Filesystem::Readfile(const char *Filepath, std::string *Databuffer)
{
    // Sanity checking to avoid time-wastage.
    if (!Fileexists(Filepath)) return false;

    // Read the full file using the raw method.
    bool Result;
    size_t Filelength = Filesize(Filepath);
    char *Filebuffer = new char[Filelength]();
    Result = Readfile(Filepath, Filebuffer, &Filelength);

    // Append to the string.
    Databuffer->append(Filebuffer, Filelength);

    delete[] Filebuffer;
    return Result;
}

#ifdef _WIN32
#include <Windows.h>
#undef min
#undef max

bool Filesystem::Searchdir(std::string Searchpath, std::vector<std::string> *Filenames, const char *Extension)
{
    WIN32_FIND_DATA FileData;
    HANDLE FileHandle;

    // Append trailing slash, asterisk and extension.
    if (Searchpath.back() != '\\') Searchpath.append("\\");
    Searchpath.append("*");
    if(Extension) Searchpath.append(".");
    if(Extension) Searchpath.append(Extension);

    // Find the first extension.
    FileHandle = FindFirstFileA(Searchpath.c_str(), &FileData);
    if (FileHandle == (void *)ERROR_INVALID_HANDLE || FileHandle == (void *)INVALID_HANDLE_VALUE)
        return false;

    do
    {
        // Respect hidden files.
        if(FileData.cFileName[0] != '.')
            Filenames->push_back(FileData.cFileName);
    } while (FindNextFileA(FileHandle, &FileData));

    return !!Filenames->size();
}
bool Filesystem::Searchdirrecursive(std::string Searchpath, std::vector<std::string> *Filenames, const char *Extension)
{
    static std::mutex Threadguard;
    std::vector<std::thread> Workers;
    WIN32_FIND_DATA Filedata;
    HANDLE FileHandle;
    std::string Path;

    // Append trailing slash, asterisk and extension.
    if (Searchpath.back() != '\\') Searchpath.append("\\");
    Path = Searchpath;
    Searchpath.append("*");
    if(Extension) Searchpath.append(".");
    if(Extension) Searchpath.append(Extension);

    // Find the first extension.
    FileHandle = FindFirstFileA(Searchpath.c_str(), &Filedata);
    if (FileHandle == (void *)ERROR_INVALID_HANDLE || FileHandle == (void *)INVALID_HANDLE_VALUE)
        return false;

    do
    {
        // Respect hidden files and folders.
        if (Filedata.cFileName[0] == '.') continue;

        // Start a new thread for directories.
        if (Filedata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            Workers.push_back(std::thread(Searchdirrecursive, Path + Filedata.cFileName, Filenames, Extension));
            continue;
        }

        // Append the full path.
        Threadguard.lock();
        {
            Filenames->push_back(Path + Filedata.cFileName);
        }
        Threadguard.unlock();

    } while (FindNextFileA(FileHandle, &Filedata));

    // Wait for all threads to finish.
    for (std::thread &Thread : Workers)
        if (Thread.joinable())
            Thread.join();

    return !!Filenames->size();
}
#else
bool Filesystem::Searchdir(std::string Searchpath, std::vector<std::string> *Filenames, const char *Extension)
{
    /* 
        TODO(Convery): 
        Implement a nix version when we need it.
    */
    return false;
}
bool Filesystem::Searchdirrecursive(std::string Searchpath, std::vector<std::string> *Filenames, const char *Extension)
{
    /* 
        TODO(Convery): 
        Implement a nix version when we need it.
    */
    return false;
}
#endif
