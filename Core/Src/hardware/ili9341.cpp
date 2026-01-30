#include "hardware/ili9341.hpp"
#include "stm32f4xx_hal_spi.h"


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
    if (auto res = writeData(0xE8); !res.isOk()) // set BGR to RGB conversion and landscape orientation
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
    if (auto res = setWindow(0, 319, 0, 239); !res.isOk())
        return res;

    // Jeśli wyślesz 0xF800 (Czerwony) bez zamiany, ekran dostanie 0x00F8 (Niebieski)
    uint16_t swappedColor = (color << 8) | (color >> 8);

    static uint16_t lineBuffer[1600];

    for (int i = 0; i < 1600; i++)
    {
        lineBuffer[i] = swappedColor;
    }

    HAL_GPIO_WritePin(_dc_port, _dc_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_RESET);

    _taskToNotify = xTaskGetCurrentTaskHandle();

    for (int y = 0; y < 48; y++)
    {
        if (HAL_SPI_Transmit_DMA(_hspi, (uint8_t*)lineBuffer, 1600* 2) != HAL_OK)
        {
            HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_SET);
            return Result<void>(ErrorCode::ILI9341_SPIFailed);
        }

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }

    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_SET);
    _taskToNotify = nullptr;

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