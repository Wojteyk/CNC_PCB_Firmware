/**
 * @file MachineConfig.hpp
 * @brief Machine configuration structures and enumerations
 * @details Contains configuration structures for CNC machine including feed rates,
 *          acceleration, coordinate planes, units, and stepper motor settings.
 * @author CNC Project
 * @version 1.0
 */

#pragma once

#include <cstdint>

namespace CNC
{

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

    /// Acceleration value in units/second²
    float acceleration = 100.0f;

    /// Default working plane
    Plane defaultPlane = Plane::XY;

    /// Default measurement units
    Units defaultUnits = Units::MM;

    /// Default data source for commands
    DataSource defaultDataSource = DataSource::None;

    /**
     * @struct StepsPerMM
     * @brief Stepper motor calibration parameters
     * @details Defines the relationship between physical distances and stepper motor steps
     */
    struct
    {
        float x = 80.0f;  ///< Steps per millimeter for X axis
        float y = 80.0f;  ///< Steps per millimeter for Y axis
        float z = 400.0f; ///< Steps per millimeter for Z axis
    } stepsPerMM;
};
} // namespace CNC
