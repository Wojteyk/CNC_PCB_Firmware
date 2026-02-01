#pragma once
#include <stdint.h>
#include "common/systemError.hpp"
#include "common/lcdConstants.hpp"

class IGuiDriver {
public:
    virtual ~IGuiDriver() = default;
    virtual Result<void> fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) = 0;
    virtual Result<void> drawString(int16_t x, int16_t y, const char* str, uint16_t color, uint16_t bg) = 0;
    virtual Result<void> fillScreen(uint16_t color) = 0;
};