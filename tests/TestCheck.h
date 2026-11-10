#pragma once
#include <cstdio>
#include <cstdlib>

// Always-on regression requirement: never compiled out in Release / NDEBUG.
// Avoids conflicting /DNDEBUG and /UNDEBUG compiler flags in MSVC builds.
#define REQUIRE(condition) do { \
    if (!(condition)) { \
        std::fprintf(stderr, "%s:%d: REQUIRE failed: %s\n", __FILE__, __LINE__, #condition); \
        std::abort(); \
    } \
} while (false)
