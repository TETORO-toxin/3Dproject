#pragma once
#include <string>
#include <iostream>
#include <cstdio>
#ifdef _WIN32
#include <windows.h>
#endif

static inline void LogMsg(const std::string &s)
{
    std::cout << s << std::endl;
    std::cerr << s << std::endl;
    printf("%s\n", s.c_str());
    fflush(stdout);
    fflush(stderr);
#ifdef _WIN32
    FILE* f = nullptr;
    if (fopen_s(&f, "enemy_log.txt", "a") == 0 && f) {
        fprintf(f, "%s\n", s.c_str());
        fflush(f);
        fclose(f);
    }
#else
    FILE* f = fopen("enemy_log.txt", "a");
    if (f) {
        fprintf(f, "%s\n", s.c_str());
        fflush(f);
        fclose(f);
    }
#endif
#ifdef _WIN32
    OutputDebugStringA((s + "\n").c_str());
#endif
}
