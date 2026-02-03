#pragma once

#include "stdint.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_spi.h"
#include "common/systemError.hpp"

struct Point
{
    int16_t x;
    int16_t y;
};

class XPT2046
{
  public:
    XPT2046(SPI_HandleTypeDef* hspi,
            GPIO_TypeDef* csPort,
            uint16_t csPin,
            GPIO_TypeDef* irqPort,
            uint16_t irqPin);

    Result<void> init();

    bool isPressed();

    Point getPoint();

  private:
    SPI_HandleTypeDef* _hspi;
    GPIO_TypeDef* _csPort;
    uint16_t _csPin;
    GPIO_TypeDef* _irqPort;
    uint16_t _irqPin;

    uint16_t getRaw(uint8_t cmd);

    long map(long x, long in_min, long in_max, long out_min, long out_max);
    
    static constexpr uint8_t GET_X_CMD = 0xD0;
    static constexpr uint8_t GET_Y_CMD = 0x90;

    static constexpr int16_t RAW_X_MIN = 350;
    static constexpr int16_t RAW_X_MAX = 3770;
    static constexpr int16_t RAW_Y_MIN = 370;
    static constexpr int16_t RAW_Y_MAX = 3860;

    static constexpr int16_t SCREEN_W = 320;
    static constexpr int16_t SCREEN_H = 240;
};