#pragma once
#include <stdint.h>
#include "common/systemError.hpp"
#include "common/lcdConstants.hpp"

/**
 * @file iGuiDriver.hpp
 * @brief Generic GUI drawing interface used by views and widgets.
 */

/**
 * @brief Abstract GUI drawing interface.
 */
class IGuiDriver {
public:
    virtual ~IGuiDriver() = default;

    /**
     * @brief Fill rectangle with color.
     * @param x Left coordinate.
     * @param y Top coordinate.
     * @param w Rectangle width.
     * @param h Rectangle height.
     * @param color RGB565 color.
     * @return Operation status.
     */
    virtual Result<void> fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) = 0;

    /**
     * @brief Draw text string.
     * @param x Text baseline X.
     * @param y Text baseline Y.
     * @param str Null-terminated string.
     * @param color Foreground color.
     * @param bg Background color.
     * @return Operation status.
     */
    virtual Result<void> drawString(int16_t x, int16_t y, const char* str, uint16_t color, uint16_t bg) = 0;

    /**
     * @brief Fill whole display with color.
     * @param color RGB565 color.
     * @return Operation status.
     */
    virtual Result<void> fillScreen(uint16_t color) = 0;
};