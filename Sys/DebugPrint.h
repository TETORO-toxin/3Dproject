#pragma once
#include <windows.h>
#include <cstdarg>
#include <cstdio>

inline void DebugPrint(const char* fmt, ...) {
    char buf[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    OutputDebugStringA(buf);
    // also append to a log file so output is visible even without debugger attached
    FILE* f = nullptr;
    fopen_s(&f, "debug.log", "a");
    if (f) {
        fprintf(f, "%s", buf);
        fclose(f);
    }
}
