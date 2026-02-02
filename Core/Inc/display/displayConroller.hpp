#pragma once
#include "common/cppTask.hpp"
#include "common/lcdConstants.hpp"
#include "common/systemError.hpp"
#include "display/view.hpp"
#include "hardware/xpt2046.hpp"

template <typename LcdDriver, typename TouchDriver> class DisplayController : public CppTask
{
  public:
    DisplayController(LcdDriver& screenDriver, TouchDriver& touchDriver)
        : CppTask("Gui", 4096, 5)
        , _screenDriver(screenDriver)
        , _touchDriver(touchDriver)
    {
    }

  protected:
    void run() override
    {
        if (auto res = _screenDriver.init(); !res.isOk())
            ErrorHandler::report(res.error);

        GuiEvent event;
        TouchState touchState = TouchState::IDLE;
        uint8_t holdCnt = 0;
        Point lastPoint = {0, 0};

        while (1)
        {
            if (xQueueReceive(guiEventQueue, &event, 0) == pdPASS)
            {
                if (event == GuiEvent::ShowControls)
                    _currentView = &_controls;
                else if (event == GuiEvent::ShowMain)
                    _currentView = &_mainMenu;

                _screenDriver.fillScreen(Colors::Background);
                _currentView->forceRedraw();
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
                        touchState = TouchState::IDLE;
                    }
                    break;

                case TouchState::HOLD:
                    if (pressed)
                    {
                        if (++holdCnt >= 5) 
                        {
                            _currentView->handleHold(_touchDriver.getPoint());
                            holdCnt = 0;
                        }
                    }
                    else
                    {
                        touchState = TouchState::IDLE;
                    }
                    break;
                }
            }

            _currentView->drawAll(_screenDriver);
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }

  private:
    LcdDriver& _screenDriver;
    MainMenu _mainMenu;
    Controls _controls;
    View* _currentView = &_mainMenu;
    TouchDriver& _touchDriver;
};
