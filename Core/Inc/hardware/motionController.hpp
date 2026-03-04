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

static constexpr uint8_t HW_QUEUE_SIZE = 32;

enum struct WorkingState
{
    Idle = 0,
    Working = 1,
    Homing = 2,
    Error = 3,
};

template <typename driver> class MotionController
{
  public:
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
        __HAL_TIM_SET_AUTORELOAD(_tim, _currentArr); // safe feedrate at the start

        if (HAL_TIM_Base_Start_IT(_tim) != HAL_OK)
        {
            return Result<void>(ErrorCode::Controller_TimInitFail);
        }
        return Result<void>();
    }

    void addMove(const StepCmd& stepCmd)
    {

        xQueueSend(_stepsQueue, &stepCmd, portMAX_DELAY);
    }

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
                ; // dealy for tmc to see slope
            _axisX.stepLow();
            _axisY.stepLow();
            _axisZ.stepLow();
        }

        _dda.currentSteps++;

        bool arrChanged = false;

        if (_dda.currentSteps < _currentCmd.accelSteps)
        {
            if (_currentArr > _currentCmd.targetArr)
            {
                if (_currentArr > (_currentCmd.targetArr + _currentCmd.accIncrease))
                    _currentArr -= _currentCmd.accIncrease;
                else
                    _currentArr = _currentCmd.targetArr;

                arrChanged = true;
            }
        }

        else if (_dda.currentSteps >= _currentCmd.decelSteps && _currentCmd.slowDown)
        {
            if (_currentArr < _currentCmd.startArr)
            {
                if (_currentArr < (_currentCmd.startArr - _currentCmd.accIncrease))
                    _currentArr += _currentCmd.accIncrease;
                else
                    _currentArr = _currentCmd.startArr;
                arrChanged = true;
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
                //__HAL_TIM_SET_AUTORELOAD(_tim, _config.startingSpeedToArr);
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
            

            return true;
        }

        return false;
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