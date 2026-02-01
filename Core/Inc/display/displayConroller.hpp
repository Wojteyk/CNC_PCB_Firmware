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

        _currentView = &_mainMenu;
        _screenDriver.fillScreen(Colors::DarkGrey);
        bool alreadyPressed = false;
        while (1)
        {
            if (_currentView)
            {
                if (_touchDriver.isPressed())
                {
                    if (!alreadyPressed)
                    {
                        _currentView->handleTouch(_touchDriver.getPoint());
                        alreadyPressed = true;
                    }
                }
                else
                {
                    alreadyPressed = false;
                }

                _currentView->drawAll(_screenDriver);
            }
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }

  private:
    LcdDriver& _screenDriver;
    MainMenu _mainMenu;
    View* _currentView = &_mainMenu;
    TouchDriver& _touchDriver;
};
