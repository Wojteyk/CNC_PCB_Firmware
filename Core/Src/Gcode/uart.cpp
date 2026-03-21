#include "Gcode/uart.hpp"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <cstring>

Uart::Uart(UART_HandleTypeDef* huart, uint16_t bufferSize, QueueHandle_t targetQueue)
    : CppTask("UART", 512, 2)
    , _huart(huart)
    , _taskHandle(nullptr)
    , _targetQueue(targetQueue)
    , _linePos(0)
    , _dmaBuffer(nullptr)
    , _bufferSize(bufferSize)
    , _oldPos(0)
{
    _dmaBuffer = new uint8_t[_bufferSize];
    memset(_dmaBuffer, 0, _bufferSize);
    memset(_lineBuffer, 0, sizeof(_lineBuffer));
}

Uart::~Uart()
{
    if (_dmaBuffer != nullptr)
    {
        delete[] _dmaBuffer;
    }
}

void Uart::init()
{
    _taskHandle = xTaskGetCurrentTaskHandle();
    HAL_UARTEx_ReceiveToIdle_DMA(_huart, _dmaBuffer, _bufferSize);
    __HAL_DMA_DISABLE_IT(_huart->hdmarx, DMA_IT_HT);
}

void Uart::run()
{
    _taskHandle = xTaskGetCurrentTaskHandle();
    
    HAL_UARTEx_ReceiveToIdle_DMA(_huart, _dmaBuffer, _bufferSize);
    __HAL_DMA_DISABLE_IT(_huart->hdmarx, DMA_IT_HT);
    while (1)
    {
        processDma();
    }
}

void Uart::sendOk()
{
    const char* msg = "ok\n";

    HAL_UART_Transmit(_huart, (uint8_t*)msg, strlen(msg), 100);
}

void Uart::handleCallback()
{
    if (_taskHandle != NULL)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        vTaskNotifyGiveFromISR(_taskHandle, &xHigherPriorityTaskWoken);

        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void Uart::processDma()
{
    if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) == pdTRUE)
    {
        uint16_t currentPos = _bufferSize - __HAL_DMA_GET_COUNTER(_huart->hdmarx);

        if (currentPos == _oldPos)
            return;

        if (currentPos > _oldPos)
        {
            uint16_t length = currentPos - _oldPos;
            processData(&_dmaBuffer[_oldPos], length);
        }
        else
        {
            uint16_t firstPart = _bufferSize - _oldPos;
            uint16_t secondPart = currentPos;

            processData(&_dmaBuffer[_oldPos], firstPart);
            processData(_dmaBuffer, secondPart);
        }

        _oldPos = currentPos;
    }
}

void Uart::processData(const uint8_t* data, uint16_t length)
{
    for (uint16_t i = 0; i < length; i++)
    {
        char c = (char)data[i];
        
        if (c == '\r') 
            continue;

        if (c == '\n')
        {
            if (_linePos > 0)
            {
                _lineBuffer[_linePos] = '\0';

                if (_targetQueue != NULL)
                {
                    if(xQueueSend(_targetQueue, _lineBuffer, portMAX_DELAY) == pdPASS)
                    {
                        sendOk();
                    }
                }
                
                _linePos = 0;
            }
        }
        else if (_linePos < sizeof(_lineBuffer) - 1)
        {
            _lineBuffer[_linePos++] = c;
        }
        else
        {
            _linePos = 0;
        }
    }
}