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

// RGB pixel color order permutations
// Offset:           R          G          B
#define IS3741_RGB ((0 << 4) | (1 << 2) | (2)) ///< Encode as R,G,B
#define IS3741_RBG ((0 << 4) | (2 << 2) | (1)) ///< Encode as R,B,G
#define IS3741_GRB ((1 << 4) | (0 << 2) | (2)) ///< Encode as G,R,B
#define IS3741_GBR ((2 << 4) | (0 << 2) | (1)) ///< Encode as G,B,R
#define IS3741_BRG ((1 << 4) | (2 << 2) | (0)) ///< Encode as B,R,G
#define IS3741_BGR ((2 << 4) | (1 << 2) | (0)) ///< Encode as B,G,R

// BASE IS31 CLASSES -------------------------------------------------------

/**************************************************************************/
/*!
    @brief  Class for Lumissil IS31FL3741 LED driver. This is the base class
            upon which the rest of this code builds. It focuses on lowest-
            level I2C operations and the chip registers, and has no concept
            of a 2D graphics coordinate system, nor of RGB colors. It is
            linear and monochromatic.
*/
/**************************************************************************/
class Adafruit_IS31FL3741 {
public:
  /*!
    @brief  Constructor for IS31FL3741 LED driver.
  */
  Adafruit_IS31FL3741() {}
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

  // Although Adafruit_IS31FL3741 itself has no concept of color, most of
  // its subclasses do. These color-related operations go here so that all
  // subclasses have access. Any 'packed' 24-bit colors received or returned
  // by these functions are always in 0xRRGGBB order; RGB reordering for
  // specific devices takes place elsewhere, in subclasses.

  /*!
    @brief    Converter for RGB888-format color (separate) to RGB565-format
    @param    red    8-bit red value.
    @param    green  8-bit green value.
    @param    blue   8-bit blue value.
    @returns  Packed 16-bit RGB565 color.
    @note     Yes, the name is unfortunate -- have lowercase color565()
              here, and uppercase ColorHSV() later. This is for
              compatibility with existing code from Adafruit_GFX and
              Adafruit_NeoPixel, which were separately developed and used
              differing cases. The idea here is to help re-use existing
              Arduino sketch code from other projects, so don't "fix" this.
  */
  static uint16_t color565(uint8_t red, uint8_t green, uint8_t blue) {
    return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3);
  }

  /*!
    @brief    Converter for RGB888-format color (packed) to RGB565-format.
    @param    color  24-bit value (0x00RRGGBB)
    @returns  Packed 16-bit RGB565 color (0bRRRRRGGGGGGBBBBB)
    @note     See notes above re: naming.
  */
  static uint16_t color565(uint32_t color) {
    return ((color >> 8) & 0xF800) | ((color >> 5) & 0x07E0) |
           ((color >> 3) & 0x001F);
  }

  static uint32_t ColorHSV(uint16_t hue, uint8_t sat = 255, uint8_t val = 255);

protected:
  bool selectPage(uint8_t page);
  bool setLEDvalue(uint8_t first_page, uint16_t lednum, uint8_t value);
  bool fillTwoPages(uint8_t first_page, uint8_t value);

  int8_t _page = -1; ///< Cached value of the page we're currently addressing
  Adafruit_I2CDevice *_i2c_dev = NULL; ///< Pointer to I2C device
};

/**************************************************************************/
/*!
    @brief  Class for a "buffered" Lumissil IS31FL3741 LED driver -- LED PWM
            state is staged in RAM (requiring 352 extra bytes vs base class)
            and sent to device only when show() is called. Otherwise
            functionally identical. LED scaling values (vs PWM) are NOT
            staged in RAM and are issued individually as normal; scaling is
            infrequently used and not worth the extra memory it would incur.
*/
/**************************************************************************/
class Adafruit_IS31FL3741_buffered : public Adafruit_IS31FL3741 {
public:
  Adafruit_IS31FL3741_buffered();
  bool begin(uint8_t addr = IS3741_ADDR_DEFAULT, TwoWire *theWire = &Wire);
  void show(void); // DON'T const this
  /*!
    @brief    Return address of LED buffer.
    @returns  uint8_t*  Pointer to first LED position in buffer.
  */
  uint8_t *getBuffer(void) { return &ledbuf[1]; } // See notes in show()
protected:
  uint8_t ledbuf[352]; ///< LEDs in RAM. +1 byte is intentional, see show()
};

// INTERMEDIARY CLASSES FOR COLORS AND GFX ---------------------------------

/**************************************************************************/
/*!
    @brief  Class for specifying RGB byte sequence order when above classes
            are used with RGB LEDs. Not used on its own...other subclasses
            of Adafruit_IS31FL3741 (direct or buffered) may reference this
            (alongside Adafruit_GFX) via multiple inheritance. It's mostly
            so the same code isn't needed repeatedly later.
*/
/**************************************************************************/
class Adafruit_IS31FL3741_ColorOrder {
public:
  /*!
    @brief  Constructor for Adafruit_IS31FL3741_ColorOrder
    @param  order  One of the IS3741_* color defined (e.g. IS3741_RGB).
  */
  Adafruit_IS31FL3741_ColorOrder(uint8_t order)
      : rOffset((order >> 4) & 3), gOffset((order >> 2) & 3),
        bOffset(order & 3) {}
  uint8_t rOffset; ///< Index of red element within RGB triplet
  uint8_t gOffset; ///< Index of green element within RGB triplet
  uint8_t bOffset; ///< Index of blue element within RGB triplet
};

/**************************************************************************/
/*!
    @brief  Class encapsulating a direct (unbuffered) IS31FL3741, ColorOrder
            and GFX all in one -- mostly so a common fill() function can be
            provided for all subclassed objects instead of repeated
            implementations. Other functions like drawPixel() are still
            unique per subclass. Not used on its own, other direct
            (unbuffered) subclasses reference this.
*/
/**************************************************************************/
class Adafruit_IS31FL3741_colorGFX : public Adafruit_IS31FL3741,
                                     public Adafruit_IS31FL3741_ColorOrder,
                                     public Adafruit_GFX {
public:
  Adafruit_IS31FL3741_colorGFX(uint8_t width, uint8_t height, uint8_t order);
  // Overload the base (monochrome) fill() with a GFX RGB565-style color.
  void fill(uint16_t color = 0);
};

/**************************************************************************/
/*!
    @brief  Class encapsulating a buffered IS31FL3741, ColorOrder and GFX
            all in one -- mostly so a common fill() function can be provided
            for all subclassed objects instead of repeated implementations.
            Other functions like drawPixel() are still unique per subclass.
            Not used on its own, other buffered subclasses reference this.
*/
/**************************************************************************/
class Adafruit_IS31FL3741_colorGFX_buffered
    : public Adafruit_IS31FL3741_buffered,
      public Adafruit_IS31FL3741_ColorOrder,
      public Adafruit_GFX {
public:
  Adafruit_IS31FL3741_colorGFX_buffered(uint8_t width, uint8_t height,
                                        uint8_t order);
  // Overload the base (monochrome) fill() with a GFX RGB565-style color.
  void fill(uint16_t color = 0);
};

/* =======================================================================
   So, IN PRINCIPLE, additional classes Adafruit_IS31FL3741_monoGFX and
   Adafruit_IS31FL3741_monoGFX_buffered could go here for hypothetical
   single-color "grayscale" matrices -- they'd be similar to the two
   colorGFX classes above, but without inheriting ColorOrder. Since no
   actual hardware along such lines currently exists, they are not
   implemented, but this is where they'd be. I've got deadlines.

   Now we'll build on the classes above to create specific LED board
   varieties. These "complete" items are then instantiated in user code.
   There are two of each -- a direct (unbuffered) and buffered version.
   It's done this way (rather than a single class with a buffer flag) to
   avoid dynamic allocation -- object & buffer just go on heap or stack
   as needed (the optional canvas in glasses is an exception).
   =======================================================================*/

/**************************************************************************/
/*!
    @brief  Class for Lumissil IS31FL3741 OEM evaluation board, direct
            (unbuffered).
*/
/**************************************************************************/
class Adafruit_IS31FL3741_EVB : public Adafruit_IS31FL3741_colorGFX {
public:
  /*!
    @brief  Constructor for Lumissil IS31FL3741 OEM evaluation board,
            13x9 pixels, direct (unbuffered).
  */
  Adafruit_IS31FL3741_EVB(uint8_t order = IS3741_BGR)
      : Adafruit_IS31FL3741_colorGFX(9, 13, order) {}
  void drawPixel(int16_t x, int16_t y, uint16_t color);
};

/**************************************************************************/
/*!
    @brief  Class for Lumissil IS31FL3741 OEM evaluation board, buffered.
*/
/**************************************************************************/
class Adafruit_IS31FL3741_EVB_buffered
    : public Adafruit_IS31FL3741_colorGFX_buffered {
public:
  /*!
    @brief  Constructor for Lumissil IS31FL3741 OEM evaluation board,
            13x9 pixels, buffered.
  */
  Adafruit_IS31FL3741_EVB_buffered(uint8_t order = IS3741_BGR)
      : Adafruit_IS31FL3741_colorGFX_buffered(9, 13, order) {}
  void drawPixel(int16_t x, int16_t y, uint16_t color);
};

/**************************************************************************/
/*!
    @brief  Class for IS31FL3741 Adafruit STEMMA QT board, direct
            (unbuffered).
*/
/**************************************************************************/
class Adafruit_IS31FL3741_QT : public Adafruit_IS31FL3741_colorGFX {
public:
  /*!
    @brief  Constructor for STEMMA QT version (13 x 9 LEDs), direct
            (unbuffered).
  */
  Adafruit_IS31FL3741_QT(uint8_t order = IS3741_BGR)
      : Adafruit_IS31FL3741_colorGFX(13, 9, order) {}
  void drawPixel(int16_t x, int16_t y, uint16_t color);
};

/**************************************************************************/
/*!
    @brief  Class for IS31FL3741 Adafruit STEMMA QT board, buffered.
*/
/**************************************************************************/
class Adafruit_IS31FL3741_QT_buffered
    : public Adafruit_IS31FL3741_colorGFX_buffered {
public:
  /*!
    @brief  Constructor for STEMMA QT version (13 x 9 LEDs), buffered.
  */
  Adafruit_IS31FL3741_QT_buffered(uint8_t order = IS3741_BGR)
      : Adafruit_IS31FL3741_colorGFX_buffered(13, 9, order) {}
  void drawPixel(int16_t x, int16_t y, uint16_t color);
};

/* =======================================================================
   This is the newer and simpler way (to the user) of using Adafruit
   EyeLights LED glasses. Declaring an EyeLights object (direct or
   buffered) gets you the matrix and rings automatically; no need to
   instantiate as separate objects, nor does one need to explicitly
   declare a base Adafruit_IS31FL3741 object and pass it in. Internally
   the code has a few more layers but the user doesn't need to see that.
   =======================================================================*/

/**************************************************************************/
/*!
    @brief  Base class for EyeLights LED ring. Holds a few items that are
            common to direct or buffered instances, left or right ring.
*/
/**************************************************************************/
class Adafruit_EyeLights_Ring_Base {
public:
  Adafruit_EyeLights_Ring_Base(void *parent, bool isRight);
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
  uint16_t _brightness = 256; ///< Internally 1-256 for math
  void *parent;               ///< Pointer back to EyeLights object
  const uint16_t *ring_map;   ///< Pointer to LED index lookup table
};

/**************************************************************************/
/*!
    @brief  Class for direct (unbuffered) EyeLights LED ring, left or right.
*/
/**************************************************************************/
class Adafruit_EyeLights_Ring : public Adafruit_EyeLights_Ring_Base {
public:
  Adafruit_EyeLights_Ring(void *parent, bool isRight)
      : Adafruit_EyeLights_Ring_Base(parent, isRight) {}
  void setPixelColor(int16_t n, uint32_t color);
  void fill(uint32_t color);
};

/**************************************************************************/
/*!
    @brief  Class for buffered EyeLights LED ring, left or right.
*/
/**************************************************************************/
class Adafruit_EyeLights_Ring_buffered : public Adafruit_EyeLights_Ring_Base {
public:
  Adafruit_EyeLights_Ring_buffered(void *parent, bool isRight)
      : Adafruit_EyeLights_Ring_Base(parent, isRight) {}
  void setPixelColor(int16_t n, uint32_t color);
  void fill(uint32_t color);
};

/**************************************************************************/
/*!
    @brief  Base class for EyeLights LED glasses. Holds a few items that
            are common to direct or buffered instances.
*/
/**************************************************************************/
class Adafruit_EyeLights_Base {
public:
  Adafruit_EyeLights_Base(bool withCanvas) {
    if (withCanvas)
      canvas = new GFXcanvas16(18 * 3, 5 * 3);
  }
  ~Adafruit_EyeLights_Base() { delete canvas; }
  /*!
    @brief    Get pointer to GFX canvas for smooth drawing.
    @returns  GFXcanvas16*  Pointer to GFXcanvas16 object, or NULL.
  */
  GFXcanvas16 *getCanvas(void) const { return canvas; }

protected:
  GFXcanvas16 *canvas = NULL; ///< Pointer to GFX canvas
};

/**************************************************************************/
/*!
    @brief  Class for Adafruit EyeLights, direct (unbuffered).
*/
/**************************************************************************/
class Adafruit_EyeLights : public Adafruit_EyeLights_Base,
                           public Adafruit_IS31FL3741_colorGFX {
public:
  Adafruit_EyeLights(bool withCanvas = false, uint8_t order = IS3741_BGR)
      : Adafruit_EyeLights_Base(withCanvas),
        Adafruit_IS31FL3741_colorGFX(18, 5, order), left_ring(this, false),
        right_ring(this, true) {}
  void drawPixel(int16_t x, int16_t y, uint16_t color);
  void scale();
  Adafruit_EyeLights_Ring left_ring;
  Adafruit_EyeLights_Ring right_ring;
};

/**************************************************************************/
/*!
    @brief  Class for Adafruit EyeLights, buffered.
*/
/**************************************************************************/
class Adafruit_EyeLights_buffered
    : public Adafruit_EyeLights_Base,
      public Adafruit_IS31FL3741_colorGFX_buffered {
public:
  Adafruit_EyeLights_buffered(bool withCanvas = false,
                              uint8_t order = IS3741_BGR)
      : Adafruit_EyeLights_Base(withCanvas),
        Adafruit_IS31FL3741_colorGFX_buffered(18, 5, order),
        left_ring(this, false), right_ring(this, true) {}
  void drawPixel(int16_t x, int16_t y, uint16_t color);
  void scale();
  Adafruit_EyeLights_Ring_buffered left_ring;
  Adafruit_EyeLights_Ring_buffered right_ring;
};

/* =======================================================================
   This is the older (likely deprecated) way of using Adafruit EyeLights.
   It requires a few extra steps of the user for object declarations, and
   doesn't handle different RGB color orders.
   =======================================================================*/

/**************************************************************************/
/*!
    @brief  Class for Adafruit LED Glasses (matrix portion).
    @note   This class is deprecated but provided for compatibility.
            New code should use the Adafruit_EyeLights classes.
*/
/**************************************************************************/
class Adafruit_IS31FL3741_GlassesMatrix : public Adafruit_GFX {
public:
  /*!
    @brief  Constructor for LED glasses (matrix portion, 18x5 LEDs)
    @param  controller  Pointer to core object (underlying hardware).
  */
  Adafruit_IS31FL3741_GlassesMatrix(Adafruit_IS31FL3741 *controller)
      : Adafruit_GFX(18, 5), _is31(controller) {}
  void drawPixel(int16_t x, int16_t y, uint16_t color);

protected:
  Adafruit_IS31FL3741 *_is31 = NULL; ///< Pointer to core object
};

/**************************************************************************/
/*!
    @brief  Class for Adafruit LED Glasses (ring portion). Not used by user
            code directly, the left and right classes below create distinct
            subclasses for that.
    @note   This class is deprecated but provided for compatibility.
            New code should use the Adafruit_EyeLights classes.
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
    @brief  Class for Adafruit LED Glasses (left ring).
    @note   This class is deprecated but provided for compatibility.
            New code should use the Adafruit_EyeLights classes.
*/
/**************************************************************************/
class Adafruit_IS31FL3741_GlassesLeftRing
    : public Adafruit_IS31FL3741_GlassesRing {
public:
  /*!
    @brief  Constructor for glasses left LED ring.
    @param  controller  Pointer to Adafruit_IS31FL3741 object.
  */
  Adafruit_IS31FL3741_GlassesLeftRing(Adafruit_IS31FL3741 *controller)
      : Adafruit_IS31FL3741_GlassesRing(controller, false) {}
};

/**************************************************************************/
/*!
    @brief  Class for Adafruit LED Glasses (right ring).
    @note   This class is deprecated but provided for compatibility.
            New code should use the Adafruit_EyeLights classes.
*/
/**************************************************************************/
class Adafruit_IS31FL3741_GlassesRightRing
    : public Adafruit_IS31FL3741_GlassesRing {
public:
  /*!
    @brief  Constructor for glasses right LED ring.
    @param  controller  Pointer to Adafruit_IS31FL3741 object.
  */
  Adafruit_IS31FL3741_GlassesRightRing(Adafruit_IS31FL3741 *controller)
      : Adafruit_IS31FL3741_GlassesRing(controller, true) {}
};

/**************************************************************************/
/*!
    @brief  Class for Adafruit LED Glasses (matrix portion) with LED data
            being buffered on the microcontroller and sent only when show()
            is called.
    @note   This class is deprecated but provided for compatibility.
            New code should use the Adafruit_EyeLights classes.
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
  GFXcanvas16 *getCanvas(void) const { return canvas; }

protected:
  Adafruit_IS31FL3741_buffered *_is31; ///< Pointer to core object
  GFXcanvas16 *canvas = NULL;          ///< Pointer to GFX canvas
};

/**************************************************************************/
/*!
    @brief  Class for Adafruit LED Glasses (ring portion) with LED data
            being buffered on the microcontroller and sent only when show()
            is called. Not used by user code directly, the left and right
            classes below create distinct subclasses for that.
    @note   This class is deprecated but provided for compatibility.
            New code should use the Adafruit_EyeLights classes.
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
    @brief  Class for Lumissil IS31FL3741 Glasses (left ring) with LED
            data being buffered on the microcontroller and sent only when
            show() is called.
    @note   This class is deprecated but provided for compatibility.
            New code should use the Adafruit_EyeLights classes.
*/
/**************************************************************************/
class Adafruit_IS31FL3741_GlassesLeftRing_buffered
    : public Adafruit_IS31FL3741_GlassesRing_buffered {
public:
  /*!
    @brief  Constructor for buffered glasses left LED ring.
    @param  controller  Pointer to Adafruit_IS31FL3741_buffered object.
  */
  Adafruit_IS31FL3741_GlassesLeftRing_buffered(
      Adafruit_IS31FL3741_buffered *controller)
      : Adafruit_IS31FL3741_GlassesRing_buffered(controller, false) {}
};

/**************************************************************************/
/*!
    @brief  Class for Adafruit LED Glasses (right ring) with LED data being
            buffered on the microcontroller and sent only when show() is
            called.
    @note   This class is deprecated but provided for compatibility.
            New code should use the Adafruit_EyeLights classes.
*/
/**************************************************************************/
class Adafruit_IS31FL3741_GlassesRightRing_buffered
    : public Adafruit_IS31FL3741_GlassesRing_buffered {
public:
  /*!
    @brief  Constructor for buffered glasses right LED ring.
    @param  controller  Pointer to Adafruit_IS31FL3741_buffered object.
  */
  Adafruit_IS31FL3741_GlassesRightRing_buffered(
      Adafruit_IS31FL3741_buffered *controller)
      : Adafruit_IS31FL3741_GlassesRing_buffered(controller, true) {}
};

#endif // _ADAFRUIT_IS31FL3741_H_
