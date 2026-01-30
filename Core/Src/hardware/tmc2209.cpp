#include "hardware/tmc2209.hpp"
#include "common/systemError.hpp"
#include "projdefs.h"
#include "stm32f4xx_hal_def.h"
#include "stm32f4xx_hal_uart.h"
#include <cstdint>

static TMC2209* tmc_instance = nullptr;

TMC2209::TMC2209(UART_HandleTypeDef* huart,
                 uint8_t nodeAddr,
                 GPIO_TypeDef* dirPort,
                 uint32_t dirPin,
                 GPIO_TypeDef* stepPort,
                 uint32_t stepPin)
    : _huart(huart)
    , _nodeAddr(nodeAddr)
    , _dirPort(dirPort)
    , _stepPort(stepPort)
    , _dirPin(dirPin)
    , _stepPin(stepPin)
{
}

uint8_t TMC2209::calculateCRC(uint8_t* datagram, uint8_t datagramLength)
{
    uint8_t crc = 0;
    uint8_t currentByte;

    for (uint8_t i = 0; i < datagramLength; i++)
    {
        currentByte = datagram[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            if ((crc >> 7) ^ (currentByte & 0x01))
            {
                crc = (crc << 1) ^ 0x07;
            }
            else
            {
                crc = (crc << 1);
            }
            currentByte = currentByte >> 1;
        }
    }
    return crc;
}

Result<uint32_t> TMC2209::readRegister(uint8_t regAddr)
{
    uint8_t request[4];
    request[0] = 0x05;
    request[1] = _nodeAddr;
    request[2] = regAddr & 0x7F;
    request[3] = calculateCRC(request, 3);

    HAL_UART_AbortReceive(_huart);
    __HAL_UART_CLEAR_OREFLAG(_huart);

    volatile uint32_t dummy = _huart->Instance->DR; // clear the bufor
    (void)dummy;

    xQueueReset(_resultQueue);

    if (HAL_UART_Receive_IT(_huart, _rxBuffer, 12) != HAL_OK)
    {
        return Result<uint32_t>(ErrorCode::TMC_UartTimeout);
    }

    if (HAL_UART_Transmit(_huart, request, 4, 10) != HAL_OK)
    {
        HAL_UART_AbortReceive(_huart);
        return Result<uint32_t>(ErrorCode::TMC_UartTimeout);
    }

    auto result = waitForResult(100);
    if (!result.isOk())
        return Result<uint32_t>(ErrorCode::TMC_UartTimeout);

    return Result<uint32_t>(result.value);
}

Result<void> TMC2209::writeRegister(uint8_t regAddr, uint32_t data)
{
    uint8_t msg[8];

    msg[0] = 0x05; // Sync byte
    msg[1] = _nodeAddr;
    msg[2] = regAddr | 0x80;
    msg[3] = (data >> 24) & 0xFF;
    msg[4] = (data >> 16) & 0xFF;
    msg[5] = (data >> 8) & 0xFF;
    msg[6] = data & 0xFF; // LSB
    msg[7] = calculateCRC(msg, 7);

    if (HAL_UART_Transmit(_huart, msg, 8, 10) != HAL_OK)
    {
        return Result<void>(ErrorCode::TMC_UartTimeout);
    }

    return Result<void>();
}

Result<uint32_t> TMC2209::waitForResult(uint32_t timeoutMs)
{
    uint32_t receivedValue;

    if (xQueueReceive(_resultQueue, &receivedValue, pdMS_TO_TICKS(timeoutMs)) == pdPASS)
    {
        return Result<uint32_t>(receivedValue);
    }
    return Result<uint32_t>(ErrorCode::TMC_UartTimeout);
}

Result<void> TMC2209::setAndCheckRegister(uint8_t regAddr, uint32_t data)
{
    writeRegister(regAddr, data);

    vTaskDelay(pdMS_TO_TICKS(2));

    auto res = readRegister(regAddr);
    if (!res.isOk())
        return Result<void>(res.error);

    if (data == res.value)
    {
        return Result<void>();
    }

    return Result<void>(ErrorCode::TMC_ResponseNotCorrect);
}

Result<void> TMC2209::init()
{
    tmc_instance = this;

    _resultQueue = xQueueCreate(1, sizeof(uint32_t));

    configASSERT(_resultQueue);

    if (auto res = writeRegister(REG_NODECONF, CMD_NODECONF); !res.isOk())
        return res;

    if (auto res = setAndCheckRegister(REG_GCONF, CMD_GCONF); !res.isOk())
        return res;

    if (auto res = writeRegister(REG_IHOLD_IRUN, CMD_IHOLD_IRUN); !res.isOk())
        return res;

    if (auto res = writeRegister(REG_TPOWERDOWN, CMD_TPOWERDOWN); !res.isOk())
        return res;

    if (auto res = setAndCheckRegister(REG_CHOPCONF, CMD_CHOPCONF); !res.isOk())
        return res;

    tmc_instance = nullptr;

    return Result<void>();
}

void TMC2209::handleInterrupt()
{
    uint8_t* reply = &_rxBuffer[4];

    if (reply[7] == calculateCRC(reply, 7))
    {
        uint32_t value = ((uint32_t)reply[3] << 24) | ((uint32_t)reply[4] << 16) |
                         ((uint32_t)reply[5] << 8) | ((uint32_t)reply[6]);

        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendFromISR(_resultQueue, &value, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

//remember to change uart instance 

extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart)
{
    if (huart->Instance == USART1 && tmc_instance != nullptr)
    {
        tmc_instance->handleInterrupt();
    }
}

extern "C" void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart)
{
    if (huart->Instance == USART1)
    {

        __HAL_UART_CLEAR_OREFLAG(huart);
        HAL_UART_AbortReceive(huart);
    }
}