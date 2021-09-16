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

  int8_t _page = -1; ///< Cached value of the page we're currently addressing

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

class Adafruit_IS31FL3741_GlassesRing {
 public:
  Adafruit_IS31FL3741_GlassesRing(Adafruit_IS31FL3741 *controller, bool isRight);
  void setPixelColor(int16_t n, uint32_t color);
  void fill(uint32_t color);
  uint8_t numPixels(void) { return 24; }
  void setBrightness(uint8_t b) { _brightness = b + 1; }

 protected:
  Adafruit_IS31FL3741 *_is31 = NULL;
  uint16_t _brightness = 256; // Internally is 1-256 for math
  const uint16_t *ring_map;
};


/**************************************************************************/
/*!
    @brief Constructor for Lumissil IS31FL3741 Glasses
*/
/**************************************************************************/
class Adafruit_IS31FL3741_GlassesRightRing : public Adafruit_IS31FL3741_GlassesRing {
 public:
  Adafruit_IS31FL3741_GlassesRightRing(Adafruit_IS31FL3741 *controller);
};


/**************************************************************************/
/*!
    @brief Constructor for Lumissil IS31FL3741 Glasses
*/
/**************************************************************************/
class Adafruit_IS31FL3741_GlassesLeftRing : public Adafruit_IS31FL3741_GlassesRing {
 public:
  Adafruit_IS31FL3741_GlassesLeftRing(Adafruit_IS31FL3741 *controller);
};


class Adafruit_IS31FL3741_buffered : public Adafruit_IS31FL3741 {
public:
  Adafruit_IS31FL3741_buffered(uint8_t x = 9, uint8_t y = 13);
  ~Adafruit_IS31FL3741_buffered(void);
  bool begin(uint8_t addr = IS3741_ADDR_DEFAULT, TwoWire *theWire = &Wire);

  void drawPixel(int16_t x, int16_t y, uint16_t color);
  void show(void);

  uint8_t ledbuf[352]; // Intentionally 1 extra byte
};

class Adafruit_IS31FL3741_buffered_GlassesRing {
 public:
  Adafruit_IS31FL3741_buffered_GlassesRing(Adafruit_IS31FL3741_buffered *controller, bool isRight);
  void setPixelColor(int16_t n, uint32_t color);
  void fill(uint32_t color);
  uint8_t numPixels(void) { return 24; }
  void setBrightness(uint8_t b) { _brightness = b + 1; }

 protected:
  Adafruit_IS31FL3741_buffered *_is31 = NULL;
  uint16_t _brightness = 256; // Internally is 1-256 for math
  const uint16_t *ring_map;
};

class Adafruit_IS31FL3741_buffered_GlassesLeftRing : public Adafruit_IS31FL3741_buffered_GlassesRing {
public:
  Adafruit_IS31FL3741_buffered_GlassesLeftRing(Adafruit_IS31FL3741_buffered *controller);
};

class Adafruit_IS31FL3741_buffered_GlassesRightRing : public Adafruit_IS31FL3741_buffered_GlassesRing {
public:
  Adafruit_IS31FL3741_buffered_GlassesRightRing(Adafruit_IS31FL3741_buffered *controller);
};

class Adafruit_IS31FL3741_buffered_GlassesMatrix : public Adafruit_GFX {
 public:
  Adafruit_IS31FL3741_buffered_GlassesMatrix(Adafruit_IS31FL3741_buffered *controller = NULL);
  void drawPixel(int16_t x, int16_t y, uint16_t color);

protected:
  Adafruit_IS31FL3741_buffered *_is31;
};

class Adafruit_IS31FL3741_buffered_GlassesMatrix_smooth : public Adafruit_IS31FL3741_buffered_GlassesMatrix {
 public:
  Adafruit_IS31FL3741_buffered_GlassesMatrix_smooth(Adafruit_IS31FL3741_buffered *controller = NULL);
  void drawPixel(int16_t x, int16_t y, uint16_t color);
  void scale();
  GFXcanvas16 *getCanvas(void) const { return canvas; };
 protected:
  GFXcanvas16 *canvas;
};

#endif
