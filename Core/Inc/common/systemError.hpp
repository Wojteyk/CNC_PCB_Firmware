/**
 * @file systemError.hpp
 * @brief Error codes and result wrapper template
 * @details Defines all error codes used throughout the system and provides
 *          a generic Result template for returning values with error status.
 * @author CNC Project
 * @version 1.0
 */

#pragma once

#include "main.h"
#include <stdint.h>

/**
 * @enum ErrorCode
 * @brief System-wide error codes
 * @details Enumeration of all possible errors that can occur in the system,
 *          categorized into parser errors, machine errors, and system errors.
 */
enum class ErrorCode : uint8_t
{
    Ok = 0, ///< Operation successful

    /// @defgroup ParserErrors Parser Error Codes
    /// Errors related to G-code parsing
    /// @{
    Parser_InvalidFormat,   ///< G-code line has invalid format
    Parser_UnknownCommand,  ///< Unknown G-code command detected
    Parser_ValueOutOfRange, ///< Parsed value is out of acceptable range
    Parser_LineEmpty,       ///< Input line is empty or too short
    /// @}

    /// @defgroup MachineErrors Machine Error Codes
    /// Errors related to physical machine state
    /// @{
    Machine_HardLimitReached, ///< Hard limit switch activated
    Machine_SoftLimitReached, ///< Software limit boundary exceeded
    Machine_StepperFault,     ///< Stepper motor malfunction detected
    /// @}

    /// @defgroup SystemErrors System Error Codes
    /// Errors related to system operation
    /// @{
    System_QueueFull,  ///< Command queue is full
    System_QueueEmpty, ///< Attempt to read from empty queue

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

    /// @}
};

/**
 * @template Result
 * @brief Generic result wrapper for value + error status
 * @tparam T Type of the value being returned
 * @details Template structure that combines a returned value with an error code.
 *          This allows functions to return both a result and error information.
 * @example
 * @code
 * Result<CNC::MotionCmd> parseResult = parser.parseLine("G1 X10 Y20");
 * if (parseResult.isOk()) {
 *     // Use parseResult.value
 * } else {
 *     // Handle error: parseResult.error
 * }
 * @endcode
 */
template <typename T> struct Result
{
    /// The returned value (default-constructed if an error occurred)
    T value;

    /// Error code indicating operation status
    ErrorCode error;

    /**
     * @brief Constructor for successful result
     * @param v The value to return
     * @post error is set to ErrorCode::Ok
     */
    Result(T v)
        : value(v)
        , error(ErrorCode::Ok)
    {
    }

    /**
     * @brief Constructor for error result
     * @param e The error code
     * @post value is default-constructed
     */
    Result(ErrorCode e)
        : value(T{})
        , error(e)
    {
    }

    /**
     * @brief Check if operation was successful
     * @return true if error is ErrorCode::Ok, false otherwise
     */
    bool isOk() const
    {
        return error == ErrorCode::Ok;
    }
};

template <> struct Result<void>
{
    ErrorCode error;

    // Konstruktor dla sukcesu
    Result()
        : error(ErrorCode::Ok)
    {
    }

    // Konstruktor dla konkretnego błędu
    Result(ErrorCode e)
        : error(e)
    {
    }

    // Metoda pomocnicza
    bool isOk() const
    {
        return error == ErrorCode::Ok;
    }
};

/**
 * @class ErrorHandler
 * @brief Static utility class for managing and responding to system errors.
 * @details Provides logic to distinguish between minor operational warnings
 * and critical hardware failures that require a system halt.
 */
class ErrorHandler
{
  public:
    /**
     * @brief Global entry point for error handling.
     * @param e ErrorCode to process.
     */
    static void report(ErrorCode e);

  private:
    /**
     * @brief Halts the system in a safe state.
     * @param e The fatal ErrorCode.
     */
    static void panicMode(ErrorCode e);

    /**
     * @brief Logic to categorize errors as critical or non-critical.
     * @param e ErrorCode to evaluate.
     * @return true if system must stop.
     */
    static bool isCritical(ErrorCode e);
};
