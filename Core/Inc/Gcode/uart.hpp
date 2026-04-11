#pragma once

#include <cstdint>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"
#include "common/cppTask.hpp"

class Uart : public CppTask
{
  public:
    Uart(UART_HandleTypeDef* huart, uint16_t bufferSize, QueueHandle_t targetQueue);
    ~Uart();

    void init();
    void sendOk(); 
    void handleCallback();

  protected:
    void run() override;

  private:
    void processDma();
    void processData(const uint8_t* data, uint16_t length);

    UART_HandleTypeDef* _huart;
    TaskHandle_t _taskHandle;
    QueueHandle_t _targetQueue;

    char _lineBuffer[128];
    uint16_t _linePos;
    uint8_t* _dmaBuffer;
    uint16_t _bufferSize;
    uint16_t _oldPos;
};