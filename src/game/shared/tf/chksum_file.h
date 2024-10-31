// basic file operations
#include "filesystem.h"

// ( bits / 4 ) + 1 [nullterm]
const size_t MD5_charsize       = ( 128 / 4 ) + 1;
const size_t SHA1_charsize      = ( 160 / 4 ) + 1;
const size_t XXH3_64_charsize   = ( 64  / 4 ) + 1;
const size_t XXH3_128_charsize  = ( 128 / 4 ) + 1;

#pragma once

class CChecksum
{
public:
    static bool             MD5__File       (const char* file, char* outstring, const char* searchpath = "GAME" );
    static bool             SHA1__File      (const char* file, char* outstring, const char* searchpath = "GAME" );
    static bool             XXH3_64__File   (const char* file, char* outstring, const char* searchpath = "GAME" );
    static bool             XXH3_128__File  (const char* file, char* outstring, const char* searchpath = "GAME" );
};