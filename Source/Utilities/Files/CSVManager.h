/*
    Initial author: (https://github.com/)Convery for Ayria.se
    License: LGPL 3.0
    Started: 2016-4-20
    Notes:
        Reading and writing of CSV files.
*/

#pragma once
#include <string>
#include <vector>

struct CSVManager
{
    static bool Readfile(const char *Filepath);
    static bool Writefile(const char *Filepath);
    static std::string Getvalue(size_t Row, size_t Col);
    static std::vector<std::vector<std::string>> EntryBuffer;
};
