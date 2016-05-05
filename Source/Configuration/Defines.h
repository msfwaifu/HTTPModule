/*
    Initial author: (https://github.com/)Convery for Ayria.se
    License: LGPL 3.0
    Started: 2016-04-13
    Notes:
        This file declares global defines for the extension.
*/

#pragma once

// As Visual Studio 2015 no longer defines NDEBUG, we'll have to.
#ifdef _MSC_BUILD
#ifndef _DEBUG
#define NDEBUG
#endif
#endif

// Platform specific visibility attributes.
#ifdef __linux__
#define EXPORT_ATTR __attribute__((visibility("default")))
#define IMPORT_ATTR
#elif _WIN32
#define EXPORT_ATTR __declspec(dllexport)
#define IMPORT_ATTR __declspec(dllimport)
#else
#define EXPORT_ATTR
#define IMPORT_ATTR
#pragma warning Unknown dynamic link import/export semantics.
#endif

// Compiling for x64 check.
#if _WIN32
#if _WIN64
#define ENVIRONMENT64
#endif
#endif
#if __GNUC__
#if __x86_64__ || __ppc64__
#define ENVIRONMENT64
#endif
#endif

// Logging functions save to dir and duplicate to stdout.
#define LOGFILEDIR ".\\Plugins\\Logs\\"
#define DEBUGTOSTREAM

// The extension name that will be used throughout the application.
#define EXTENSIONNAME "HTTPSModule"

// The authors on windows should be able to check basic ranges.
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#endif
