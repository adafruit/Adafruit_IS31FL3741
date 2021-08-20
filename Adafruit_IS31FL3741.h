#ifndef _ADAFRUIT_IS31FL3741_H_
#define _ADAFRUIT_IS31FL3741_H_

#include <Adafruit_BusIO_Register.h>
#include <Adafruit_GFX.h>
#include <Adafruit_I2CDevice.h>
#include <Arduino.h>

#define IS3741_ADDR_DEFAULT 0x30

#define IS3741_COMMANDREGISTER 0xFD
#define IS3741_COMMANDREGISTERLOCK 0xFE
#define IS3741_INTMASKREGISTER 0xF0
#define IS3741_INTSTATUSREGISTER 0xF1
#define IS3741_IDREGISTER 0xFC

#define IS3741_FUNCREG_CONFIG 0x00
#define IS3741_FUNCREG_GCURRENT 0x01
#define IS3741_FUNCREG_RESET 0x3F

/**************************************************************************/
/*!
    @brief Constructor for generic IS31FL3741 breakout version
*/
/**************************************************************************/
class Adafruit_IS31FL3741 : public Adafruit_GFX {
public:
  Adafruit_IS31FL3741(uint8_t x = 9, uint8_t y = 13);
  bool begin(uint8_t addr = IS3741_ADDR_DEFAULT, TwoWire *theWire = &Wire);
  bool reset(void);
  bool enable(bool en);

  bool unlock(void);

  bool setGlobalCurrent(uint8_t current);
  uint8_t getGlobalCurrent(void);

  bool setLEDscaling(uint16_t lednum, uint8_t scale);
  bool setLEDscaling(uint8_t scale);

  bool setLEDPWM(uint16_t lednum, uint8_t pwm);
  bool fill(uint8_t fillpwm = 0);

  void drawPixel(int16_t x, int16_t y, uint16_t color);
  static uint16_t color565(uint8_t red, uint8_t green, uint8_t blue);

protected:
  bool selectPage(uint8_t page);

  uint8_t _page = -1; ///< Cached value of the page we're currently addressing

private:
  Adafruit_I2CDevice *_i2c_dev = NULL;
};

/**************************************************************************/
/*!
    @brief Constructor for Lumissil IS31FL3741 EVB
*/
/**************************************************************************/
class Adafruit_IS31FL3741_EVB : public Adafruit_IS31FL3741 {
public:
  Adafruit_IS31FL3741_EVB(void);
  void drawPixel(int16_t x, int16_t y, uint16_t color);
};

/**************************************************************************/
/*!
    @brief Constructor for Lumissil IS31FL3741 QT
*/
/**************************************************************************/
class Adafruit_IS31FL3741_QT : public Adafruit_IS31FL3741 {
public:
  Adafruit_IS31FL3741_QT(void);
  void drawPixel(int16_t x, int16_t y, uint16_t color);
};

#endif
