#pragma once
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_spi.h"
#include "common/systemError.hpp"

#define LCD_BUFFER_SIZE 1600

class ILI9341
{

  public:
    ILI9341(SPI_HandleTypeDef* hspi,
            GPIO_TypeDef* cs_port,
            uint16_t cs_pin,
            GPIO_TypeDef* dc_port,
            uint16_t dc_pin,
            GPIO_TypeDef* rst_port,
            uint16_t rst_pin);

    Result<void> init();

    Result<void> fillScreen(uint16_t color);

    void handleTxComplete();

    Result<void> setWindow(uint16_t x0, uint16_t x1, uint16_t y0, uint16_t y1);

    Result<void> fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

    Result<void> drawChar(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg);

    Result<void> drawString(uint16_t x, uint16_t y, const char* str, uint16_t color, uint16_t bg);

  private:
    SPI_HandleTypeDef* _hspi;

    TaskHandle_t _taskToNotify = nullptr;

    GPIO_TypeDef* _cs_port;
    uint16_t _cs_pin;
    GPIO_TypeDef* _dc_port;
    uint16_t _dc_pin;
    GPIO_TypeDef* _rst_port;
    uint16_t _rst_pin;

    Result<void> writeCmd(uint8_t cmd);
    Result<void> writeData(uint8_t data);
    Result<void> writeData16(uint16_t data);
    Result<void> pushColorBlock(uint32_t color, uint32_t pixelCnt);

    static constexpr uint16_t Y_MAX = 239;
    static constexpr uint16_t X_MAX = 319;

    static constexpr uint8_t CMD_SWRESET = 0x01;
    static constexpr uint8_t CMD_SLPOUT = 0x11;
    static constexpr uint8_t CMD_DISPON = 0x29;
    static constexpr uint8_t CMD_CASET = 0x2A;
    static constexpr uint8_t CMD_PASET = 0x2B;
    static constexpr uint8_t CMD_RAMWR = 0x2C;
    static constexpr uint8_t CMD_MADCTL = 0x36;
    static constexpr uint8_t CMD_PIXFMT = 0x3A;
};