/**
 * @file MachineConfig.hpp
 * @brief Machine configuration structures and enumerations
 * @details Contains configuration structures for CNC machine including feed rates,
 *          acceleration, coordinate planes, units, and stepper motor settings.
 * @author CNC Project
 * @version 1.0
 */

#pragma once

#include <stdint.h>

/**
 * @enum Plane
 * @brief Machine working plane selection
 * @details Defines the primary working plane for machine operations
 */
enum class Plane
{
    XY, ///< XY Plane (default)
    XZ, ///< XZ Plane
    YZ  ///< YZ Plane
};

/**
 * @enum Units
 * @brief Length measurement units
 */
enum class Units
{
    MM,    ///< Millimeters
    Inches ///< Inches
};

/**
 * @enum DataSource
 * @brief Data source for G-code commands
 */
enum class DataSource
{
    None, ///< No data source
    SD,   ///< SD Card
    UART  ///< UART/Serial connection
};

/**
 * @struct MachineConfig
 * @brief Main machine configuration structure
 * @details Contains all configuration parameters for the CNC machine including
 *          movement speeds, acceleration, coordinate system settings, and mechanical calibration.
 */
struct MachineConfig
{
    /// Default feed rate in units/minute
    float defaultFeedRate = 500.0f;

    /// Default working plane
    Plane defaultPlane = Plane::XY;

    /// Default measurement units
    Units defaultUnits = Units::MM;

    /// Default data source for commands
    DataSource defaultDataSource = DataSource::None;

    float stepsPerMM_XY = 1600.0f; ///< Steps per millimeter for X, Y axis
    float stepsPerMM_Z = 1600.0f;  ///< Steps per millimeter for Z axis

    uint32_t xStepMax = 120*stepsPerMM_XY;
    uint32_t yStepMax = 120*stepsPerMM_XY;
    uint32_t zStepMax = 35*stepsPerMM_Z;

    uint32_t startingSpeedToArr = 2500;     //  400hz
    uint32_t defaultTargetSpeedToArr = 100; //  10khz
    uint32_t rapidTargetSpeedToArr = 70;    // ~14khz
    uint32_t slowTargetSpeedToArr = 200;

    struct Acceleration
    {
        uint16_t increase;
        uint16_t steps;
    };

    Acceleration defaultAcceleration = {25, 0};
    Acceleration rapidAcceleration = {10, 0};

    MachineConfig()
    {
        defaultAcceleration.steps =
            (startingSpeedToArr - defaultTargetSpeedToArr) / defaultAcceleration.increase;

        rapidAcceleration.steps =
            (startingSpeedToArr - rapidTargetSpeedToArr) / rapidAcceleration.increase;
    }
};
