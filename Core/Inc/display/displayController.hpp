#pragma once
#include "common/cppTask.hpp"
#include "common/lcdConstants.hpp"
#include "common/systemError.hpp"
#include "display/view.hpp"
#include "hardware/xpt2046.hpp"
#include "planner/planner.hpp"

/**
 * @file displayController.hpp
 * @brief FreeRTOS GUI task integrating display, touch and planner state.
 */

/**
 * @brief GUI task that manages views and touch input.
 */
template <typename LcdDriver, typename TouchDriver, typename T>
class DisplayController : public CppTask
{
  public:
        /**
         * @brief Construct display controller.
         * @param screenDriver LCD drawing driver.
         * @param touchDriver Touch input driver.
         * @param planner Motion planner used to read current coordinates.
         * @param gcodeQueue Queue used by GUI buttons to send G-code strings.
         */
    DisplayController(LcdDriver& screenDriver,
                      TouchDriver& touchDriver,
                      Planner<T>& planner,
                      QueueHandle_t gcodeQueue)
        : CppTask("Gui", 4096, 5)
        , _screenDriver(screenDriver)
        , _touchDriver(touchDriver)
        , _planner(planner)
        , _mainMenu(gcodeQueue)
        , _controls(gcodeQueue)
        , _errorPage()
        , _currentView(&_mainMenu)
    {
    }

  protected:
        /**
         * @brief Main GUI task loop.
         * @details Initializes hardware, processes GUI events, reads touch state,
         * updates active view and renders widgets periodically.
         */
    void run() override
    {
        if (auto res = _screenDriver.init(); !res.isOk())
            ErrorHandler::report(res.error);

        if (auto res = _touchDriver.init(); !res.isOk())
            ErrorHandler::report(res.error);

        GuiEvent event;
        TouchState touchState = TouchState::IDLE;
        uint8_t holdCnt = 0;
        Point lastPoint = {0, 0};
        _screenDriver.fillScreen(Colors::Background);

        while (1)
        {
            MachineState targetState = _planner.getCurrentState();

            if (xQueueReceive(guiEventQueue, &event, 0) == pdPASS)
            {
                if (event == GuiEvent::ShowControls)
                    _currentView = &_controls;
                else if (event == GuiEvent::ShowMain)
                    _currentView = &_mainMenu;
                else if (event == GuiEvent::ShowError)
                {
                    _errorPage.setError(g_lastError);
                    _currentView = &_errorPage;
                }

                _screenDriver.fillScreen(Colors::Background);
                _currentView->forceRedraw();
            }

            if (_currentView == &_controls)
            {
                _controls.updateCurrentPos(targetState);
            }

            if (_currentView)
            {
                bool pressed = _touchDriver.isPressed();

                switch (touchState)
                {
                case TouchState::IDLE:
                    if (pressed)
                    {
                        lastPoint = _touchDriver.getPoint();
                        _currentView->handleTouch(lastPoint);

                        touchState = TouchState::PRESSED;
                        holdCnt = 0;
                    }
                    break;

                case TouchState::PRESSED:
                    if (pressed)
                    {
                        if (++holdCnt > 8)
                        {
                            touchState = TouchState::HOLD;
                            holdCnt = 0;
                        }
                    }
                    else
                    {
                        touchState = TouchState::RELEASED;
                    }
                    break;

                case TouchState::HOLD:
                    if (pressed)
                    {
                        if (++holdCnt >= 3)
                        {
                            _currentView->handleHold(_touchDriver.getPoint());
                            holdCnt = 0;
                        }
                    }
                    else
                    {
                        touchState = TouchState::RELEASED;
                    }
                    break;
                case TouchState::RELEASED:
                    _currentView->handleRelease();
                    touchState = TouchState::IDLE;
                    break;
                }
            }

            _currentView->drawAll(_screenDriver);
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }

  private:
    LcdDriver& _screenDriver;
    TouchDriver& _touchDriver;
    Planner<T>& _planner;

    MainMenu _mainMenu;
    Controls _controls;
    ErrorPage _errorPage;

    View* _currentView;
};
