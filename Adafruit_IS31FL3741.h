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


/**************************************************************************/
/*!
    @brief Constructor for Lumissil IS31FL3741 Glasses
*/
/**************************************************************************/
class Adafruit_IS31FL3741_GlassesMatrix : public Adafruit_GFX {
 public:
  Adafruit_IS31FL3741_GlassesMatrix(Adafruit_IS31FL3741 *controller);
  void drawPixel(int16_t x, int16_t y, uint16_t color);

 protected:
  Adafruit_IS31FL3741 *_is31 = NULL;  

};


/**************************************************************************/
/*!
    @brief Constructor for Lumissil IS31FL3741 Glasses
*/
/**************************************************************************/
class Adafruit_IS31FL3741_GlassesRightRing {
 public:
  Adafruit_IS31FL3741_GlassesRightRing(Adafruit_IS31FL3741 *controller);
  void setPixelColor(int16_t n, uint32_t color);
  void fill(uint32_t color);
  uint8_t numPixels(void) { return 24; }
  void setBrightness(uint8_t b) { _brightness = b; }

 protected:
  Adafruit_IS31FL3741 *_is31 = NULL;
  uint8_t _brightness = 255;
  uint16_t ledmap[24][3] = {
    {287, 31, 30}, // 0
    {278, 1, 0}, // 1
    {273, 274, 275}, // 2
    {282, 283, 284}, // 3
    {270, 271, 272}, // 4
    {27, 28, 29}, // 5
    {23, 24, 25}, // 6
    {276, 277, 22}, // 7
    {20, 21, 26}, // 8
    {50, 51, 56}, // 9
    {80, 81, 86}, // 10
    {110, 111, 116}, // 11
    {140, 141, 146}, // 12
    {170, 171, 176}, // 13
    {200, 201, 206}, // 14
    {230, 231, 236}, // 15
    {260, 261, 266}, // 16
    {348, 349, 262}, // 17
    {233, 234, 235}, // 18
    {237, 238, 239}, // 19
    {339, 340, 232}, // 20
    {327, 328, 329}, // 21
    {305, 91, 90}, // 22
    {296, 61, 60}, // 23 
  };
};


/**************************************************************************/
/*!
    @brief Constructor for Lumissil IS31FL3741 Glasses
*/
/**************************************************************************/
class Adafruit_IS31FL3741_GlassesLeftRing {
 public:
  Adafruit_IS31FL3741_GlassesLeftRing(Adafruit_IS31FL3741 *controller);
  void setPixelColor(int16_t n, uint32_t color);
  void fill(uint32_t color);
  uint8_t numPixels(void) { return 24; }
  void setBrightness(uint8_t b) { _brightness = b; }

 protected:
  Adafruit_IS31FL3741 *_is31 = NULL;
  uint8_t _brightness = 255;
  uint16_t ledmap[24][3] = {
    {341, 211, 210}, // 0
    {332, 181, 180}, // 1
    {323, 151, 150}, // 2
    {127, 126, 125}, // 3
    {154, 153, 152}, // 4
    {163, 162, 161}, // 5
    {166, 165, 164}, // 6
    {244, 243, 242}, // 7
    {259, 258, 257}, // 8
    {169, 168, 167}, // 9
    {139, 138, 137}, // 10
    {109, 108, 107}, // 11
    {79, 78, 77}, // 12
    {49, 48, 47}, // 13
    {199, 198, 197}, // 14
    {229, 228, 227}, // 15
    {19, 18, 17}, // 16
    {4, 3, 2}, // 17
    {16, 15, 14}, // 18
    {13, 12, 11}, // 19
    {10, 9, 8}, // 20
    {217, 216, 215}, // 21
    {7, 6, 5}, // 22
    {350, 241, 240}, // 23
  };
};
#endif
