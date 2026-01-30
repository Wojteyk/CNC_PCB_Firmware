#pragma once
#include "common/cppTask.hpp"
#include "common/colors.hpp"
#include "common/systemError.hpp"

template <typename lcdDriver> class DisplayController : public CppTask
{
  public:
    DisplayController(lcdDriver& screenDriver)
        : CppTask("Gui", 4096, 5)
        , _screenDriver(screenDriver)
    {
    }

    protected:
        void run() override
        {
            if(auto res = _screenDriver.init(); !res.isOk())
                ErrorHandler::report(res.error);

            while(1)
            {
                if(auto res = _screenDriver.fillScreen(Colors::Red);!res.isOk())
                    ErrorHandler::report(res.error);
                vTaskDelay(pdMS_TO_TICKS(2000));
            }
        }

  private:
    lcdDriver& _screenDriver;
};



