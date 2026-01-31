#pragma once
#include "common/cppTask.hpp"
#include "FreeRTOS.h"
#include "hardware/montionController.hpp"
#include "queue.h"
#include <algorithm>
#include <math.h>
#include "common/montionTypes.hpp"
#include "common/machineConfig.hpp"

template <typename T> class Planner : public CppTask
{
  public:
    Planner(MotionController<T>* controller, const MachineConfig& config)
        : CppTask("Planner", 1024, 2)
        , _config(config)
        ,_controller(controller)
    {
        _montionQueue = xQueueCreate(queueSize, sizeof(MotionCmd));
    }

    QueueHandle_t getQueueHandle() const
    {
        return _montionQueue;
    }

  protected:
    void run() override
    {
        MotionCmd currentCmd;

        while (true)
        {
            if (xQueueReceive(_montionQueue, &currentCmd, portMAX_DELAY) == pdPASS)
            {
                StepCmd stepcmd = calculateSteps(currentCmd);

                _controller->addMove(stepcmd);
            }
        }
    }

  private:

    StepCmd calculateSteps(const MotionCmd& cmd)
    {
        StepCmd stepCmd;

        int32_t targetStepsX =
            static_cast<int32_t>(cmd.x.value_or(_state.currentX) * _config.stepsPerMM_XY);
        int32_t targetStepsY =
            static_cast<int32_t>(cmd.y.value_or(_state.currentY) * _config.stepsPerMM_XY);
        int32_t targetStepsZ =
            static_cast<int32_t>(cmd.z.value_or(_state.currentZ) * _config.stepsPerMM_Z);

        stepCmd.dX = std::abs(targetStepsX - _state.stepX);
        stepCmd.dY = std::abs(targetStepsY - _state.stepY);
        stepCmd.dZ = std::abs(targetStepsZ - _state.stepZ);

        stepCmd.dirMask = 0;
        if(targetStepsX >= _state.stepX) stepCmd.dirMask |=1;   // bit for x
        if(targetStepsY >= _state.stepY) stepCmd.dirMask |=2;   // bit for y
        if(targetStepsZ >= _state.stepZ) stepCmd.dirMask |=4;   // bit for z

        _state.stepX = targetStepsX;
        _state.stepY = targetStepsY;
        _state.stepZ = targetStepsZ;
        _state.currentX = cmd.x.value_or(_state.currentX);
        _state.currentY = cmd.y.value_or(_state.currentY);
        _state.currentZ = cmd.z.value_or(_state.currentZ);

        stepCmd.totalSteps = std::max({stepCmd.dX, stepCmd.dY, stepCmd.dZ});

        stepCmd.startArr = _config.startingSpeedToArr;
        stepCmd.targetArr = _config.defaultTargetspeedToArr;
        stepCmd.accelSteps = _config.acceleration.steps;
        stepCmd.decelSteps = stepCmd.totalSteps -  _config.acceleration.steps; 

        if (stepCmd.totalSteps == 0) return stepCmd;

        return stepCmd;
    }

    QueueHandle_t _montionQueue;
    static constexpr uint8_t queueSize = 20;

    const MachineConfig& _config;

    MachineState _state;
    MotionController<T>* _controller;
};