/*
    Initial author: (https://github.com/)Convery for Ayria.se
    License: LGPL 3.0
    Started: 2016-04-13
    Notes:
        A basic system for file operations.
        MSVC makes use of C++17 TS.
*/

#pragma once
#include <string>
#include <vector>

struct Filesystem
{
    static bool Writefile(const char *Filepath, const void *Databuffer, const size_t Datalength, const bool Append);
    static bool Writefile(const char *Filepath, const std::string &Databuffer, const bool Append);
    static bool Readfile(const char *Filepath, void *Databuffer, size_t *Datalength);
    static bool Readfile(const char *Filepath, std::string *Databuffer);

    static uint32_t Modified(const char *Filepath);
    static size_t Filesize(const char *Filepath);
    static bool Fileexists(const char *Filepath);
    static bool Createdir(const char *Path);

    static bool Searchdir(std::string Searchpath, std::vector<std::string> *Filenames, const char *Extension = nullptr);
    static bool Searchdirrecursive(std::string Searchpath, std::vector<std::string> *Filenames, const char *Extension = nullptr);
};
