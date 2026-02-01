#include "hardware/xpt2046.hpp"


XPT2046::XPT2046(SPI_HandleTypeDef* hspi,
                 GPIO_TypeDef* csPort,
                 uint16_t csPin,
                 GPIO_TypeDef* irqPort,
                 uint16_t irqPin)
    : _hspi(hspi)
    , _csPort(csPort)
    , _csPin(csPin)
    , _irqPort(irqPort)
    , _irqPin(irqPin)
{
}

void XPT2046::init() {
        HAL_GPIO_WritePin(_csPort, _csPin, GPIO_PIN_SET); // CS nieaktywny
    }

uint16_t XPT2046::getRaw(uint8_t cmd)
{
    uint8_t txBuff[3] = {cmd, 0x00, 0x00};
    uint8_t rxBuff[3] = {0,0,0};

    HAL_GPIO_WritePin(_csPort, _csPin, GPIO_PIN_RESET);

    HAL_SPI_TransmitReceive(_hspi, txBuff, rxBuff, 3, 10);

    HAL_GPIO_WritePin(_csPort, _csPin, GPIO_PIN_SET);

    return (((rxBuff[1] << 8) | rxBuff[2]) >> 3);
}

bool XPT2046::isPressed() {
        return (HAL_GPIO_ReadPin(_irqPort, _irqPin) == GPIO_PIN_RESET);
    }

long XPT2046::map(long x, long in_min, long in_max, long out_min, long out_max) {
        return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    }

Point XPT2046::getPoint()
{
    if(!isPressed()) return {-1,-1};
    uint16_t rawX = getRaw(GET_X_CMD);
    uint16_t rawY = getRaw(GET_Y_CMD);
    
    if (rawX < 50 || rawX > 4000 || rawY < 50 || rawY > 4000) return {-1, -1}; //if some noise

    int16_t x ,y;

    x = map(rawY, RAW_Y_MIN, RAW_Y_MAX, 0, SCREEN_W);
    y = map(rawX, RAW_X_MIN, RAW_X_MAX, 0, SCREEN_H);

    if (x < 0) x = 0;
    if (x >= SCREEN_W) x = SCREEN_W - 1;
    if (y < 0) y = 0;
    if (y >= SCREEN_H) y = SCREEN_H - 1;
    
    return {x,y};
}