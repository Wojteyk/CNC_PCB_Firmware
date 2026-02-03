/**
 * @file GcodeParser.hpp
 * @brief G-code parser for CNC machine commands
 * @details Provides parsing functionality for standard G-code commands used in CNC machines.
 *          Supports G0, G1, G2, G3 motion commands with X, Y, Z coordinates and feed rate.
 * @author CNC Project
 * @version 1.0
 */

#pragma once

#include <string_view>

#include "common/motionTypes.hpp"
#include "common/systemError.hpp"
#include "common/cppTask.hpp"
#include "planner/planner.hpp"

/**
 * @class GcodeParser
 * @brief Parser for G-code motion commands
 * @details Parses G-code lines and extracts motion commands with optional parameters.
 *          Handles uppercase/lowercase conversion and various number formats (int, float,
 * negative).
 * @example
 * @code
 * GcodeParser parser;
 * auto result = parser.parseLine("G1 X10.5 Y20.3 Z5 F100");
 * if (result.isOk()) {
 *     // result.value contains the parsed MotionCmd
 *     // with motion, x, y, z, f fields set
 * }
 * @endcode
 */
class GcodeParser : public CppTask
{
  public:
    GcodeParser(QueueHandle_t targetQueue)
        : CppTask("Gcode", 512, 2)
        , _targetQueue(targetQueue)
    {
      _gcodeQueue = xQueueCreate(10, BUFFER_SIZE);
    }

    QueueHandle_t getQueueHandle() const
    {
      return _gcodeQueue;
    }

  protected:
    void run() override;

  private:
    /**
     * @brief Parse a single G-code line
     * @param line G-code line to parse (e.g., "G1 X10 Y20 Z5 F100")
     * @return Result<CNC::MotionCmd> containing parsed motion command or error code
     * @details Parses G-code commands with the following format:
     *          - G[0-3] for motion type (Rapid, Linear, Arc CW/CCW)
     *          - X, Y, Z coordinates (optional, any order)
     *          - F feed rate (optional)
     *
     *          Command format: G# [X##] [Y##] [Z##] [F##]
     *          - Case insensitive (g, x, y, z, f are converted to uppercase)
     *          - Values can be integers or floats
     *          - Negative values are supported
     *          - Extra spaces are allowed
     *
     * @retval Ok Successfully parsed command
     * @retval Parser_InvalidFormat Invalid number format or missing value
     * @retval Parser_UnknownCommand Unknown command character detected
     * @retval Parser_LineEmpty Input line is empty or too short (< 2 characters)
     *
     * @example
     * @code
     * auto result = parser.parseLine("G1 X10.5 Y20.3");
     * if (result.isOk()) {
     *     assert(result.value.motion == CNC::MotionType::Linear);
     *     assert(result.value.x.value() == 10.5f);
     *     assert(result.value.y.value() == 20.3f);
     * }
     * @endcode
     */
    Result<MotionCmd> parseLine(std::string_view line);

    /**
     * @brief Parse a floating-point number from string
     * @param number String view containing the number (modified to skip past parsed number)
     * @param value Output parameter for parsed float value
     * @return true if parsing succeeded, false otherwise
     * @details Uses std::from_chars for parsing.
     * @internal
     */
    bool parseFloat(std::string_view& number, float& value);

    /**
     * @brief Parse an integer from string
     * @param number String view containing the integer (modified to skip past parsed number)
     * @param value Output parameter for parsed integer value
     * @return true if parsing succeeded, false otherwise
     * @details Uses std::from_chars for parsing. Supports negative values.
     * @internal
     */
    bool parseInt(std::string_view& number, int& value);

    QueueHandle_t _targetQueue;
    QueueHandle_t _gcodeQueue;

    static constexpr uint8_t BUFFER_SIZE = 64;
    static constexpr uint8_t QUEUE_SIZE = 20;
};