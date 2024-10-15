#include "util.h"

// Function to convert a char* (UTF-8) to std::wstring (UTF-16)
std::wstring to_wstring(const char* str) {
    std::mbstate_t state = std::mbstate_t();
    std::size_t len = 1 + std::mbsrtowcs(nullptr, &str, 0, &state); // Get length required
    std::wstring wstr(len, L'\0');  // Reserve space for wide string

    std::mbsrtowcs(&wstr[0], &str, len, &state); // Convert to wide string
    return wstr;
}