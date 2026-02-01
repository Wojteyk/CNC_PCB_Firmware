#pragma once
#include "display/iGuiDriver.hpp"
#include "display/widgets.hpp"
#include "FreeRTOS.h"
#include "queue.h"

extern QueueHandle_t gcodeQueue;

class View
{
  public:
    virtual ~View() = default;

    void drawAll(IGuiDriver& driver)
    {
        for (uint8_t i = 0; i < _widgetCnt; i++)
        {
            _widgets[i]->draw(driver);
        }
    }

    void handleTouch(Point p)
    {
        for (uint8_t i = 0; i < _widgetCnt; i++)
        {
            _widgets[i]->checkTouch(p);
        }
    }

  protected:
    void addWidget(Widget* w)
    {
        if (_widgetCnt < MAX_WIDGETS)
        {
            _widgets[_widgetCnt++] = w;
        }
    }

  private:
    static constexpr uint8_t MAX_WIDGETS = 10;
    Widget* _widgets[MAX_WIDGETS];
    uint8_t _widgetCnt = 0;
};

class MainMenu : public View
{
  public:
    MainMenu()
        : _mainLabel(135, 20, "CNC PCB", Colors::Blue)
        , _btnStart(100, 40, 120, 40, "START", Colors::White, Colors::Green)
        , _btnControl(100, 100, 120, 40, "Controls", Colors::White, Colors::BlueishGrey,sendGcodeX)
        , _btnSettings(100, 160, 120, 40, "SETUP", Colors::White, Colors::Blue)
    {
        addWidget(&_mainLabel);
        addWidget(&_btnStart);
        addWidget(&_btnControl);
        addWidget(&_btnSettings);
    }

    static void sendGcodeX()
    {
        if (gcodeQueue != nullptr)
        {
            const char* cmd = "G91 Z1";
            xQueueSend(gcodeQueue, cmd, 0);
        }
    }

  private:
    Label _mainLabel;
    Button _btnStart;
    Button _btnControl;
    Button _btnSettings;
};