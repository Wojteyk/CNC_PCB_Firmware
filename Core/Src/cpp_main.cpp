#include "main.h"
#include "hardware/ili9341.hpp"
#include "common/systemError.hpp"

// Importujemy uchwyt SPI z main.c
extern SPI_HandleTypeDef hspi1;

ILI9341 displayDriver(&hspi1,
                      LCD_CS_GPIO_Port,
                      LCD_CS_Pin,
                      LCD_DC_GPIO_Port,
                      LCD_DC_Pin,
                      LCD_RST_GPIO_Port,
                      LCD_RST_Pin);

// --- Główny test ---
extern "C" void cpp_main()
{
    auto res = displayDriver.init();
    if (!res.isOk())
    {
        ErrorHandler::report(res.error);
    }

    while (true)
    {
        auto res2 = displayDriver.fillScreen(0x00F8);
        if (!res2.isOk())
        {
            ErrorHandler::report(res2.error);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));

        auto res3 = displayDriver.fillScreen(0xF800);
        if (!res3.isOk())
        {
            ErrorHandler::report(res2.error);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

extern "C" void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef* hspi)
{
    if (hspi->Instance == SPI1)
    {
        displayDriver.handleTxComplete();
    }
}