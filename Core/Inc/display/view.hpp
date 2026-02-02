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

    void handleHold(Point p)
    {
        for (uint8_t i = 0; i < _widgetCnt; i++)
        {
            _widgets[i]->checkHold(p);
        }
    }

    void handleRelease()
    {
        for (uint8_t i = 0; i < _widgetCnt; i++)
        {
            _widgets[i]->release();
        }
    }

    void forceRedraw()
    {
        for (uint8_t i = 0; i < _widgetCnt; i++)
        {
            _widgets[i]->forceRedraw();
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
        , _btnStart(100, 40, 120, 40, "START", Colors::White, Colors::Green, sendStartGcode)
        , _btnControl(100,
                      100,
                      120,
                      40,
                      "Controls",
                      Colors::White,
                      Colors::BlueishGrey,
                      moveToControls)
        , _btnSettings(100, 160, 120, 40, "SETUP", Colors::White, Colors::Blue)
    {
        addWidget(&_mainLabel);
        addWidget(&_btnStart);
        addWidget(&_btnControl);
        addWidget(&_btnSettings);
    }

    static void moveToControls()
    {
        GuiEvent ev = GuiEvent::ShowControls;
        xQueueSend(guiEventQueue, &ev, 0);
    }

    static void sendStartGcode()
    {
        if (gcodeQueue != nullptr)
        {
            const char* cmd = "G1 Z1";
            xQueueSend(gcodeQueue, cmd, 0);
        }
    }

  private:
    Label _mainLabel;
    Button _btnStart;
    Button _btnControl;
    Button _btnSettings;
};

class Controls : public View
{
  public:
    Controls()
        : _mainLabel(135, 20, "Controls", Colors::Blue)
        , _btnUpZ(200,
                  40,
                  60,
                  40,
                  "Z +",
                  Colors::White,
                  Colors::LightGrey,
                  sendGcodeZUp,
                  keepGcodeZUp)
        , _btnDownZ(200,
                    100,
                    60,
                    40,
                    "Z -",
                    Colors::White,
                    Colors::LightGrey,
                    sendGcodeZDown,
                    keepGcodeZDown)
        , _btnBack(260, 180, 60, 40, "Back", Colors::White, Colors::Orange, backToMenu)
    {
        addWidget(&_mainLabel);
        addWidget(&_btnUpZ);
        addWidget(&_btnDownZ);
        addWidget(&_btnBack);
    }

    static void sendGcodeZUp()
    {
        if (gcodeQueue != nullptr)
        {
            const char* cmd = "G91 Z0.2";
            xQueueSend(gcodeQueue, cmd, 0);
        }
    }

    static void sendGcodeZDown()
    {
        if (gcodeQueue != nullptr)
        {
            const char* cmd = "G91 Z-0.2";
            xQueueSend(gcodeQueue, cmd, 0);
        }
    }

    static void keepGcodeZDown()
    {
        const char* cmd = "G91 Z-0.1";
        xQueueSend(gcodeQueue, cmd, 0);
    }

    static void keepGcodeZUp()
    {
        const char* cmd = "G91 Z0.1";
        xQueueSend(gcodeQueue, cmd, 0);
    }

    static void backToMenu()
    {
        GuiEvent ev = GuiEvent::ShowMain;
        xQueueSend(guiEventQueue, &ev, 0);
    }

  private:
    Label _mainLabel;
    Button _btnUpZ;
    Button _btnDownZ;
    Button _btnBack;
};