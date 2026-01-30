#pragma once

#include "common/cppTask.hpp"
#include "common/montionTypes.hpp"
#include "common/systemError.hpp"
#include "main.h"
#include "stm32f4xx_hal_def.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_tim.h"

template <typename driver> class MotionController
{
  public:
    MotionController(TIM_HandleTypeDef* tim, driver& axisX, driver& axisY, driver& axisZ)
        : _tim(tim)
        , _axisX(axisX)
        , _axisY(axisY)
        , _axisZ(axisZ)
    {
        instance = this;
    }

    static MotionController* instance;

    Result<void> init()
    {
        if (auto res = _axisX.init(); !res.isOk())
            return res;
        if (auto res = _axisY.init(); !res.isOk())
            return res;
        if (auto res = _axisZ.init(); !res.isOk())
            return res;

        if (HAL_TIM_Base_Start_IT(_tim) != HAL_OK)
        {
            return Result<void>(ErrorCode::Controller_TimInitFail);
        }
        return Result<void>();
    }

    bool isBusy()
    {
        return _currentCmd.isAcitve;
    }

    void executeAction(const StepCmd& stepCmd)
    {
        _dda.accX = _dda.accY = _dda.accZ = 0;
        _dda.currentSteps = 0;
        _dda.totalSteps = stepCmd.totalSteps;

        _axisX.setDirection(stepCmd.dirX);
        _axisY.setDirection(stepCmd.dirY);
        _axisZ.setDirection(stepCmd.dirZ);

        _currentCmd.stepX = stepCmd.dX;
        _currentCmd.stepY = stepCmd.dY;
        _currentCmd.stepZ = stepCmd.dZ;

        _currentCmd.isAcitve = true;
    }

    void tick() // pls dont use freertos here
    {
        //HAL_GPIO_TogglePin(STEP_X_GPIO_Port, STEP_X_Pin);

        if (!_currentCmd.isAcitve)
            return;

        if (_dda.totalSteps <= 0)
        {
            _currentCmd.isAcitve = false;
            return;
        }

        bool stepMade = false;

        _dda.accX += _currentCmd.stepX;
        if (_dda.accX >= _dda.totalSteps)
        {
            _dda.accX -= _dda.totalSteps;
            _axisX.stepHigh();
            stepMade = true;
        }

        _dda.accY += _currentCmd.stepY;
        if (_dda.accY >= _dda.totalSteps)
        {
            _dda.accY -= _dda.totalSteps;
            _axisY.stepHigh();
            stepMade = true;
        }

        _dda.accZ += _currentCmd.stepZ;
        if (_dda.accZ >= _dda.totalSteps)
        {
            _dda.accZ -= _dda.totalSteps;
            _axisZ.stepHigh();
            stepMade = true;
        }

        if (stepMade)
        {
            for (volatile int i = 0; i < 5; i++)
                ; // dealy for tmc to see slope
            _axisX.stepLow();
            _axisY.stepLow();
            _axisZ.stepLow();
        }

        _dda.currentSteps++;
        if (_dda.currentSteps >= _dda.totalSteps)
        {
            _currentCmd.isAcitve = false;
        }
    }

  private:
    TIM_HandleTypeDef* _tim;
    driver& _axisX;
    driver& _axisY;
    driver& _axisZ;

    ControllerCmd _currentCmd;

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