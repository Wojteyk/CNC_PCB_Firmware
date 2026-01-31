#include "hardware/ili9341.hpp"
#include "stm32f4xx_hal_spi.h"
#include "common/lcdConstants.hpp"

uint16_t lineBuffer[LCD_BUFFER_SIZE];

ILI9341::ILI9341(SPI_HandleTypeDef* hspi,
                 GPIO_TypeDef* cs_port,
                 uint16_t cs_pin,
                 GPIO_TypeDef* dc_port,
                 uint16_t dc_pin,
                 GPIO_TypeDef* rst_port,
                 uint16_t rst_pin)
    : _hspi(hspi)
    , _cs_port(cs_port)
    , _cs_pin(cs_pin)
    , _dc_port(dc_port)
    , _dc_pin(dc_pin)
    , _rst_port(rst_port)
    , _rst_pin(rst_pin)
{
}

Result<void> ILI9341::init()
{
    // hard reset
    HAL_GPIO_WritePin(_rst_port, _rst_pin, GPIO_PIN_RESET);
    vTaskDelay(pdMS_TO_TICKS(20));
    HAL_GPIO_WritePin(_rst_port, _rst_pin, GPIO_PIN_SET);
    vTaskDelay(pdMS_TO_TICKS(50));

    if (auto res = writeCmd(CMD_SWRESET); !res.isOk())
        return res;
    vTaskDelay(pdMS_TO_TICKS(20));
    if (auto res = writeCmd(CMD_SLPOUT); !res.isOk())
        return res;
    vTaskDelay(pdMS_TO_TICKS(120));
    if (auto res = writeCmd(CMD_MADCTL); !res.isOk())
        return res;
    if (auto res = writeData(0xE8);
        !res.isOk()) // set BGR to RGB conversion and landscape orientation
        return res;
    if (auto res = writeCmd(CMD_PIXFMT); !res.isOk())
        return res;
    if (auto res = writeData(0x55); !res.isOk()) // set RGB565
        return res;
    if (auto res = writeCmd(CMD_DISPON); !res.isOk()) // display on
        return res;

    return Result<void>(ErrorCode::Ok);
}

Result<void> ILI9341::setWindow(uint16_t x0, uint16_t x1, uint16_t y0, uint16_t y1)
{
    //  (X)
    if (auto res = writeCmd(0x2A); !res.isOk())
        return res;
    writeData16(x0);
    writeData16(x1);

    //  (Y)
    if (auto res = writeCmd(0x2B); !res.isOk())
        return res;
    writeData16(y0);
    writeData16(y1);

    if (auto res = writeCmd(0x2C); !res.isOk())
        return res;

    return Result<void>(ErrorCode::Ok);
}

Result<void> ILI9341::fillScreen(uint16_t color)
{
    if (auto res = setWindow(0, X_MAX, 0, Y_MAX); !res.isOk())
        return res;
    pushColorBlock(color, 76800*2);

    return Result<void>(ErrorCode::Ok);
}

Result<void> ILI9341::fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (x > X_MAX || y > Y_MAX)
        return Result<void>(ErrorCode::LCD_OutOfRange);
    if ((x + w) > X_MAX)
        w = X_MAX - x;
    if ((y + h) > Y_MAX)
        h = Y_MAX - y;

    if (auto res = setWindow(x, w + x-1, y, y + h-1); !res.isOk())
        return res;

    return pushColorBlock(color, w * h);
}

Result<void> ILI9341::pushColorBlock(uint32_t color, uint32_t pixelCnt)
{
    uint16_t swappedColor = (color << 8) | (color >> 8);

    uint32_t bufferSize = (pixelCnt < LCD_BUFFER_SIZE) ? pixelCnt : LCD_BUFFER_SIZE;

    for (int i = 0; i < bufferSize; i++)
        lineBuffer[i] = swappedColor;

    uint32_t remaining = pixelCnt;

    HAL_GPIO_WritePin(_dc_port, _dc_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_RESET);
    _taskToNotify = xTaskGetCurrentTaskHandle();
    xTaskNotifyStateClear(NULL);

    while (remaining > 0)
    {
        uint32_t chunkSize = (remaining < LCD_BUFFER_SIZE) ? remaining : LCD_BUFFER_SIZE;
        if (HAL_SPI_Transmit_DMA(_hspi, (uint8_t*)lineBuffer, chunkSize*2) != HAL_OK)
        {
            HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_SET);
            _taskToNotify = nullptr;
            return Result<void>(ErrorCode::LCD_TransmissionFailed);
        }

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        remaining -= chunkSize;
    }

    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_SET);
    _taskToNotify = nullptr;
    return Result<void>(ErrorCode::Ok);
}

Result<void> ILI9341::drawChar(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg)
{
    if (c < 32 || c > 126)
        return Result<void>(ErrorCode::Ok);

    if (auto res = setWindow(x, x + FONT_WIDTH - 1, y, y + FONT_HEIGHT - 1); !res.isOk())
        return res;

    uint16_t swappedFg = (color << 8) | (color >> 8);
    uint16_t swappedBg = (bg << 8) | (bg >> 8);

    const uint8_t* charBitmap = Font7x10[c - 32];

    uint32_t buffIdx = 0;

    for (int row = 0; row < FONT_HEIGHT; row++)
    {
        uint8_t bitmask = charBitmap[row];
        for (int col = 0; col < FONT_WIDTH; col++)
        {
            bool pixelOn = (bitmask & (0x80 >> col));
            lineBuffer[buffIdx++] = pixelOn ? swappedFg : swappedBg;
        }
    }

    HAL_GPIO_WritePin(_dc_port, _dc_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_RESET);
    _taskToNotify = xTaskGetCurrentTaskHandle();
    xTaskNotifyStateClear(NULL);

    if (HAL_SPI_Transmit_DMA(_hspi, (uint8_t*)lineBuffer, (FONT_WIDTH * FONT_HEIGHT) * 2) != HAL_OK)
    {
        HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_SET);
        _taskToNotify = nullptr;
        return Result<void>(ErrorCode::ILI9341_SPIFailed);
    }

    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_SET);
    _taskToNotify = nullptr;

    return Result<void>(ErrorCode::Ok);
}

Result<void> ILI9341::drawString(uint16_t x, uint16_t y, const char* str, uint16_t color, uint16_t bg)
{
    while(*str)
    {
        if(x+FONT_WIDTH > X_MAX)
        {
            y += FONT_HEIGHT + 3;
            x = 2;
        }

        if(auto res = drawChar(x, y, *str, color, bg); !res.isOk()) return res;

        x += 7;
        str++;
    }
    return Result<void>(ErrorCode::Ok);
}

Result<void> ILI9341::writeCmd(uint8_t cmd)
{
    HAL_GPIO_WritePin(_dc_port, _dc_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_RESET);

    HAL_StatusTypeDef status = HAL_SPI_Transmit(_hspi, &cmd, 1, 10);

    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_SET);

    if (status != HAL_OK)
    {
        return Result<void>(ErrorCode::ILI9341_SPIFailed);
    }
    return Result<void>(ErrorCode::Ok);
}

Result<void> ILI9341::writeData(uint8_t data)
{
    HAL_GPIO_WritePin(_dc_port, _dc_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_RESET);

    HAL_StatusTypeDef status = HAL_SPI_Transmit(_hspi, &data, 1, 10);

    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_SET);

    if (status != HAL_OK)
    {
        return Result<void>(ErrorCode::ILI9341_SPIFailed);
    }
    return Result<void>(ErrorCode::Ok);
}

Result<void> ILI9341::writeData16(uint16_t data)
{
    uint8_t buf[2] = {(uint8_t)(data >> 8), (uint8_t)(data & 0xFF)};
    HAL_GPIO_WritePin(_dc_port, _dc_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_RESET);

    HAL_StatusTypeDef status = HAL_SPI_Transmit(_hspi, buf, 2, 10);

    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_SET);

    if (status != HAL_OK)
    {
        return Result<void>(ErrorCode::ILI9341_SPIFailed);
    }
    return Result<void>(ErrorCode::Ok);
}

void ILI9341::handleTxComplete()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (_taskToNotify != nullptr)
    {
        vTaskNotifyGiveFromISR(_taskToNotify, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}