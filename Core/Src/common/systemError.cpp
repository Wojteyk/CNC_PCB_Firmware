#include "FreeRTOS.h"
#include "queue.h"
#include "common/systemError.hpp"
#include "main.h"
#include "stm32f4xx_hal_gpio.h"
#include "common/lcdConstants.hpp"

struct ErrorMapping {
    ErrorCode code;
    const char* message;
};

static volatile bool g_emergencyStop = false;

void ErrorHandler::report(ErrorCode e)
{
    g_lastError = e;  // Store the error code
    
    if (isCritical(e))
    {
        g_emergencyStop = true;
        panicMode(e);
    }
}

bool ErrorHandler::emergencyStopActive()
{
    return g_emergencyStop;
}

void ErrorHandler::clearEmergencyStop()
{
    g_emergencyStop = false;
}

void ErrorHandler::panicMode(ErrorCode e)
{
    // turn off everything irq, motors and essential things
    GuiEvent event = GuiEvent::ShowError;
    if (guiEventQueue != nullptr)
    {
        xQueueSend(guiEventQueue, &event, 0);
    }
}
bool ErrorHandler::isCritical(ErrorCode e)
{
    switch (e)
    {
    case ErrorCode::Machine_HardLimitReached:
    case ErrorCode::Machine_StepperFault:
    case ErrorCode::System_QueueCreateFail:
    case ErrorCode::Task_CreationFailed:
    case ErrorCode::Parser_InvalidFormat:
    case ErrorCode::TMC_UartTimeout:
    case ErrorCode::TMC_ResponseNotCorrect:
    case ErrorCode::Controller_TimInitFail:
    case ErrorCode::Controller_QueueCreateFail:
    case ErrorCode::Machine_SoftLimitReached:
        return true;

    default:
        return false;
    }
}

static const ErrorMapping errorTable[] = {
    { ErrorCode::Ok,                        "Operation Successful" },
    
    // Parser Errors
    { ErrorCode::Parser_InvalidFormat,      "G-Code: Invalid Format" },
    { ErrorCode::Parser_UnknownCommand,     "G-Code: Unknown Command" },
    { ErrorCode::Parser_ValueOutOfRange,    "G-Code: Value Out of Range" },
    { ErrorCode::Parser_LineEmpty,          "G-Code: Empty Line" },

    // Machine Errors
    { ErrorCode::Machine_HardLimitReached,  "CRITICAL: Hard Limit Hit" },
    { ErrorCode::Machine_SoftLimitReached,  "Boundary Limit Exceeded" },
    { ErrorCode::Machine_StepperFault,      "Stepper Driver Fault" },

    // System Errors
    { ErrorCode::System_QueueFull,          "System: Queue Overrun" },
    { ErrorCode::System_QueueEmpty,         "System: Queue Underrun" },
    { ErrorCode::System_QueueCreateFail,    "System: Queue Create Failed" },
    { ErrorCode::Task_InvalidParams,        "RTOS: Invalid Task Params" },
    { ErrorCode::Task_AlreadyRunning,       "RTOS: Task Already Active" },
    { ErrorCode::Task_CreationFailed,       "RTOS: Task Creation Failed" },

    // Hardware & Communication
    { ErrorCode::TMC_UartTimeout,           "TMC: UART Timeout" },
    { ErrorCode::TMC_ResponseNotCorrect,    "TMC: Invalid Response" },
    { ErrorCode::Controller_TimInitFail,    "HW: Timer Init Failed" },
    { ErrorCode::Controller_QueueCreateFail, "HW: Queue Init Failed" },
    { ErrorCode::ILI9341_SPIFailed,         "LCD: SPI Communication Error" },
    { ErrorCode::LCD_TransmissionFailed,    "LCD: Data Transfer Failed" },
    { ErrorCode::LCD_OutOfRange,            "LCD: Drawing Out of Bounds" }
};

const char* ErrorHandler::toMessage(ErrorCode e) {
    for (const auto& item : errorTable) {
        if (item.code == e) {
            return item.message;
        }
    }
    return "Unknown Error Code";
}