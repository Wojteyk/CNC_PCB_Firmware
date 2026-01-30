#pragma once

#include "FreeRTOS.h"
#include "portmacro.h"
#include "projdefs.h"
#include "task.h"
#include <cstdint>
#include "common/systemError.hpp"

class CppTask
{
  public:
    CppTask(const char* name, uint16_t stackSize, UBaseType_t priority)
        : _name(name)
        , _stackSize(stackSize)
        , _priority(priority)
        , _handle(nullptr)
    {
    }

    virtual ~CppTask() = default;

    Result<void> start()
    {
        if (_handle != nullptr)
            return ErrorCode::Task_AlreadyRunning;

        BaseType_t res = xTaskCreate(taskEntryPoint, _name, _stackSize, this, _priority, &_handle);

        if (res != pdPASS)
        {
            return ErrorCode::Task_CreationFailed;
        }

        return ErrorCode::Ok;
    }

  protected:
    virtual void run() = 0;

  private:
    static void taskEntryPoint(void* params)
    {
        CppTask* task = static_cast<CppTask*>(params);

        task->run();
    }

    const char* _name;
    uint16_t _stackSize;
    UBaseType_t _priority;
    TaskHandle_t _handle;
};
