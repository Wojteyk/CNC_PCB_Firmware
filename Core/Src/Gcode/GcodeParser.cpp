#include "Gcode/GcodeParser.hpp"
#include <charconv>
#include <string_view>
#include <cctype>
#include "common/systemError.hpp"
#include "main.h"
#include "planner/planner.hpp"
#include "portmacro.h"
#include <cstring>

void GcodeParser::run()
{
    char lineBuffer[BUFFER_SIZE];
    // const char* startupCmds[] = {"G0 Z4", "G0 Z8"};
    // for (const char* c : startupCmds)
    // {
    //     auto res = parseLine(c);
    //     if (res.isOk())
    //         xQueueSend(_targetQueue, &res.value, portMAX_DELAY);
    // }
    while (true)
    {
        if (xQueueReceive(_gcodeQueue, lineBuffer, portMAX_DELAY) == pdPASS)
        {
            lineBuffer[BUFFER_SIZE - 1] = '\0';
            size_t lineLen = strnlen(lineBuffer, BUFFER_SIZE);
            std::string_view s(lineBuffer, lineLen);
            auto res = parseLine(s);

            if (res.isOk())
            {
                xQueueSend(_targetQueue, &res.value, portMAX_DELAY);
            }
            else
            {
                ErrorHandler::report(res.error);
            }
        }
    }
}

Result<MotionCmd> GcodeParser::parseLine(std::string_view line)
{
    if (line.empty() || line.size() < 2)
    {
        return Result<MotionCmd>(ErrorCode::Parser_LineEmpty);
    }

    MotionCmd cmd;

    while (!line.empty() && line.front() != '\n')
    {
        char c = std::toupper(line.front());
        switch (c)
        {
        case 'G':
            line.remove_prefix(1);
            {
                int g;
                if (!parseInt(line, g))
                    return Result<MotionCmd>(ErrorCode::Parser_InvalidFormat);
                cmd.motion = static_cast<MotionType>(g);
            }
            continue;
        case 'X':
            line.remove_prefix(1);
            {
                float x;
                if (!parseFloat(line, x))
                    return Result<MotionCmd>(ErrorCode::Parser_InvalidFormat);
                cmd.x = x;
            }
            continue;
        case 'Y':
            line.remove_prefix(1);
            {
                float y;
                if (!parseFloat(line, y))
                    return Result<MotionCmd>(ErrorCode::Parser_InvalidFormat);
                cmd.y = y;
            }
            continue;
        case 'Z':
            line.remove_prefix(1);
            {
                float z;
                if (!parseFloat(line, z))
                    return Result<MotionCmd>(ErrorCode::Parser_InvalidFormat);
                cmd.z = z;
            }
            continue;
        case 'F':
            line.remove_prefix(1);
            {
                float f;
                if (!parseFloat(line, f))
                    return Result<MotionCmd>(ErrorCode::Parser_InvalidFormat);
                cmd.f = f;
            }
            continue;
        case ' ':
            line.remove_prefix(1);
            continue;

        default:
            return Result<MotionCmd>(ErrorCode::Parser_UnknownCommand);
        }
    }

    return Result<MotionCmd>(cmd);
}

bool GcodeParser::parseFloat(std::string_view& number, float& value)
{
    auto [ptr, ec] = std::from_chars(number.data(), number.data() + number.size(), value);

    if (ec != std::errc{})
    {
        return false;
    }

    number.remove_prefix(ptr - number.data());

    return true;
}

bool GcodeParser::parseInt(std::string_view& number, int& value)
{
    auto [ptr, ec] = std::from_chars(number.data(), number.data() + number.size(), value);

    if (ec != std::errc{})
    {
        return false;
    }

    number.remove_prefix(ptr - number.data());

    return true;
}