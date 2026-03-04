#pragma once
#include "common/cppTask.hpp"
#include "FreeRTOS.h"
#include "semphr.h"
#include "hardware/motionController.hpp"
#include "queue.h"
#include <algorithm>
#include <math.h>
#include "common/motionTypes.hpp"
#include "common/machineConfig.hpp"

/**
 * @file planner.hpp
 * @brief Motion planner converting MotionCmd queue entries into StepCmd segments.
 */

/**
 * @brief Planner task converting MotionCmd to StepCmd.
 */
template <typename T> class Planner : public CppTask
{
  public:
        /**
         * @brief Construct planner.
         * @param controller Motion controller receiving generated StepCmd entries.
         * @param config Machine configuration with limits and acceleration data.
         */
    Planner(MotionController<T>* controller, const MachineConfig& config)
        : CppTask("Planner", 1024, 2)
        , _config(config)
        , _controller(controller)
    {
        _state = {};
        _motionQueue = xQueueCreate(QUEUE_SIZE, sizeof(MotionCmd));
        _stateMutex = xSemaphoreCreateMutex();
    }

    /** @brief Return queue handle for motion input commands. */
    QueueHandle_t getQueueHandle() const
    {
        return _motionQueue;
    }

    /** @brief Return a thread-safe snapshot of current machine state. */
    MachineState getCurrentState()
    {
        MachineState tempState;
        if (xSemaphoreTake(_stateMutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            tempState = _state;
            xSemaphoreGive(_stateMutex);
        }
        return tempState;
    }

  protected:
        /** @brief Main planner task loop. */
    void run() override
    {
        MotionCmd currentCmd;

        while (true)
        {
            if (xQueueReceive(_motionQueue, &currentCmd, portMAX_DELAY) == pdPASS)
            {
                StepCmd stepcmd = calculateSteps(currentCmd);

                _controller->addMove(stepcmd);
            }
        }
    }

  private:
        /** @brief Target state in both position and step domain. */
    struct Target
    {
        int32_t stepX, stepY, stepZ;
        float posX, posY, posZ;
    };

    /** @brief Convert motion command to step command. */
    StepCmd calculateSteps(const MotionCmd& cmd)
    {
        StepCmd stepCmd = {};

        if (xSemaphoreTake(_stateMutex, portMAX_DELAY) == pdTRUE)
        {

            if (cmd.motion == MotionType::SetHome )
            {
                setHome(cmd);
                xSemaphoreGive(_stateMutex);
                return stepCmd;
            }

            auto target = calculateTarget(cmd);


            if (cmd.motion == MotionType::Homing) 
            {
                _state.machineStepX = 0;
                _state.machineStepY = 0;
                _state.machineStepZ = 19800;
            }
            else
            {
                if(!checkSoftLimits(target)){
                    ErrorHandler::report(ErrorCode::Machine_SoftLimitReached);
                    xSemaphoreGive(_stateMutex);
                    return stepCmd; 
                }
            }

            stepCmd.dX = std::abs(target.stepX - _state.stepX);
            stepCmd.dY = std::abs(target.stepY - _state.stepY);
            stepCmd.dZ = std::abs(target.stepZ - _state.stepZ);

            stepCmd.totalSteps = std::max({stepCmd.dX, stepCmd.dY, stepCmd.dZ});

            stepCmd.dirMask = 0;
            if (target.stepX >= _state.stepX)
                stepCmd.dirMask |= 1;
            if (target.stepY >= _state.stepY)
                stepCmd.dirMask |= 2;
            if (target.stepZ >= _state.stepZ)
                stepCmd.dirMask |= 4;

            updateState(target);


            MotionCmd nextCmd;
            stepCmd.slowDown = true;
            if (xQueuePeek(_motionQueue, &nextCmd, 0) == pdPASS)
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

            if(cmd.motion == MotionType::Homing)
            {
                stepCmd.homing = true;
                stepCmd.totalSteps = 320000;
                stepCmd.dX = 320000;
                stepCmd.dY = 320000;
                stepCmd.dZ = 320000;                
            }

            applyMotionRamp(stepCmd, cmd);

            xSemaphoreGive(_stateMutex);
        }

        if (stepCmd.totalSteps == 0)
            return stepCmd;

        return stepCmd;
    }

    /** @brief Validate software limits and update machine-step state. */
    bool checkSoftLimits(const Target& target){
    int32_t diffX = target.stepX - _state.stepX;
    int32_t diffY = target.stepY - _state.stepY;
    int32_t diffZ = target.stepZ - _state.stepZ;

    int32_t potentialMachineStepX = _state.machineStepX + diffX;
    int32_t potentialMachineStepY = _state.machineStepY + diffY;
    int32_t potentialMachineStepZ = _state.machineStepZ + diffZ;

    if (potentialMachineStepX < 0 || potentialMachineStepX > _config.xStepMax ||
        potentialMachineStepY < 0 || potentialMachineStepY > _config.yStepMax ||
        potentialMachineStepZ < 0 || potentialMachineStepZ > _config.zStepMax) 
    {
        return false;
    }

    _state.machineStepX = potentialMachineStepX;
    _state.machineStepY = potentialMachineStepY;
    _state.machineStepZ = potentialMachineStepZ;

    return true;
    }

    /** @brief Commit target to current planner state. */
    void updateState(const Target& target)
    {
        _state.stepX = target.stepX;
        _state.stepY = target.stepY;
        _state.stepZ = target.stepZ;
        _state.currentX = target.posX;
        _state.currentY = target.posY;
        _state.currentZ = target.posZ;
    }

    /** @brief Set current position as logical home. */
    void setHome(const MotionCmd& cmd)
    {
        _state.currentX = cmd.x.value_or(_state.currentX);
        _state.currentY = cmd.y.value_or(_state.currentY);
        _state.currentZ = cmd.z.value_or(_state.currentZ);
        _state.stepX = static_cast<int32_t>(_state.currentX * _config.stepsPerMM_XY);
        _state.stepY = static_cast<int32_t>(_state.currentY * _config.stepsPerMM_XY);
        _state.stepZ = static_cast<int32_t>(_state.currentZ * _config.stepsPerMM_Z);
    }

    /** @brief Build target from absolute/relative motion command. */
    Target calculateTarget(const MotionCmd& cmd)
    {
        Target target{.stepX = _state.stepX,
                      .stepY = _state.stepY,
                      .stepZ = _state.stepZ,
                      .posX = _state.currentX,
                      .posY = _state.currentY,
                      .posZ = _state.currentZ};

        if (cmd.x.has_value())
        {
            target.posX =
                (cmd.motion == MotionType::Move || cmd.motion == MotionType::Stop) ? (_state.currentX + *cmd.x) : *cmd.x;
        }
        if (cmd.y.has_value())
        {
            target.posY =
                (cmd.motion == MotionType::Move || cmd.motion == MotionType::Stop) ? (_state.currentY + *cmd.y) : *cmd.y;
        }
        if (cmd.z.has_value())
        {
            target.posZ =
                (cmd.motion == MotionType::Move || cmd.motion == MotionType::Stop) ? (_state.currentZ + *cmd.z) : *cmd.z;
        }

        target.stepX = static_cast<int32_t>(target.posX * _config.stepsPerMM_XY);
        target.stepY = static_cast<int32_t>(target.posY * _config.stepsPerMM_XY);
        target.stepZ = static_cast<int32_t>(target.posZ * _config.stepsPerMM_Z);

        return target;
    }

    /** @brief Apply acceleration profile for selected motion type. */
    void applyMotionRamp(StepCmd& stepCmd, const MotionCmd& cmd)
    {
        if (cmd.motion == MotionType::Linear)
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
        else if (cmd.motion == MotionType::Move || cmd.motion == MotionType::Homing)
        {
            stepCmd.startArr = _config.startingSpeedToArr;
            stepCmd.targetArr = _config.slowTargetSpeedToArr;
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
            stepCmd.slowDown = false;
        }
        else if (cmd.motion == MotionType::Stop)
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

    }

    QueueHandle_t _motionQueue;
    static constexpr uint8_t QUEUE_SIZE = 20;

    const MachineConfig& _config;

    MachineState _state;
    MotionController<T>* _controller;

    mutable SemaphoreHandle_t _stateMutex;
};