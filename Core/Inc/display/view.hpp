#pragma once
#include "display/iGuiDriver.hpp"
#include "display/widgets.hpp"
#include "FreeRTOS.h"
#include <cstring>
#include <cstdio>
#include "queue.h"
#include "common/motionTypes.hpp"

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
    MainMenu(QueueHandle_t gcodeQueue)
        : _gcodeQueue(gcodeQueue)
        , _mainLabel(135, 20, "CNC PCB", Colors::Blue)
        , _btnStart(100,
                    40,
                    120,
                    40,
                    "START",
                    Colors::White,
                    Colors::Green,
                    sendStartGcode,
                    nullptr,
                    nullptr,
                    this)
        , _btnControl(100,
                      100,
                      120,
                      40,
                      "Controls",
                      Colors::White,
                      Colors::BlueishGrey,
                      moveToControls )
        , _btnSettings(100, 160, 120, 40, "SETUP", Colors::White, Colors::Blue)
    {
        addWidget(&_mainLabel);
        addWidget(&_btnStart);
        addWidget(&_btnControl);
        addWidget(&_btnSettings);
    }

    static void moveToControls(void* ctx)
    {
        GuiEvent ev = GuiEvent::ShowControls;
        xQueueSend(guiEventQueue, &ev, 0);
    }

    static void sendStartGcode(void* ctx)
    {
        auto* self = static_cast<MainMenu*>(ctx);
        if (self->_gcodeQueue != nullptr)
        {
            const char* cmd = "G1 Z1";
            xQueueSend(self->_gcodeQueue, cmd, 0);
        }
    }

  private:
    QueueHandle_t _gcodeQueue;

    Label _mainLabel;
    Button _btnStart;
    Button _btnControl;
    Button _btnSettings;
};

class Controls : public View
{
  public:
    Controls(QueueHandle_t gcodeQueue)
        : _gcodeQueue(gcodeQueue)
        , _mainLabel(135, 20, "Controls", Colors::Blue)
        , _posLabel(10, 20, "Target Pos:", Colors::White)
        , _xLabel(20, 35, "-", Colors::BlueishGrey)
        , _yLabel(20, 45, "-", Colors::BlueishGrey)
        , _zLabel(20, 55, "-", Colors::BlueishGrey)
        , _btnUpZ(200,
                  40,
                  60,
                  50,
                  "Z +",
                  Colors::White,
                  Colors::LightGrey,
                  sendGcodeZUp,
                  keepGcodeZUp,
                  nullptr,
                  this)
        , _btnDownZ(200,
                    110,
                    60,
                    50,
                    "Z -",
                    Colors::White,
                    Colors::LightGrey,
                    sendGcodeZDown,
                    keepGcodeZDown,
                    nullptr,
                    this)
        , _btnBack(250, 180, 60, 40, "Back", Colors::White, Colors::Orange, backToMenu)
    {
        addWidget(&_mainLabel);
        addWidget(&_posLabel);
        addWidget(&_xLabel);
        addWidget(&_yLabel);
        addWidget(&_zLabel);
        addWidget(&_btnUpZ);
        addWidget(&_btnDownZ);
        addWidget(&_btnBack);
    }

    void updateCurrentPos(const MachineState& state)
    {
        char buf[16];
        snprintf(buf, sizeof(buf), "X: %.2f", state.currentX);
        _xLabel.setText(buf);

        snprintf(buf, sizeof(buf), "Y: %.2f", state.currentY);
        _yLabel.setText(buf);

        snprintf(buf, sizeof(buf), "Z: %.2f", state.currentZ);
        _zLabel.setText(buf);
    }

    static void sendGcodeZUp(void* ctx)
    {
        auto* self = static_cast<Controls*>(ctx);
        if (self->_gcodeQueue != nullptr)
        {
            const char* cmd = "G91 Z0.2";
            xQueueSend(self->_gcodeQueue, cmd, 0);
        }
    }

    static void sendGcodeZDown(void* ctx)
    {
        auto* self = static_cast<Controls*>(ctx);
        if (self->_gcodeQueue != nullptr)
        {
            const char* cmd = "G91 Z-0.2";
            xQueueSend(self->_gcodeQueue, cmd, 0);
        }
    }

    static void keepGcodeZDown(void* ctx)
    {
        auto* self = static_cast<Controls*>(ctx);
        const char* cmd = "G91 Z-1";
        xQueueSend(self->_gcodeQueue, cmd, 0);
    }

    static void keepGcodeZUp(void* ctx)
    {
        auto* self = static_cast<Controls*>(ctx);
        const char* cmd = "G91 Z1";
        xQueueSend(self->_gcodeQueue, cmd, 0);
    }

    static void backToMenu(void* ctx)
    {
        GuiEvent ev = GuiEvent::ShowMain;
        xQueueSend(guiEventQueue, &ev, 0);
    }

  private:
    QueueHandle_t _gcodeQueue;

    Label _mainLabel;
    Label _posLabel;
    Label _xLabel;
    Label _yLabel;
    Label _zLabel;
    Button _btnUpZ;
    Button _btnDownZ;
    Button _btnBack;
};