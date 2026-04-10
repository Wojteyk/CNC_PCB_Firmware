/** @file motionTypes.hpp
 *  @brief Motion command types.
 *  @details Shared motion data exchanged between parser, planner and controller.
 */

#pragma once

#include <cstdint>
#include <optional>

/** @brief Supported motion commands. */
enum struct MotionType : uint8_t
{
    Rapid = 0,  ///< Rapid move (G0).
    Linear = 1, ///< Linear move (G1).
    ArcCw = 2,  ///< Clockwise arc (G2).
    ArcCCw = 3, ///< Counter-clockwise arc (G3).
    None = 4,   ///< No motion.
    SetHome = 10,
    Homing = 28,
    Move = 91,
    Stop = 92,
};

/**
 * @brief Runtime machine position in units and steps.
 */
struct MachineState
{
    /// Current logical X position.
    float currentX = 0.0f;
    /// Current logical Y position.
    float currentY = 0.0f;
    /// Current logical Z position.
    float currentZ = 0.0f;

    /// Current X position in steps.
    int32_t stepX = 0;
    /// Current Y position in steps.
    int32_t stepY = 0;
    /// Current Z position in steps.
    int32_t stepZ = 0;

    /// Machine-referenced X step position (used by soft limits/homing).
    int32_t machineStepX = 0;
    /// Machine-referenced Y step position (used by soft limits/homing).
    int32_t machineStepY = 0;
    /// Machine-referenced Z step position (used by soft limits/homing).
    int32_t machineStepZ = 0;
};

/** @brief Parsed motion command. */
struct MotionCmd
{
    /// Motion type.
    MotionType motion = MotionType::None;

    /// Optional X value.
    std::optional<float> x = std::nullopt;

    /// Optional Y value.
    std::optional<float> y = std::nullopt;

    /// Optional Z value.
    std::optional<float> z = std::nullopt;

    /// Optional feed rate.
    std::optional<float> f = std::nullopt;
};

/**
 * @brief Discrete stepper command used by low-level motion controller.
 */
struct StepCmd
{
    /// Absolute step delta for X.
    uint32_t dX;
    /// Absolute step delta for Y.
    uint32_t dY;
    /// Absolute step delta for Z.
    uint32_t dZ;

    /// Total DDA step count for this segment.
    uint32_t totalSteps;

    uint32_t startArr;
    uint32_t targetArr;
    uint32_t accelSteps;
    uint32_t decelSteps;

    /// Direction bitmask (bit0=X, bit1=Y, bit2=Z).
    uint8_t dirMask;

    /// ARR increment/decrement value per accel step.
    uint16_t accIncrease;
    /// Whether deceleration phase should be applied.
    bool slowDown;
    /// Whether this command performs homing behavior.
    bool homing;
};

