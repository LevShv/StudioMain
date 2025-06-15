#pragma once
#include <windows.h>
#include <iostream>

#define LOG(text)  do { std::cout << text << std::endl; } while (0)

#define LOG_COLOR(text, color_code) { \
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE); \
    SetConsoleTextAttribute(hConsole, color_code); \
    LOG(text); \
    SetConsoleTextAttribute(hConsole, 7); \
}

#define LOG_INFO(text)    LOG_COLOR("[INFO] " << text, 9)
#define LOG_WARN(text)    LOG_COLOR("[WARN] " << text, 14)
#define LOG_ERROR(text)   LOG_COLOR("[ERROR] " << text, 12) 
#define LOG_SUCCESS(text) LOG_COLOR("[SUCCESS] " << text, 10)