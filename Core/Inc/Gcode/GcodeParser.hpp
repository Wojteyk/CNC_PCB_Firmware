/** @file GcodeParser.hpp
 *  @brief G-code parser task.
 *  @details Reads text commands from queue and emits MotionCmd objects.
 */

#pragma once

#include <string_view>

#include "common/motionTypes.hpp"
#include "common/systemError.hpp"
#include "common/cppTask.hpp"
#include "planner/planner.hpp"

/**
 * @brief Parses queued G-code lines into motion commands.
 * @details Supports motion commands and optional X/Y/Z/F parameters.
 */
class GcodeParser : public CppTask
{
  public:
    /**
     * @brief Construct parser task.
     * @param targetQueue Planner queue receiving parsed MotionCmd items.
     */
    GcodeParser(QueueHandle_t targetQueue)
        : CppTask("Gcode", 2048, 2)
        , _targetQueue(targetQueue)
    {
      _gcodeQueue = xQueueCreate(GCODE_QUEUE_SIZE, BUFFER_SIZE);
    }

    /** @brief Get queue handle used to push incoming G-code text. */
    QueueHandle_t getQueueHandle() const
    {
      return _gcodeQueue;
    }

  protected:
    void run() override;

  private:
    /**
     * @brief Parse one G-code line.
     * @param line Input line.
     * @return Parsed command or parser error.
     */
    Result<MotionCmd> parseLine(std::string_view line);

    /**
     * @brief Parse float value from text.
     * @param number Input slice, advanced after parse.
     * @param value Parsed result.
     * @return true on success.
     */
    bool parseFloat(std::string_view& number, float& value);

    /**
     * @brief Parse integer value from text.
     * @param number Input slice, advanced after parse.
     * @param value Parsed result.
     * @return true on success.
     */
    bool parseInt(std::string_view& number, int& value);

    QueueHandle_t _targetQueue;
    QueueHandle_t _gcodeQueue;

    static constexpr uint8_t BUFFER_SIZE = 64;
    static constexpr uint8_t QUEUE_SIZE = 65;
    static constexpr uint8_t GCODE_QUEUE_SIZE = 80;
};