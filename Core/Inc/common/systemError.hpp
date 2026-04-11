/** @file systemError.hpp
 *  @brief Error codes and result helpers.
 *  @details Shared error model for parser, planner, drivers and GUI.
 */

#pragma once

#include "main.h"
#include <stdint.h>

/** @brief System error codes. */
enum class ErrorCode : uint8_t
{
    Ok = 0, ///< Success.

    Parser_InvalidFormat,   ///< Invalid G-code format.
    Parser_UnknownCommand,  ///< Unknown G-code command.
    Parser_ValueOutOfRange, ///< Parsed value out of range.
    Parser_LineEmpty,       ///< Empty input line.

    Machine_HardLimitReached, ///< Hard limit reached.
    Machine_SoftLimitReached, ///< Soft limit reached.
    Machine_StepperFault,     ///< Stepper fault.

    System_QueueFull,  ///< Queue full.
    System_QueueEmpty, ///< Queue empty.
    System_QueueCreateFail,

    Task_InvalidParams,
    Task_AlreadyRunning,
    Task_CreationFailed,

    TMC_UartTimeout,
    TMC_ResponseNotCorrect,

    Controller_TimInitFail,
    Controller_QueueCreateFail,

    ILI9341_SPIFailed,

    LCD_TransmissionFailed,
    LCD_OutOfRange,

};

/** @brief Value/error result wrapper. */
template <typename T> struct Result
{
    /// Returned value.
    T value;

    /// Operation status.
    ErrorCode error;

    /**
     * @brief Construct successful result.
     * @param v Returned value.
     */
    Result(T v)
        : value(v)
        , error(ErrorCode::Ok)
    {
    }

    /**
     * @brief Construct failed result.
     * @param e Error code.
     */
    Result(ErrorCode e)
        : value(T{})
        , error(e)
    {
    }

    /** @brief Check for success. */
    bool isOk() const
    {
        return error == ErrorCode::Ok;
    }
};

template <> struct Result<void>
{
    ErrorCode error;

    /** @brief Construct successful void result. */
    Result()
        : error(ErrorCode::Ok)
    {
    }

    /**
     * @brief Construct failed void result.
     * @param e Error code.
     */
    Result(ErrorCode e)
        : error(e)
    {
    }

    /** @brief Check for success. */
    bool isOk() const
    {
        return error == ErrorCode::Ok;
    }
};

/** @brief Handles system error reporting. */
class ErrorHandler
{
  public:
    /** @brief Handle an error code. */
    static void report(ErrorCode e);

        /** @brief Return true when emergency stop is active. */
        static bool emergencyStopActive();

        /** @brief Clear emergency stop state. */
        static void clearEmergencyStop();

    /** @brief Convert error code to readable text. */
    static const char* toMessage(ErrorCode e);

  private:
    /** @brief Enter panic mode for fatal errors. */
    static void panicMode(ErrorCode e);

    /** @brief Check if an error is critical. */
    static bool isCritical(ErrorCode e);
};
