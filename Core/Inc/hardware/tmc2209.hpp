#pragma once

#include "FreeRTOS.h"
#include "common/systemError.hpp"
#include "stm32f4xx_hal_uart.h"
#include <cstdint>
#include "queue.h"

#ifdef __cplusplus
extern "C"
{
#endif
    /**
     * @brief Global UART RX complete callback for STM32 HAL.
     * Dispatches the interrupt to the TMC2209 instance.
     */
    void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart);
    /**
     * @brief Global UART Error callback for STM32 HAL.
     * Handles overrun and communication errors.
     */
    void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart);
#ifdef __cplusplus
}
#endif

/**
 * @class TMC2209
 * @brief Driver class for the TMC2209 stepper motor driver using UART communication.
 * * This class handles the low-level UART protocol, CRC calculation, and
 * asynchronous register access via FreeRTOS queues.
 */
class TMC2209
{
  public:
    /**
     * @brief Constructs a new TMC2209 object.
     * @param huart Pointer to the STM32 HAL UART handle.
     * @param nodeAddr The UART node address of the TMC2209 (default 0).
     */
    TMC2209(UART_HandleTypeDef* huart,
            uint8_t nodeAddr,
            GPIO_TypeDef* dirPort,
            uint32_t dirPin,
            GPIO_TypeDef* stepPort,
            uint32_t stepPin);

    /**
     * @brief Initializes the driver, creates FreeRTOS primitives, and configures basic registers.
     * @return Result<void> Success or error code (e.g., TMC_UartTimeout).
     */
    Result<void> init();

    /** @brief Friend declaration to allow HAL callback access to handleInterrupt() */
    friend void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart);

    void setDirection(bool dir)
    {
        _dirPort->BSRR = dir ? _dirPin : (uint32_t)_dirPin << 16;
    }

    inline void stepHigh()
    {
        _stepPort->BSRR = _stepPin;
    }

    inline void stepLow()
    {
        _stepPort->BSRR = (uint32_t)_stepPin << 16;
    }

  private:
    /**
     * @brief Processes the received UART datagram inside the ISR.
     * Validates CRC and pushes the register value to the result queue.
     */
    void handleInterrupt();

    /**
     * @brief Writes a value to a register and immediately reads it back to verify.
     * @param regAddr The address of the register.
     * @param data The 32-bit data to write.
     * @return Result<void> Returns TMC_ResponseNotCorrect if readback fails.
     */
    Result<void> setAndCheckRegister(uint8_t regAddr, uint32_t data);

    /**
     * @brief Blocks the current task until a result is received from the ISR or timeout occurs.
     * @param timeoutMs Maximum time to wait in milliseconds.
     * @return Result<uint32_t> The 32-bit register value or timeout error.
     */
    Result<uint32_t> waitForResult(uint32_t timeoutMs);

    /**
     * @brief Sends a write datagram to the TMC2209.
     * @param regAddr The address of the register.
     * @param data The 32-bit data to write.
     * @return Result<void> Success or transmit error.
     */
    Result<void> writeRegister(uint8_t regAddr, uint32_t data);

    /**
     * @brief Sends a read request datagram and initiates interrupt-driven reception.
     * @param regAddr The address of the register to read.
     * @return Result<uint32_t> The received 32-bit value.
     */
    Result<uint32_t> readRegister(uint8_t regAddr);

    /**
     * @brief Calculates the 8-bit CRC for TMC2209 UART frames.
     * @param datagram Pointer to the data buffer.
     * @param datagramLength Length of the data to process.
     * @return uint8_t The calculated CRC byte.
     */
    static uint8_t calculateCRC(uint8_t* datagram, uint8_t datagramLength);

    UART_HandleTypeDef* _huart; ///< UART hardware handle
    uint8_t _nodeAddr;          ///< TMC2209 slave address
    QueueHandle_t _resultQueue; ///< FreeRTOS queue for ISR-to-Task communication
    uint8_t _rxBuffer[12];      ///< Buffer for incoming UART frames (including echo)
    GPIO_TypeDef* _dirPort;
    GPIO_TypeDef* _stepPort;
    uint32_t _dirPin;
    uint32_t _stepPin;

    // Register Addresses
    static constexpr uint8_t REG_GCONF = 0x00;
    static constexpr uint8_t REG_NODECONF = 0x03;
    static constexpr uint8_t REG_IHOLD_IRUN = 0x10;
    static constexpr uint8_t REG_TPOWERDOWN = 0x11;
    static constexpr uint8_t REG_CHOPCONF = 0x6C;

    // Command/Configuration Values
    static constexpr uint32_t CMD_GCONF = 0x000000B1;
    static constexpr uint32_t CMD_NODECONF = 0x00000200;
    static constexpr uint32_t CMD_IHOLD_IRUN = 0x00071008; // Irun 1A, Ihold 25%, delay hold 7
    static constexpr uint32_t CMD_TPOWERDOWN = 0x00000014; // motor sleep
    static constexpr uint32_t CMD_CHOPCONF = 0x14000003;   // 1/16 microstep, interpolation
};