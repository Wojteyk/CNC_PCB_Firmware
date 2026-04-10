#pragma once

#include "FreeRTOS.h"
#include "common/systemError.hpp"
#include "stm32f4xx_hal_uart.h"
#include <cstdint>
#include "queue.h"

/**
 * @file tmc2209.hpp
 * @brief UART driver for TMC2209 stepper controller.
 */

#ifdef __cplusplus
extern "C"
{
#endif
    /** @brief HAL UART RX callback bridge. */
    void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart);
    /** @brief HAL UART error callback bridge. */
    void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart);
#ifdef __cplusplus
}
#endif

/** @brief TMC2209 UART stepper driver. */
class TMC2209
{
  public:
        /**
         * @brief Construct driver instance.
         * @param huart UART peripheral handle.
         * @param nodeAddr TMC2209 node address.
         * @param dirPort DIR GPIO port.
         * @param dirPin DIR GPIO pin mask.
         * @param stepPort STEP GPIO port.
         * @param stepPin STEP GPIO pin mask.
         */
    TMC2209(UART_HandleTypeDef* huart,
            uint8_t nodeAddr,
            GPIO_TypeDef* dirPort,
            uint32_t dirPin,
            GPIO_TypeDef* stepPort,
            uint32_t stepPin);

    /** @brief Initialize UART and default registers. */
    Result<void> init();

    /** @brief Allow HAL callback to access private ISR handler. */
    friend void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart);

    /** @brief Set motor direction pin. */
    void setDirection(bool dir)
    {
        _dirPort->BSRR = dir ? _dirPin : (uint32_t)_dirPin << 16;
    }

    /** @brief Drive STEP pin high. */
    inline void stepHigh()
    {
        _stepPort->BSRR = _stepPin;
    }

    /** @brief Drive STEP pin low. */
    inline void stepLow()
    {
        _stepPort->BSRR = (uint32_t)_stepPin << 16;
    }

  private:
        /** @brief Process RX frame in ISR context. */
    void handleInterrupt();

        /** @brief Write register and verify by readback. */
    Result<void> setAndCheckRegister(uint8_t regAddr, uint32_t data);

        /** @brief Wait for ISR response. */
    Result<uint32_t> waitForResult(uint32_t timeoutMs);

        /** @brief Send register write frame. */
    Result<void> writeRegister(uint8_t regAddr, uint32_t data);

        /** @brief Send register read request. */
    Result<uint32_t> readRegister(uint8_t regAddr);

        /** @brief Calculate UART frame CRC. */
    static uint8_t calculateCRC(uint8_t* datagram, uint8_t datagramLength);

        UART_HandleTypeDef* _huart; ///< UART handle.
        uint8_t _nodeAddr;          ///< Driver node address.
        QueueHandle_t _resultQueue; ///< ISR-to-task result queue.
        uint8_t _rxBuffer[12];      ///< UART RX buffer.
    GPIO_TypeDef* _dirPort;
    GPIO_TypeDef* _stepPort;
    uint32_t _dirPin;
    uint32_t _stepPin;

    /// Register addresses.
    static constexpr uint8_t REG_GCONF = 0x00;
    static constexpr uint8_t REG_NODECONF = 0x03;
    static constexpr uint8_t REG_IHOLD_IRUN = 0x10;
    static constexpr uint8_t REG_TPOWERDOWN = 0x11;
    static constexpr uint8_t REG_CHOPCONF = 0x6C;

    /// Configuration values.
    static constexpr uint32_t CMD_GCONF = 0x000000B1;
    static constexpr uint32_t CMD_NODECONF = 0x00000200;
    static constexpr uint32_t CMD_IHOLD_IRUN = 0x00081F0F;
    static constexpr uint32_t CMD_TPOWERDOWN = 0x00000014;
    static constexpr uint32_t CMD_CHOPCONF = 0x14000003;
};