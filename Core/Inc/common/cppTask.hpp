#pragma once

#include "FreeRTOS.h"
#include "portmacro.h"
#include "projdefs.h"
#include "task.h"
#include <cstdint>
#include "common/systemError.hpp"

/**
 * @file cppTask.hpp
 * @brief C++ wrapper around FreeRTOS task creation.
 */

/**
 * @brief Base class for FreeRTOS tasks implemented in C++.
 */
class CppTask
{
  public:
    /**
     * @brief Construct task descriptor.
     * @param name Task name.
     * @param stackSize Task stack size in words.
     * @param priority FreeRTOS priority.
     */
    CppTask(const char* name, uint16_t stackSize, UBaseType_t priority)
        : _name(name)
        , _stackSize(stackSize)
        , _priority(priority)
        , _handle(nullptr)
    {
    }

    virtual ~CppTask() = default;

    /**
     * @brief Create and start the task.
     * @return Error status of task creation.
     */
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
    /** @brief Task body implemented by derived class. */
    virtual void run() = 0;

  private:
    /** @brief Static entry passed to xTaskCreate. */
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
