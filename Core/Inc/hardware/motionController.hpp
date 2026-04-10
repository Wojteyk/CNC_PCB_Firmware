#pragma once

#include "common/cppTask.hpp"
#include "common/motionTypes.hpp"
#include "common/systemError.hpp"
#include "common/machineConfig.hpp"
#include "main.h"
#include "stm32f4xx_hal_def.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_tim.h"
#include "FreeRTOS.h"
#include "queue.h"
#include <algorithm>

/**
 * @file motionController.hpp
 * @brief Low-level step generation and runtime execution of StepCmd segments.
 */

static constexpr uint8_t HW_QUEUE_SIZE = 32;

/** @brief Motion controller runtime state. */
enum struct WorkingState
{
    Idle = 0,
    Working = 1,
    Homing = 2,
    Error = 3,
};

/**
 * @brief Step pulse generator and command executor.
 */
template <typename driver> class MotionController
{
  public:
        /**
         * @brief Construct motion controller.
         * @param tim Timer used as step interrupt source.
         * @param axisX Axis driver for X.
         * @param axisY Axis driver for Y.
         * @param axisZ Axis driver for Z.
         * @param config Machine configuration reference.
         */
    MotionController(TIM_HandleTypeDef* tim,
                     driver& axisX,
                     driver& axisY,
                     driver& axisZ,
                     const MachineConfig& config)
        : _tim(tim)
        , _axisX(axisX)
        , _axisY(axisY)
        , _axisZ(axisZ)
        , _config(config)
    {
        _stepsQueue = xQueueCreate(HW_QUEUE_SIZE, sizeof(StepCmd));
        instance = this;
    }

    static MotionController* instance;

    /** @brief Initialize drivers, queue and timer ISR. */
    Result<void> init()
    {
        if (_stepsQueue == NULL)
            return Result<void>(ErrorCode::Controller_QueueCreateFail);

        if (auto res = _axisX.init(); !res.isOk())
            return res;
        if (auto res = _axisY.init(); !res.isOk())
            return res;
        if (auto res = _axisZ.init(); !res.isOk())
            return res;

        _currentArr = _config.startingSpeedToArr;
        _currentSpeedSps = arrToSpeed(_currentArr);
        _currentStartSpeedSps = _currentSpeedSps;
        _currentTargetSpeedSps = _currentSpeedSps;
        _currentAccelSps2 = 0.0f;
        __HAL_TIM_SET_AUTORELOAD(_tim, _currentArr);

        if (HAL_TIM_Base_Start_IT(_tim) != HAL_OK)
        {
            return Result<void>(ErrorCode::Controller_TimInitFail);
        }
        return Result<void>();
    }

    /**
     * @brief Queue one step command.
     * @param stepCmd Prepared command from planner.
     */
    void addMove(const StepCmd& stepCmd)
    {

        xQueueSend(_stepsQueue, &stepCmd, portMAX_DELAY);
    }

    /** @brief Timer tick handler called from ISR context. */
    void tick()
    {
        if (_workingState == WorkingState::Idle)
        {
            if (!tryLoadNextCommand())
                return;
        }
        else if (_workingState == WorkingState::Homing)
        {
            _currentCmd.dZ = 0;
            if(HAL_GPIO_ReadPin(XAXIS_ENDSW_GPIO_Port,XAXIS_ENDSW_Pin))
            {
                 _currentCmd.dX = 0;
            }
            if(HAL_GPIO_ReadPin(YAXIS_ENDSW_GPIO_Port,YAXIS_ENDSW_Pin))
            {
                _currentCmd.dY = 0;
            }
            if(_currentCmd.dX ==0 && _currentCmd.dY == 0)
            {
                _currentCmd.dZ = _dda.totalSteps;
                if(!HAL_GPIO_ReadPin(ZAXIS_ENDSW_GPIO_Port, ZAXIS_ENDSW_Pin))
                {
                    _dda.totalSteps = 0; 
                    _dda.currentSteps = 0;
                }
                
            }
        } 

        if (_dda.totalSteps <= 0)
        {
            _workingState = WorkingState::Idle;
            return;
        }

        bool stepMade = false;

        _dda.accX += _currentCmd.dX;
        if (_dda.accX >= _dda.totalSteps)
        {
            _dda.accX -= _dda.totalSteps;
            _axisX.stepHigh();
            stepMade = true;
        }

        _dda.accY += _currentCmd.dY;
        if (_dda.accY >= _dda.totalSteps)
        {
            _dda.accY -= _dda.totalSteps;
            _axisY.stepHigh();
            stepMade = true;
        }

        _dda.accZ += _currentCmd.dZ;
        if (_dda.accZ >= _dda.totalSteps)
        {
            _dda.accZ -= _dda.totalSteps;
            _axisZ.stepHigh();
            stepMade = true;
        }

        if (stepMade)
        {
            for (volatile int i = 0; i < 10; i++)
                ;
            _axisX.stepLow();
            _axisY.stepLow();
            _axisZ.stepLow();
        }

        _dda.currentSteps++;

        bool arrChanged = false;

        const bool inAccel = (_dda.currentSteps < _currentCmd.accelSteps);
        const bool inDecel = (_dda.currentSteps >= _currentCmd.decelSteps && _currentCmd.slowDown);
        if ((inAccel || inDecel) && _currentAccelSps2 > 0.0f)
        {
            const float dt = static_cast<float>(_currentArr) / static_cast<float>(_config.stepTimerClockHz);

            if (inAccel && _currentSpeedSps < _currentTargetSpeedSps)
            {
                _currentSpeedSps += (_currentAccelSps2 * dt);
                if (_currentSpeedSps > _currentTargetSpeedSps)
                {
                    _currentSpeedSps = _currentTargetSpeedSps;
                }
                arrChanged = true;
            }
            else if (inDecel && _currentSpeedSps > _currentStartSpeedSps)
            {
                _currentSpeedSps -= (_currentAccelSps2 * dt);
                if (_currentSpeedSps < _currentStartSpeedSps)
                {
                    _currentSpeedSps = _currentStartSpeedSps;
                }
                arrChanged = true;
            }

            if (arrChanged)
            {
                _currentArr = speedToArr(_currentSpeedSps);
            }
        }

        if (arrChanged)
        {
            __HAL_TIM_SET_AUTORELOAD(_tim, _currentArr);
        }

        if (_dda.currentSteps >= _dda.totalSteps)
        {
            if (!tryLoadNextCommand())
            {
                _workingState = WorkingState::Idle;
            }
        }
    }

  private:
    bool tryLoadNextCommand()
    {
        StepCmd nextCmd;
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        if (xQueueReceiveFromISR(_stepsQueue, &nextCmd, &xHigherPriorityTaskWoken))
        {
            _dda.accX = _dda.accY = _dda.accZ = 0;
            _dda.currentSteps = 0;
            _dda.totalSteps = nextCmd.totalSteps;
            
            if(nextCmd.homing)
            {
                _workingState = WorkingState::Homing;
                _axisX.setDirection(0);
                _axisY.setDirection(0);
                _axisZ.setDirection(0);
            }
            else{
                _workingState = WorkingState::Working;
                _axisX.setDirection((nextCmd.dirMask & 0x01) != 0);
                _axisY.setDirection((nextCmd.dirMask & 0x02) != 0);
                _axisZ.setDirection((nextCmd.dirMask & 0x04) != 0);
            }

            _currentCmd = nextCmd;

            const uint32_t cmdStartArr =
                (_currentCmd.startArr > 0) ? static_cast<uint32_t>(_currentCmd.startArr) : 1U;
            const uint32_t cmdTargetArr =
                (_currentCmd.targetArr > 0) ? static_cast<uint32_t>(_currentCmd.targetArr) : 1U;

            _currentStartSpeedSps = arrToSpeed(cmdStartArr);
            _currentTargetSpeedSps = arrToSpeed(cmdTargetArr);

            if (_currentArr == 0U)
            {
                _currentArr = cmdStartArr;
                __HAL_TIM_SET_AUTORELOAD(_tim, _currentArr);
            }

            _currentSpeedSps = arrToSpeed(_currentArr);

            if (_currentCmd.accelSteps > 0 && _currentTargetSpeedSps > _currentStartSpeedSps)
            {
                const float startV2 = _currentStartSpeedSps * _currentStartSpeedSps;
                const float targetV2 = _currentTargetSpeedSps * _currentTargetSpeedSps;
                _currentAccelSps2 =
                    (targetV2 - startV2) / (2.0f * static_cast<float>(_currentCmd.accelSteps));
            }
            else
            {
                _currentAccelSps2 = 0.0f;
            }
            

            return true;
        }

        return false;
    }

    float arrToSpeed(uint32_t arr) const
    {
        const uint32_t safeArr = (arr == 0U) ? 1U : arr;
        return static_cast<float>(_config.stepTimerClockHz) / static_cast<float>(safeArr);
    }

    uint32_t speedToArr(float speedSps) const
    {
        const float safeSpeed = std::max(speedSps, 1.0f);
        const float arr = static_cast<float>(_config.stepTimerClockHz) / safeSpeed;
        return static_cast<uint32_t>(std::max(arr, 1.0f));
    }

    void startHoming()
    {
        
    }

    TIM_HandleTypeDef* _tim;
    driver& _axisX;
    driver& _axisY;
    driver& _axisZ;

    volatile WorkingState _workingState = WorkingState::Idle;
    volatile uint32_t _currentArr;
    float _currentSpeedSps = 0.0f;
    float _currentStartSpeedSps = 0.0f;
    float _currentTargetSpeedSps = 0.0f;
    float _currentAccelSps2 = 0.0f;

    StepCmd _currentCmd;
    QueueHandle_t _stepsQueue;

    const MachineConfig& _config;

    struct DDA
    {
        volatile int32_t currentSteps;
        volatile int32_t totalSteps;

        volatile int32_t accX;
        volatile int32_t accY;
        volatile int32_t accZ;

    } _dda;
};
template <typename driver> MotionController<driver>* MotionController<driver>::instance = nullptr;