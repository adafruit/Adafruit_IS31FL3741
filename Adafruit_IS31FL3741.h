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
    @brief Class for generic IS31FL3741 breakout.
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
  /*!
    @brief Converter for RGB888-format color to RGB565-format
    @param red 8-bit red color
    @param green 8-bit green color
    @param blue 8-bit blue color
    @returns Packed 16-bit RGB565 color
  */
  static uint16_t color565(uint8_t red, uint8_t green, uint8_t blue) {
    return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3);
  };
  /*!
    @brief Converter for RGB888-format color (packed) to RGB565-format
    @param color 24-bit value (0x00RRGGBB)
    @returns Packed 16-bit RGB565 color (0bRRRRRGGGGGGBBBBB)
  */
  static uint16_t color565(uint32_t color) {
    return ((color >> 8) & 0xF800) | ((color >> 5) & 0x07E0) |
           ((color >> 3) & 0x001F);
  };

  static uint32_t colorHSV(uint16_t hue, uint8_t sat = 255, uint8_t val = 255);

protected:
  bool selectPage(uint8_t page);

  int8_t _page = -1; ///< Cached value of the page we're currently addressing

  Adafruit_I2CDevice *_i2c_dev = NULL; ///< Pointer to I2C device
};

/**************************************************************************/
/*!
    @brief Class for Lumissil IS31FL3741 EVB.
*/
/**************************************************************************/
class Adafruit_IS31FL3741_EVB : public Adafruit_IS31FL3741 {
public:
  Adafruit_IS31FL3741_EVB(void);
  void drawPixel(int16_t x, int16_t y, uint16_t color);
};

/**************************************************************************/
/*!
    @brief Class for Lumissil IS31FL3741 QT.
*/
/**************************************************************************/
class Adafruit_IS31FL3741_QT : public Adafruit_IS31FL3741 {
public:
  Adafruit_IS31FL3741_QT(void);
  void drawPixel(int16_t x, int16_t y, uint16_t color);
};

/**************************************************************************/
/*!
    @brief Class for Lumissil IS31FL3741 Glasses (matrix portion).
*/
/**************************************************************************/
class Adafruit_IS31FL3741_GlassesMatrix : public Adafruit_GFX {
public:
  Adafruit_IS31FL3741_GlassesMatrix(Adafruit_IS31FL3741 *controller);
  void drawPixel(int16_t x, int16_t y, uint16_t color);

protected:
  Adafruit_IS31FL3741 *_is31 = NULL; ///< Pointer to core object
};

/**************************************************************************/
/*!
    @brief Class for Lumissil IS31FL3741 Glasses (ring portion).
           Not used by user code directly, the left and right classes
           below create distinct subclasses for that.
*/
/**************************************************************************/
class Adafruit_IS31FL3741_GlassesRing {
public:
  Adafruit_IS31FL3741_GlassesRing(Adafruit_IS31FL3741 *controller,
                                  bool isRight);
  void setPixelColor(int16_t n, uint32_t color);
  void fill(uint32_t color);
  /*!
    @brief    Return number of LEDs in ring (a la NeoPixel)
    @returns  int  Always 24.
  */
  uint8_t numPixels(void) const { return 24; }
  /*!
    @brief  Set brightness of LED ring. This is a mathematical brightness
            scale applied to setPixel() colors when setting ring pixels,
            distinct from any value passed to setLEDscaling() functions,
            because matrix and rings share pixels.
    @param  b  Brightness from 0 (off) to 255 (max).
  */
  void setBrightness(uint8_t b) { _brightness = b + 1; }

protected:
  Adafruit_IS31FL3741 *_is31 = NULL; ///< Pointer to core object
  uint16_t _brightness = 256;        ///< Internally 1-256 for math
  const uint16_t *ring_map;          ///< Pointer to lookup table
};

/**************************************************************************/
/*!
    @brief Class for Lumissil IS31FL3741 Glasses (left ring).
*/
/**************************************************************************/
class Adafruit_IS31FL3741_GlassesLeftRing
    : public Adafruit_IS31FL3741_GlassesRing {
public:
  Adafruit_IS31FL3741_GlassesLeftRing(Adafruit_IS31FL3741 *controller);
};

/**************************************************************************/
/*!
    @brief Class for Lumissil IS31FL3741 Glasses (right ring).
*/
/**************************************************************************/
class Adafruit_IS31FL3741_GlassesRightRing
    : public Adafruit_IS31FL3741_GlassesRing {
public:
  Adafruit_IS31FL3741_GlassesRightRing(Adafruit_IS31FL3741 *controller);
};

/**************************************************************************/
/*!
    @brief Class for Lumissil IS31FL3741 Glasses with LED data being
           buffered on the microcontroller and sent only when show() is
           called.
*/
/**************************************************************************/
class Adafruit_IS31FL3741_buffered : public Adafruit_IS31FL3741 {
public:
  Adafruit_IS31FL3741_buffered(uint8_t x = 9, uint8_t y = 13);
  ~Adafruit_IS31FL3741_buffered(void);
  bool begin(uint8_t addr = IS3741_ADDR_DEFAULT, TwoWire *theWire = &Wire);
  /*!
    @brief Don't use this drawPixel(), it's only required because
           Adafruit_IS31FL3741 is a subclass of GFX which declared a
           virtual function. Use the Adafruit_GlassesMatrix_buffered
           class for drawing.
    @param x     Ignored
    @param y     Ignored
    @param color Ignored
  */
  void drawPixel(int16_t x, int16_t y, uint16_t color) const {};
  void show(void); // DON'T const this
  /*!
    @brief    Return address of LED buffer.
    @returns  uint8_t*  Pointer to first LED position in buffer.
  */
  uint8_t *getBuffer(void) { return &ledbuf[1]; }; // See notes in show()
protected:
  uint8_t ledbuf[352]; ///< LEDs in RAM. +1 byte is intentional, see show()
};

/**************************************************************************/
/*!
    @brief Class for Lumissil IS31FL3741 Glasses (matrix portion) with LED
           data being buffered on the microcontroller and sent only when
           show() is called.
*/
/**************************************************************************/
class Adafruit_IS31FL3741_GlassesMatrix_buffered : public Adafruit_GFX {
public:
  Adafruit_IS31FL3741_GlassesMatrix_buffered(
      Adafruit_IS31FL3741_buffered *controller = NULL, bool withCanvas = false);
  void drawPixel(int16_t x, int16_t y, uint16_t color);
  void scale();
  /*!
    @brief    Get pointer to GFX canvas for smooth drawing.
    @returns  GFXcanvas16*  Pointer to GFXcanvas16 object, or NULL.
  */
  GFXcanvas16 *getCanvas(void) const { return canvas; };

protected:
  Adafruit_IS31FL3741_buffered *_is31; ///< Pointer to core object
  GFXcanvas16 *canvas = NULL;          ///< Pointer to GFX canvas
};

/**************************************************************************/
/*!
    @brief Class for Lumissil IS31FL3741 Glasses (ring portion) with LED
           data being buffered on the microcontroller and sent only when
           show() is called. Not used by user code directly, the left and
           right classes below create distinct subclasses for that.
*/
/**************************************************************************/
class Adafruit_IS31FL3741_GlassesRing_buffered {
public:
  Adafruit_IS31FL3741_GlassesRing_buffered(
      Adafruit_IS31FL3741_buffered *controller, bool isRight);
  void setPixelColor(int16_t n, uint32_t color);
  void fill(uint32_t color);
  /*!
    @brief    Return number of LEDs in ring (a la NeoPixel)
    @returns  int  Always 24.
  */
  uint8_t numPixels(void) const { return 24; }
  /*!
    @brief  Set brightness of LED ring. This is a mathematical brightness
            scale applied to setPixel() colors when setting ring pixels,
            distinct from any value passed to setLEDscaling() functions,
            because matrix and rings share pixels.
    @param  b  Brightness from 0 (off) to 255 (max).
  */
  void setBrightness(uint8_t b) { _brightness = b + 1; }

protected:
  Adafruit_IS31FL3741_buffered *_is31 = NULL; ///< Pointer to core object
  uint16_t _brightness = 256;                 ///< Internally 1-256 for math
  const uint16_t *ring_map;                   ///< Pointer to lookup table
};

/**************************************************************************/
/*!
    @brief Class for Lumissil IS31FL3741 Glasses (left ring) with LED
           data being buffered on the microcontroller and sent only when
           show() is called.
*/
/**************************************************************************/
class Adafruit_IS31FL3741_GlassesLeftRing_buffered
    : public Adafruit_IS31FL3741_GlassesRing_buffered {
public:
  Adafruit_IS31FL3741_GlassesLeftRing_buffered(
      Adafruit_IS31FL3741_buffered *controller);
};

/**************************************************************************/
/*!
    @brief Class for Lumissil IS31FL3741 Glasses (right ring) with LED
           data being buffered on the microcontroller and sent only when
           show() is called.
*/
/**************************************************************************/
class Adafruit_IS31FL3741_GlassesRightRing_buffered
    : public Adafruit_IS31FL3741_GlassesRing_buffered {
public:
  Adafruit_IS31FL3741_GlassesRightRing_buffered(
      Adafruit_IS31FL3741_buffered *controller);
};
#endif
