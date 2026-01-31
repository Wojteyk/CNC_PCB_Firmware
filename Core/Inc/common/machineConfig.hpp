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

    uint32_t startingSpeedToArr = 5000; //  2khz
    uint32_t defaultTargetspeedToArr = 100; //  10khz

    struct Acceleration {
        uint16_t increase = 25;
        uint16_t steps = 0;
    } acceleration;
    
    MachineConfig()
    {
        acceleration.steps =
            (startingSpeedToArr - defaultTargetspeedToArr)
            / acceleration.increase;
    }
};
