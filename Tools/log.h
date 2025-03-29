#pragma once
#include <windows.h>
#include <iostream>

// Базовый макрос для цветного вывода
#define LOG_COLOR(text, color_code) { \
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE); \
    SetConsoleTextAttribute(hConsole, color_code); \
    std::cout << text << std::endl; \
    SetConsoleTextAttribute(hConsole, 7); \
}

// Конкретные макросы логирования (используем числа без кавычек)
#define LOG_INFO(text)    LOG_COLOR("[INFO] " << text, 9)    // Синий (Windows)
#define LOG_WARN(text)    LOG_COLOR("[WARN] " << text, 14)   // Жёлтый
#define LOG_ERROR(text)   LOG_COLOR("[ERROR] " << text, 12)  // Красный
#define LOG_SUCCESS(text) LOG_COLOR("[SUCCESS] " << text, 10) // Зелёный