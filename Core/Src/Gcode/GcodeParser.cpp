#include "Gcode/GcodeParser.hpp"
#include <charconv>
#include <string_view>
#include <cctype>
#include "common/systemError.hpp"
#include "main.h"
#include "planner/planner.hpp"
#include "portmacro.h"

void GcodeParser::run()
{
    {
        while (true)
        {
            auto res = parseLine("G1 Z4");

            if (res.isOk())
            {
                xQueueSend(_targetQueue, &res.value, portMAX_DELAY);
            }
            else
            {
                ErrorHandler::report(res.error);
            }

            auto res2 = parseLine("G1 Z-4");

            if (res2.isOk())
            {
                xQueueSend(_targetQueue, &res2.value, portMAX_DELAY);
            }
            else
            {
                ErrorHandler::report(res2.error);
            }

             vTaskDelay(pdMS_TO_TICKS(1000)); 
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