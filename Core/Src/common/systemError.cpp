#include "common/systemError.hpp"
#include "main.h"
#include "stm32f4xx_hal_gpio.h"

void ErrorHandler::report(ErrorCode e)
{
    if (isCritical(e))
    {
        panicMode(e);
    }

    // show msg erro somewhere
}

void ErrorHandler::panicMode(ErrorCode e)
{
    while (1)
    {
        // turn off everything irq, motors and essential things
       // HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
        for (volatile int i = 0; i < 500000; i++)
            ;
    }
}
bool ErrorHandler::isCritical(ErrorCode e)
{
    switch (e)
    {
    case ErrorCode::Machine_HardLimitReached:
    case ErrorCode::Machine_StepperFault:
    case ErrorCode::Task_CreationFailed:
    case ErrorCode::Parser_InvalidFormat:
    case ErrorCode::TMC_UartTimeout:
    case ErrorCode::TMC_ResponseNotCorrect:
        return true;

    default:
        return false;
    }
}