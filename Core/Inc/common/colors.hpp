#pragma once
#include <stdint.h>

namespace Colors {

    static constexpr uint16_t Black       = 0x0000;
    static constexpr uint16_t White       = 0xFFFF;
    static constexpr uint16_t Red         = 0xF800;
    static constexpr uint16_t Green       = 0x07E0;
    static constexpr uint16_t Blue        = 0x001F;
    static constexpr uint16_t Cyan        = 0x07FF;
    static constexpr uint16_t Magenta     = 0xF81F;
    static constexpr uint16_t Yellow      = 0xFFE0;
    static constexpr uint16_t Orange      = 0xFD20;
    
    static constexpr uint16_t LightGrey   = 0xC618; 
    static constexpr uint16_t DarkGrey    = 0x7BEF; 
    static constexpr uint16_t BlueishGrey = 0x4A69; 
    
    static constexpr uint16_t Background  = Black;
    static constexpr uint16_t Text        = White;
    static constexpr uint16_t Error       = Red;       
    static constexpr uint16_t Success     = Green;    
    static constexpr uint16_t Warning     = Orange;    
    static constexpr uint16_t Idle        = BlueishGrey; 

    constexpr uint16_t make(uint8_t r, uint8_t g, uint8_t b) {
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }
}