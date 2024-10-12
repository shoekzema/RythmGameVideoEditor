#pragma once

#include <cstdint>
#include <vector>

struct Frame {
    int width;
    int height;
    std::vector<uint8_t> data;  // Stores pixel data
    int linesize;               // Number of bytes per line

    Frame(int w, int h, int line_size)
        : width(w), height(h), linesize(line_size), data(w* h * 3) {}  // Assuming RGB (3 bytes per pixel)
};
