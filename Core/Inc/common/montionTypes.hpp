/**
 * @file montionTypes.hpp
 * @brief Motion command data structures
 * @details Defines data structures for motion commands including motion types
 *          and command parameters (axes, feed rate).
 * @note File name contains typo 'montion' instead of 'motion' for historical reasons
 * @author CNC Project
 * @version 1.0
 */

#pragma once

#include <cstdint>
#include <optional>

/**
 * @enum MotionType
 * @brief Types of motion commands
 * @details Enumeration of supported G-code motion types
 */
enum struct MotionType : uint8_t
{
    Rapid = 0,  ///< Rapid positioning (G0) - fastest movement
    Linear = 1, ///< Linear interpolation (G1) - controlled speed
    ArcCw = 2,  ///< Clockwise arc (G2) - circular motion
    ArcCCw = 3, ///< Counter-clockwise arc (G3) - circular motion
    None = 4,    ///< No motion specified
    MoveOnce = 91,
};

struct MachineState
{

    float currentX = 0.0f;
    float currentY = 0.0f;
    float currentZ = 0.0f;

    int32_t stepX = 0;
    int32_t stepY = 0;
    int32_t stepZ = 0;
};

/**
 * @struct MotionCmd
 * @brief Motion command with axes and feed rate
 * @details Represents a single G-code motion command with optional
 *          X, Y, Z coordinates and feed rate.
 */
struct MotionCmd
{
    /// Type of motion to perform
    MotionType motion = MotionType::None;

    /// Optional X-axis coordinate (units: depends on MachineConfig::defaultUnits)
    std::optional<float> x = std::nullopt;

    /// Optional Y-axis coordinate
    std::optional<float> y = std::nullopt;

    /// Optional Z-axis coordinate
    std::optional<float> z = std::nullopt;

    /// Optional feed rate (movement speed in units/minute)
    std::optional<float> f = std::nullopt;
};

struct StepCmd
{
    int32_t dX;
    int32_t dY;
    int32_t dZ;

    int32_t totalSteps;

    uint32_t startArr;  
    uint32_t targetArr; 
    uint32_t accelSteps; 
    uint32_t decelSteps; 

    uint8_t dirMask;
};

