/** @file machineConfig.hpp
 *  @brief Machine configuration types.
 *  @details Holds static machine limits, speed profiles and calibration values.
 */

#pragma once

#include <stdint.h>

/** @brief Machine working plane. */
enum class Plane
{
    XY, ///< XY plane.
    XZ, ///< XZ plane.
    YZ  ///< YZ plane.
};

/** @brief Length units. */
enum class Units
{
    MM,    ///< Millimeters.
    Inches ///< Inches.
};

/** @brief G-code input source. */
enum class DataSource
{
    None, ///< No source.
    SD,   ///< SD card.
    UART  ///< UART.
};

/** @brief Machine configuration values. */
struct MachineConfig
{
    /// Default feed rate [units/min].
    float defaultFeedRate = 500.0f;

    /// Default plane.
    Plane defaultPlane = Plane::XY;

    /// Default units.
    Units defaultUnits = Units::MM;

    /// Default command source.
    DataSource defaultDataSource = DataSource::None;

    float stepsPerMM_XY = 1600.0f; ///< Steps per millimeter for X/Y.
    float stepsPerMM_Z = 1600.0f;  ///< Steps per millimeter for Z.

    uint32_t xStepMax = 140*stepsPerMM_XY;   ///< Soft limit for X axis in steps.
    uint32_t yStepMax = 140*stepsPerMM_XY;   ///< Soft limit for Y axis in steps.
    uint32_t zStepMax = 35*stepsPerMM_Z;     ///< Soft limit for Z axis in steps.

    uint32_t stepTimerClockHz = 1000000;     ///< TIM tick after prescaler [Hz].

    uint32_t startingSpeedToArr = 2500;      ///< Start speed ARR 
    uint32_t slowTargetSpeedToArr = 200;     ///< slow target ARR 
    uint32_t defaultTargetSpeedToArr = 100;   ///< Default target ARR 
    uint32_t rapidTargetSpeedToArr = 80;     ///< Rapid target ARR 

    uint16_t defaultAccelerationSteps = 100; 
    uint16_t rapidAccelerationSteps   = 250;
   
};