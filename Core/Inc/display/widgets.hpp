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
    {
    }

    virtual ~Widget() = default;

    virtual void draw(IGuiDriver& driver) = 0;

    virtual void checkTouch(Point p)
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
        , _text(text)
        , _color(color)
        , _redraw(true)
    {
    }

    void draw(IGuiDriver& driver) override
    {
        if (!_redraw)
            return;
        driver.drawString(_x, _y, _text, _color, Colors::Background);
        _redraw = false;
    }

    void setText(const char* newText)
    {
        _text = newText;
        _redraw = true;
    }

  private:
    const char* _text;
    uint16_t _color;
    bool _redraw;
};

class Button : public Widget
{
  public:
    using ClickAction = void (*)();

    Button(int16_t x,
           int16_t y,
           int16_t w,
           int16_t h,
           const char* text,
           uint16_t textColor,
           uint16_t color,
           ClickAction action = nullptr)
        : Widget(x, y, w, h)
        , _text(text)
        , _textColor(textColor)
        , _color(color)
        , _redraw(true)
        ,_action(action)
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
        if (p.x >= _x && p.x <= (_x + _w) && p.y >= _y && p.y <= (_y + _h))
        {
            setParams(_text, Colors::Red, _color);
            if (_action) 
            {
                _action();
                _redraw = true;                
            }
        }
    }

  private:
    const char* _text;
    uint16_t _textColor;
    uint16_t _color;
    bool _redraw;
    ClickAction _action;
};