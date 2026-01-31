#pragma once
#include "common/cppTask.hpp"
#include "common/lcdConstants.hpp"
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
        if (auto res = _screenDriver.init(); !res.isOk())
            ErrorHandler::report(res.error);

        if (auto res = drawMainPage(); !res.isOk())
        ErrorHandler::report(res.error);
        while (1)
        {
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }

  private:
    Result<void> drawMainPage()
    {
        if (auto res = _screenDriver.fillScreen(Colors::DarkGrey); !res.isOk())
            return res;

        if (auto res = _screenDriver.drawString(132, 20, "CNC  PCB", Colors::Blue, Colors::DarkGrey);
            !res.isOk())
            return res;

        if (auto res = _screenDriver.fillRect(100, 40, 120, 40,  Colors::Cyan);
            !res.isOk())
            return res;

        if (auto res = _screenDriver.drawString(145, 55, "START", Colors::DarkGrey, Colors::Cyan);
            !res.isOk())
            return res;

        if (auto res = _screenDriver.fillRect(100, 120, 120, 40,  Colors::Cyan);
            !res.isOk())
            return res;
        
        return Result<void>(ErrorCode::Ok);
    }

    lcdDriver& _screenDriver;
};
