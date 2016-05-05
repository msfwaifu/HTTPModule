/*
    Initial author: (https://github.com/)Convery for Ayria.se
    License: LGPL 3.0
    Started: 2016-04-13
    Notes:
        Create a new string from variadic arguments.
        va("%i hedgehogs!", 42) == "42 hedgehogs!" 
*/

#pragma once

// While it's preferred to use a specialized version, va()
// will call the correct version with some performance loss.
const char *va(const char *fmt, ...);
const char *va_small(const char *fmt, ...);
const char *va_large(const char *fmt, ...);
