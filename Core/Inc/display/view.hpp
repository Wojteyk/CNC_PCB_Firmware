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
    static constexpr uint8_t MAX_WIDGETS = 16;
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
                      moveToControls)
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
            const char* cmd;

            cmd = "G0 X2.50 Y0.00 Z2"; xQueueSend(self->_gcodeQueue, cmd, 0); // Najazd na start (prawy brzeg)
            cmd = "G1 Z0"; xQueueSend(self->_gcodeQueue, cmd, 0);             // Opuszczenie narzędzia

            // cmd = cmd = "G1 X20 Y10"; xQueueSend(self->_gcodeQueue, cmd, 0);
            // cmd = cmd = "G1 Y0"; xQueueSend(self->_gcodeQueue, cmd, 0);
            // cmd = cmd = "G1 X40 Y10"; xQueueSend(self->_gcodeQueue, cmd, 0);
            // cmd = cmd = "G1 Y0"; xQueueSend(self->_gcodeQueue, cmd, 0);
            // cmd = cmd = "G1 X60 Y10"; xQueueSend(self->_gcodeQueue, cmd, 0);
            // cmd = cmd = "G1 Y0"; xQueueSend(self->_gcodeQueue, cmd, 0);
        // Rysowanie 24 segmentów
        cmd = "G1 X2.41 Y0.65"; xQueueSend(self->_gcodeQueue, cmd, 0);
        cmd = "G1 X2.17 Y1.25"; xQueueSend(self->_gcodeQueue, cmd, 0);
        cmd = "G1 X1.77 Y1.77"; xQueueSend(self->_gcodeQueue, cmd, 0);
        cmd = "G1 X1.25 Y2.17"; xQueueSend(self->_gcodeQueue, cmd, 0);
        cmd = "G1 X0.65 Y2.41"; xQueueSend(self->_gcodeQueue, cmd, 0);
        cmd = "G1 X0.00 Y2.50"; xQueueSend(self->_gcodeQueue, cmd, 0); // Górny punkt
        cmd = "G1 X-0.65 Y2.41"; xQueueSend(self->_gcodeQueue, cmd, 0);
        cmd = "G1 X-1.25 Y2.17"; xQueueSend(self->_gcodeQueue, cmd, 0);
        cmd = "G1 X-1.77 Y1.77"; xQueueSend(self->_gcodeQueue, cmd, 0);
        cmd = "G1 X-2.17 Y1.25"; xQueueSend(self->_gcodeQueue, cmd, 0);
        cmd = "G1 X-2.41 Y0.65"; xQueueSend(self->_gcodeQueue, cmd, 0);
        cmd = "G1 X-2.50 Y0.00"; xQueueSend(self->_gcodeQueue, cmd, 0); // Lewy punkt
        cmd = "G1 X-2.41 Y-0.65"; xQueueSend(self->_gcodeQueue, cmd, 0);
        cmd = "G1 X-2.17 Y-1.25"; xQueueSend(self->_gcodeQueue, cmd, 0);
        cmd = "G1 X-1.77 Y-1.77"; xQueueSend(self->_gcodeQueue, cmd, 0);
        cmd = "G1 X-1.25 Y-2.17"; xQueueSend(self->_gcodeQueue, cmd, 0);
        cmd = "G1 X-0.65 Y-2.41"; xQueueSend(self->_gcodeQueue, cmd, 0);
        cmd = "G1 X0.00 Y-2.50"; xQueueSend(self->_gcodeQueue, cmd, 0); // Dolny punkt
        cmd = "G1 X0.65 Y-2.41"; xQueueSend(self->_gcodeQueue, cmd, 0);
        cmd = "G1 X1.25 Y-2.17"; xQueueSend(self->_gcodeQueue, cmd, 0);
        cmd = "G1 X1.77 Y-1.77"; xQueueSend(self->_gcodeQueue, cmd, 0);
        cmd = "G1 X2.17 Y-1.25"; xQueueSend(self->_gcodeQueue, cmd, 0);
        cmd = "G1 X2.41 Y-0.65"; xQueueSend(self->_gcodeQueue, cmd, 0);
        cmd = "G1 X2.50 Y0.00"; xQueueSend(self->_gcodeQueue, cmd, 0);  // Zamknięcie okręgu

        cmd = "G0 Z2"; xQueueSend(self->_gcodeQueue, cmd, 0);
        cmd = "G0 X0 Y0"; xQueueSend(self->_gcodeQueue, cmd, 0);
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
    struct JogConfig
    {
        const char* cmdPress;
        const char* cmdHold;
        const char* cmdRelease;
        QueueHandle_t queue;
    };

  public:
    Controls(QueueHandle_t gcodeQueue)
        : _gcodeQueue(gcodeQueue)
        , _cfgXUp{nullptr, "G91 X0.5" ,"G92 X0.2" , gcodeQueue}
        , _cfgXDown{nullptr,"G91 X-0.5", "G92 X-0.2", gcodeQueue}
        , _cfgHomeX{"G10 X0",nullptr, nullptr, gcodeQueue}
        , _cfgYUp{nullptr, "G91 Y0.5", "G92 Y0.2", gcodeQueue}
        , _cfgYDown{nullptr,"G91 Y-0.5","G92 Y-0.2", gcodeQueue}
        , _cfgHomeY{"G10 Y0",nullptr, nullptr, gcodeQueue}
        , _cfgZUp{nullptr, "G91 Z0.5", "G92 Z0.2", gcodeQueue}
        , _cfgZDown{nullptr, "G91 Z-0.5", "G92 Z-0.1", gcodeQueue}
        , _cfgHomeZ{"G10 Z0", nullptr, nullptr, gcodeQueue}
        , _cfgHoming{"G28", nullptr, "G10 X0 Y0 Z0", gcodeQueue}
        , _mainLabel(135, 10, "Controls", Colors::Blue)
        , _posLabel(240, 10, "Target Pos:", Colors::White)
        , _xLabel(250, 40, "-", Colors::White)
        , _yLabel(250, 51, "-", Colors::White)
        , _zLabel(250, 62, "-", Colors::White)
        , _btnUpX(15,
                  40,
                  60,
                  50,
                  "X +",
                  Colors::White,
                  Colors::LightGrey,
                  nullptr,
                  jogHoldHandler,
                  jogReleaseHandler,
                  &_cfgXUp)
        , _btnDownX(15,
                    110,
                    60,
                    50,
                    "X -",
                    Colors::White,
                    Colors::LightGrey,
                    nullptr,
                    jogHoldHandler,
                    jogReleaseHandler,
                    &_cfgXDown)
        , _btnHomeX(20,
                    180,
                    50,
                    50,
                    "H:X",
                    Colors::White,
                    Colors::BlueishGrey,
                    jogPressHandler,
                    nullptr,
                    nullptr,
                    &_cfgHomeX)
        , _btnUpY(95,
                  40,
                  60,
                  50,
                  "Y +",
                  Colors::White,
                  Colors::LightGrey,
                  nullptr,
                  jogHoldHandler,
                  jogReleaseHandler,
                  &_cfgYUp)
        , _btnDownY(95,
                    110,
                    60,
                    50,
                    "Y -",
                    Colors::White,
                    Colors::LightGrey,
                    nullptr,
                    jogHoldHandler,
                    jogReleaseHandler,
                    &_cfgYDown)
        , _btnHomeY(100,
                    180,
                    50,
                    50,
                    "H:Y",
                    Colors::White,
                    Colors::BlueishGrey,
                    jogPressHandler,
                    nullptr,
                    nullptr,
                    &_cfgHomeY)
        , _btnUpZ(175,
                  40,
                  60,
                  50,
                  "Z +",
                  Colors::White,
                  Colors::LightGrey,
                  nullptr,
                  jogHoldHandler,
                  jogReleaseHandler,
                  &_cfgZUp)
        , _btnDownZ(175,
                    110,
                    60,
                    50,
                    "Z -",
                    Colors::White,
                    Colors::LightGrey,
                    nullptr,
                    jogHoldHandler,
                    jogReleaseHandler,
                    &_cfgZDown)
        , _btnHomeZ(180,
                    180,
                    50,
                    50,
                    "H:Z",
                    Colors::White,
                    Colors::BlueishGrey,
                    jogPressHandler,
                    nullptr,
                    nullptr,
                    &_cfgHomeZ)
        , _btnHoming(260,
                    110,
                    50,
                    50,
                    "HOME",
                    Colors::White,
                    Colors::Green,
                    jogPressHandler,
                    nullptr,
                    jogReleaseHandler,
                    &_cfgHoming)
        , _btnBack(250, 190, 60, 40, "Back", Colors::White, Colors::Orange, backToMenu)
    {
        addWidget(&_mainLabel);
        addWidget(&_posLabel);
        addWidget(&_xLabel);
        addWidget(&_yLabel);
        addWidget(&_zLabel);
        addWidget(&_btnUpX);
        addWidget(&_btnDownX);
        addWidget(&_btnHomeX);
        addWidget(&_btnUpY);
        addWidget(&_btnDownY);
        addWidget(&_btnHomeY);
        addWidget(&_btnUpZ);
        addWidget(&_btnDownZ);
        addWidget(&_btnHomeZ);
        addWidget(&_btnHoming);
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

    static void jogPressHandler(void* ctx)
    {
        auto* cfg = static_cast<JogConfig*>(ctx);
        if (cfg && cfg->cmdPress)
        {
            xQueueSend(cfg->queue, cfg->cmdPress, 0);
        }
    }

    static void jogHoldHandler(void* ctx)
    {
        auto* cfg = static_cast<JogConfig*>(ctx);
        if (cfg && cfg->cmdHold)
        {
            xQueueSend(cfg->queue, cfg->cmdHold, 0);
        }
    }

    static void jogReleaseHandler(void* ctx)
    {
        auto* cfg = static_cast<JogConfig*>(ctx);
        if (cfg && cfg->cmdRelease)
        {
            xQueueSend(cfg->queue, cfg->cmdRelease, 0);
        }
    }

    static void backToMenu(void* ctx)
    {
        GuiEvent ev = GuiEvent::ShowMain;
        xQueueSend(guiEventQueue, &ev, 0);
    }

  private:
    QueueHandle_t _gcodeQueue;

    JogConfig _cfgXUp;
    JogConfig _cfgXDown;
    JogConfig _cfgHomeX;

    JogConfig _cfgYUp;
    JogConfig _cfgYDown;
    JogConfig _cfgHomeY;

    JogConfig _cfgZUp;
    JogConfig _cfgZDown;
    JogConfig _cfgHomeZ;

    JogConfig _cfgHoming;

    Label _mainLabel;
    Label _posLabel;
    Label _xLabel;
    Label _yLabel;
    Label _zLabel;
    Button _btnUpX;
    Button _btnDownX;
    Button _btnHomeX;
    Button _btnUpY;
    Button _btnDownY;
    Button _btnHomeY;
    Button _btnUpZ;
    Button _btnDownZ;
    Button _btnHomeZ;
    Button _btnHoming;
    Button _btnBack;
};


class ErrorPage : public View
{
    public: 
    ErrorPage()
    :_mainLabel(135, 10, "Errors", Colors::Blue)
    ,_errorInfo(100,30,"Cricital Error", Colors::Warning)
    , _btnBack(250, 190, 60, 40, "Back", Colors::White, Colors::Orange, backToMenu)
    {
        addWidget(&_mainLabel);
        addWidget(&_errorInfo);
        addWidget(&_btnBack);
    }

    void setError(ErrorCode e)
    {
        const char* errorMsg = ErrorHandler::toMessage(e);
        _errorInfo.setText(errorMsg);
        forceRedraw();
    }

    static void backToMenu(void* ctx)
    {
        GuiEvent ev = GuiEvent::ShowMain;
        xQueueSend(guiEventQueue, &ev, 0);
    }
    
        
    private:
    Label _mainLabel;
    Label _errorInfo;
    Button _btnBack;

};