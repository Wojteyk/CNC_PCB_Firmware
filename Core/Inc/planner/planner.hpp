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
        , _controller(controller)
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
    struct Target
    {
        int32_t stepX, stepY, stepZ;
        float posX, posY, posZ;
    };

    StepCmd calculateSteps(const MotionCmd& cmd)
    {
        StepCmd stepCmd;

        auto target = calculateTarget(cmd);

        stepCmd.dX = std::abs(target.stepX - _state.stepX);
        stepCmd.dY = std::abs(target.stepY - _state.stepY);
        stepCmd.dZ = std::abs(target.stepZ - _state.stepZ);

        stepCmd.totalSteps = std::max({stepCmd.dX, stepCmd.dY, stepCmd.dZ});

        stepCmd.dirMask = 0;
        if (target.stepX >= _state.stepX)
            stepCmd.dirMask |= 1; // bit for x
        if (target.stepY >= _state.stepY)
            stepCmd.dirMask |= 2; // bit for y
        if (target.stepZ >= _state.stepZ)
            stepCmd.dirMask |= 4; // bit for z

        updateState(target);

        MotionCmd nextCmd;
        stepCmd.slowDown = true;
        if (xQueuePeek(_montionQueue, &nextCmd, 0) == pdPASS)
        {

            auto nextTarget = calculateTarget(nextCmd);
            uint8_t nextDirMask = 0;
            if (nextTarget.stepX >= _state.stepX)
                nextDirMask |= 1;
            if (nextTarget.stepY >= _state.stepY)
                nextDirMask |= 2;
            if (nextTarget.stepZ >= _state.stepZ)
                nextDirMask |= 4;

            if (nextDirMask == stepCmd.dirMask && nextCmd.motion == cmd.motion)
            {
                stepCmd.slowDown = false;
            }
        }

        applyMotionRamp(stepCmd, cmd);

        if (stepCmd.totalSteps == 0)
            return stepCmd;

        return stepCmd;
    }

    void updateState(Target target)
    {
        _state.stepX = target.stepX;
        _state.stepY = target.stepY;
        _state.stepZ = target.stepZ;
        _state.currentX = target.posX;
        _state.currentY = target.posY;
        _state.currentZ = target.posZ;
    }

    Target calculateTarget(const MotionCmd& cmd)
    {
        Target target;

        if (cmd.x.has_value())
        {
            target.posX =
                (cmd.motion == MotionType::MoveOnce) ? (_state.currentX + *cmd.x) : *cmd.x;
        }
        if (cmd.y.has_value())
        {
            target.posY =
                (cmd.motion == MotionType::MoveOnce) ? (_state.currentY + *cmd.y) : *cmd.y;
        }
        if (cmd.z.has_value())
        {
            target.posZ =
                (cmd.motion == MotionType::MoveOnce) ? (_state.currentZ + *cmd.z) : *cmd.z;
        }

        target.stepX = static_cast<int32_t>(target.posX * _config.stepsPerMM_XY);
        target.stepY = static_cast<int32_t>(target.posY * _config.stepsPerMM_XY);
        target.stepZ = static_cast<int32_t>(target.posZ * _config.stepsPerMM_Z);

        return target;
    }

    void applyMotionRamp(StepCmd& stepCmd, const MotionCmd& cmd)
    {
        if (cmd.motion == MotionType::Linear )
        {
            stepCmd.startArr = _config.startingSpeedToArr;
            stepCmd.targetArr = _config.defaultTargetSpeedToArr;
            stepCmd.accIncrease = _config.defaultAcceleration.increase;
            if (stepCmd.totalSteps <= _config.defaultAcceleration.steps * 2)
            {
                stepCmd.accelSteps = stepCmd.totalSteps / 2;
                stepCmd.decelSteps = stepCmd.totalSteps / 2;
            }
            else
            {
                stepCmd.accelSteps = _config.defaultAcceleration.steps;
                stepCmd.decelSteps = stepCmd.totalSteps - _config.defaultAcceleration.steps;
            }
        }
        else if (cmd.motion == MotionType::Rapid)
        {
            stepCmd.startArr = _config.startingSpeedToArr;
            stepCmd.targetArr = _config.rapidTargetSpeedToArr;
            stepCmd.accIncrease = _config.rapidAcceleration.increase;
            if (stepCmd.totalSteps <= _config.rapidAcceleration.steps * 2)
            {
                stepCmd.accelSteps = stepCmd.totalSteps / 2;
                stepCmd.decelSteps = stepCmd.totalSteps / 2;
            }
            else
            {
                stepCmd.accelSteps = _config.rapidAcceleration.steps;
                stepCmd.decelSteps = stepCmd.totalSteps - _config.rapidAcceleration.steps;
            }
        }
        else if (cmd.motion == MotionType::MoveOnce) {
            stepCmd.startArr = _config.startingSpeedToArr;
            stepCmd.targetArr = _config.defaultTargetSpeedToArr;
            stepCmd.accIncrease = _config.defaultAcceleration.increase;
            if (stepCmd.totalSteps <= _config.defaultAcceleration.steps * 2)
            {
                stepCmd.accelSteps = stepCmd.totalSteps / 2;
                stepCmd.decelSteps = stepCmd.totalSteps / 2;
            }
            else
            {
                stepCmd.accelSteps = _config.defaultAcceleration.steps;
                stepCmd.decelSteps = stepCmd.totalSteps - _config.defaultAcceleration.steps;
            }
        }
    }

    QueueHandle_t _montionQueue;
    static constexpr uint8_t queueSize = 20;

    const MachineConfig& _config;

    MachineState _state;
    MotionController<T>* _controller;
};