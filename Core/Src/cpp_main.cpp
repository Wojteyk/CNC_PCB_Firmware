#include "main.h"
#include "hardware/ili9341.hpp"
#include "common/systemError.hpp"
#include "display/displayController.hpp"
#include "planner/planner.hpp"
#include "Gcode/GcodeParser.hpp"
#include "hardware/tmc2209.hpp"
#include "hardware/motionController.hpp"
#include "common/machineConfig.hpp"
#include "hardware/xpt2046.hpp"
#include "FreeRTOS.h"
#include "queue.h"

extern "C" SPI_HandleTypeDef hspi1;
extern "C" SPI_HandleTypeDef hspi2;
extern "C" UART_HandleTypeDef huart1;
extern "C" TIM_HandleTypeDef htim10;

size_t freeHeapNow;
QueueHandle_t guiEventQueue = nullptr;

MachineConfig globalConfig;

XPT2046 touch(&hspi2, TOUCH_CS_GPIO_Port, TOUCH_CS_Pin, TOUCH_IRQ_GPIO_Port, TOUCH_IRQ_Pin);
TMC2209 zAxis(&huart1, 0, ZAXIS_DIR_GPIO_Port, ZAXIS_DIR_Pin, ZAXIS_STEP_GPIO_Port, ZAXIS_STEP_Pin);
TMC2209 xAxis(&huart1, 1, XAXIS_DIR_GPIO_Port, XAXIS_DIR_Pin, XAXIS_STEP_GPIO_Port, XAXIS_STEP_Pin);
MotionController<TMC2209> mController(&htim10, xAxis, zAxis, zAxis, globalConfig);
Planner<TMC2209> planner(&mController, globalConfig);
GcodeParser gParser(planner.getQueueHandle());

ILI9341 displayDriver(&hspi1,
                      LCD_CS_GPIO_Port,
                      LCD_CS_Pin,
                      LCD_DC_GPIO_Port,
                      LCD_DC_Pin,
                      LCD_RST_GPIO_Port,
                      LCD_RST_Pin);

DisplayController<ILI9341, XPT2046, TMC2209> dispController(displayDriver, touch, planner, gParser.getQueueHandle());

extern "C" void cpp_main()
{
    guiEventQueue = xQueueCreate(5, sizeof(GuiEvent));

    dispController.start();

    auto res = mController.init();
    if (!res.isOk())
    {
        ErrorHandler::report(res.error);
    }

    planner.start();
    gParser.start();

    while (1)
    {
        freeHeapNow = xPortGetFreeHeapSize();
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

extern "C" void MotionController_Tick()
{
    if (MotionController<TMC2209>::instance != nullptr)
    {
        MotionController<TMC2209>::instance->tick();
    }
}