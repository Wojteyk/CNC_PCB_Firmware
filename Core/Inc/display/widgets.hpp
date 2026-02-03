#pragma once
#include "display/iGuiDriver.hpp"
#include <cstring>
#include "hardware/xpt2046.hpp"
#include "queue.h"

class Widget
{
  public:
    Widget(int16_t x, int16_t y, int16_t w, int16_t h)
        : _x(x)
        , _y(y)
        , _w(w)
        , _h(h)
        , _redraw(true)
    {
    }

    virtual ~Widget() = default;

    virtual void draw(IGuiDriver& driver) = 0;

    virtual void checkTouch(Point p)
    {
    }
    virtual void checkHold(Point p)
    {
    }

    virtual void release()
    {
    }

    void forceRedraw()
    {
        _redraw = true;
    }

  protected:
    int16_t _x;
    int16_t _y;
    int16_t _w;
    int16_t _h;
    bool _redraw;
};

class Label : public Widget
{
  public:
    Label(int16_t x, int16_t y, const char* text, uint16_t color)
        : Widget(x, y, 0, 0)
        , _color(color)
    {
        setText(text);
    }

    void draw(IGuiDriver& driver) override
    {
        if (!_redraw)
            return;
        driver.drawString(_x, _y, _textBuffer, _color, Colors::Background);
        _redraw = false;
    }

    void setText(const char* newText)
    {

        if (strcmp(_textBuffer, newText) != 0)
        {
            strncpy(_textBuffer, newText, sizeof(_textBuffer) - 1);
            _textBuffer[sizeof(_textBuffer) - 1] = '\0';
            _redraw = true;
        }
    }

  private:
    char _textBuffer[32];
    uint16_t _color;
};

class Button : public Widget
{
  public:
    using ClickAction = void (*)(void* ctx);

    Button(int16_t x,
           int16_t y,
           int16_t w,
           int16_t h,
           const char* text,
           uint16_t textColor,
           uint16_t color,
           ClickAction action = nullptr,
           ClickAction actionHold = nullptr,
           ClickAction actionRelease = nullptr,
           void* context = nullptr)
        : Widget(x, y, w, h)
        , _text(text)
        , _textColor(textColor)
        , _color(color)
        , _action(action)
        , _actionHold(actionHold)
        , _actionRelease(actionRelease)
        , _pressed(false)
        , _prevColor(color)
        ,_ctx(context)
    {
    }

    void draw(IGuiDriver& driver) override
    {
        if (!_redraw)
            return;

        driver.fillRect(_x, _y, _w, _h, _color);

        int16_t textX = _x + (_w - (strlen(_text) * FONT_WIDTH)) / 2;
        int16_t textY = _y + (_h - FONT_HEIGHT) / 2;

        driver.drawString(textX, textY, _text, _textColor, _color);

        _redraw = false;
        pressAnimation(_pressed);
    }

    void setParams(const char* text, uint16_t textColor, uint16_t color)
    {
        _text = text;
        _textColor = textColor;
        _color = color;
        _redraw = true;
    }

    void checkTouch(Point p) override
    {
        if (isInside(p))
        {
            _pressed = true;
            _color = Colors::White;
            _redraw = true;
            if (_action)
                _action(_ctx);
        }
    }

    void checkHold(Point p) override
    {
        if (isInside(p))
        {
            _pressed = true;
            _color = Colors::White;
            _redraw = true;
            if (_actionHold)
                _actionHold(_ctx);
        }
    }

    void release() override
    {
        if (_pressed)
        {
            _pressed = false;
            _color = _prevColor;
            _redraw = true;
            if(_actionRelease)
                _actionRelease(_ctx);
        }
    }

  private:
    void pressAnimation(bool pressed)
    {
        if (pressed)
        {
            setParams(_text, _textColor, _prevColor);
            _redraw = true;
            _pressed = false;
        }
    }

    bool isInside(Point p)
    {
        return (p.x >= _x && p.x <= (_x + _w) && p.y >= _y && p.y <= (_y + _h));
    }

    const char* _text;
    uint16_t _textColor;
    uint16_t _color;
    ClickAction _action;
    ClickAction _actionHold;
    ClickAction _actionRelease;
    bool _pressed;
    uint16_t _prevColor;
    void* _ctx;
};