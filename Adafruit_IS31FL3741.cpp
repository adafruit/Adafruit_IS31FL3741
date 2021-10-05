#include <Adafruit_IS31FL3741.h>

// drawPixel() and setPixelColor() for various classes are all doing
// certain things similarly, but not exactly 100%. These #defines encompass
// the similar parts, done this way (rather than functions) to avoid call
// overhead in lowest-level functions. Don't fret about this being "too
// big," as most projects will likely only invoke a single class, not like
// every variant gets instantiated.

#ifndef _swap_int16_t
#define _swap_int16_t(a, b)                                                    \
  {                                                                            \
    int16_t t = a;                                                             \
    a = b;                                                                     \
    b = t;                                                                     \
  }
#endif

// This does GFX-style "soft rotation" in each drawPixel():
#define _IS31_ROTATE_(_X_, _Y_)                                                \
  switch (getRotation()) {                                                     \
  case 1:                                                                      \
    _swap_int16_t(_X_, _Y_);                                                   \
    _X_ = WIDTH - 1 - _X_;                                                     \
    break;                                                                     \
  case 2:                                                                      \
    _X_ = WIDTH - 1 - _X_;                                                     \
    _Y_ = HEIGHT - 1 - _Y_;                                                    \
    break;                                                                     \
  case 3:                                                                      \
    _swap_int16_t(_X_, _Y_);                                                   \
    _Y_ = HEIGHT - 1 - _Y_;                                                    \
    break;                                                                     \
  }

// This expands a GFX-style RGB565 color to RGB888 in 3 uint8_t variables,
// declared here, i.e. at the corresponding position in the drawPixel() func.
#define _IS31_EXPAND_(_COLOR_, _R_, _G_, _B_)                                  \
  uint8_t _R_ = ((_COLOR_ >> 8) & 0xF8) | (_COLOR_ >> 13);                     \
  uint8_t _G_ = ((_COLOR_ >> 3) & 0xFC) | ((_COLOR_ >> 9) & 0x03);             \
  uint8_t _B_ = ((_COLOR_ << 3) & 0xF8) | ((_COLOR_ >> 2) & 0x07);

// This scales a packed 24-bit RGB value by a brightness level (0-255) and
// places in 3 uint8_t variables declared here, i.e. at the corresponding
// position in the setPixelColor() function. It's used for NeoPixel-like
// behavior with the LED rings.
#define _IS31_SCALE_RGB_(_COLOR_, _R_, _G_, _B_, _BRIGHTNESS_)                 \
  uint8_t _R_ = (((uint16_t)((_COLOR_ >> 16) & 0xFF)) * _BRIGHTNESS_) >> 8;    \
  uint8_t _G_ = (((uint16_t)((_COLOR_ >> 8) & 0xFF)) * _BRIGHTNESS_) >> 8;     \
  uint8_t _B_ = (((uint16_t)(_COLOR_ & 0xFF)) * _BRIGHTNESS_) >> 8;

// This scales unpacked separate 8-bit RGB components by a brightness level
// (0-255) and places in the same 3 uint8_t variables passed.
#define _IS31_SCALE_RGB_SEPARATE_(_R_, _G_, _B_, _BRIGHTNESS_)                 \
  _R_ = ((uint16_t)_R_ * _BRIGHTNESS_) >> 8;                                   \
  _G_ = ((uint16_t)_G_ * _BRIGHTNESS_) >> 8;                                   \
  _B_ = ((uint16_t)_B_ * _BRIGHTNESS_) >> 8;

// IS31FL3741 (DIRECT, UNBUFFERED) -----------------------------------------
// Most of these functions are also used by the IS31 buffered subclass,
// only a few are overloaded. Those appear later.

/**************************************************************************/
/*!
    @brief    Initialize I2C and IS31FL3741 hardware.
    @param    addr     I2C address where we expect to find the chip.
    @param    theWire  Pointer to TwoWire I2C bus to use, defaults to &Wire.
    @returns  true on success, false if chip isn't found.
*/
/**************************************************************************/
bool Adafruit_IS31FL3741::begin(uint8_t addr, TwoWire *theWire) {
  delete _i2c_dev;
  _i2c_dev = new Adafruit_I2CDevice(addr, theWire);

  if (_i2c_dev->begin()) {
    // User code can set this faster if it wants, this is simply
    // the max ordained I2C speed on AVR.
    _i2c_dev->setSpeed(400000);

    Adafruit_BusIO_Register id_reg =
        Adafruit_BusIO_Register(_i2c_dev, IS3741_IDREGISTER);
    if ((id_reg.read() == (addr * 2)) && reset()) {
      return true; // Success!
    }
  }

  return false; // Sad
}

/**************************************************************************/
/*!
    @brief    Perform software reset, update all registers to POR values.
    @returns  true if I2C command acknowledged, false on error.
*/
/**************************************************************************/
bool Adafruit_IS31FL3741::reset(void) {
  selectPage(4);
  Adafruit_BusIO_Register reset_reg =
      Adafruit_BusIO_Register(_i2c_dev, IS3741_FUNCREG_RESET);
  return reset_reg.write(0xAE);
}

/**************************************************************************/
/*!
    @brief    Enable/disable output via the shutdown register bit.
    @param    en  true to enable, false to disable.
    @returns  true if I2C command acknowledged, false on error.
*/
/**************************************************************************/
bool Adafruit_IS31FL3741::enable(bool en) {
  selectPage(4);
  Adafruit_BusIO_Register config_reg =
      Adafruit_BusIO_Register(_i2c_dev, IS3741_FUNCREG_CONFIG);
  Adafruit_BusIO_RegisterBits shutdown_bit =
      Adafruit_BusIO_RegisterBits(&config_reg, 1, 0);
  return shutdown_bit.write(en);
}

/**************************************************************************/
/*!
    @brief    Set global current-mirror from 0 (off) to 255 (brightest).
    @param    current  8-bit level, 0 to 255.
    @returns  true if I2C command acknowledged, false on error.
*/
/**************************************************************************/
bool Adafruit_IS31FL3741::setGlobalCurrent(uint8_t current) {
  selectPage(4);
  Adafruit_BusIO_Register gcurr_reg =
      Adafruit_BusIO_Register(_i2c_dev, IS3741_FUNCREG_GCURRENT);
  return gcurr_reg.write(current);
}

/**************************************************************************/
/*!
    @brief    Get the global current-mirror register setting.
    @returns  0 (off) to 255 (brightest)
*/
/**************************************************************************/
uint8_t Adafruit_IS31FL3741::getGlobalCurrent(void) {
  selectPage(4);
  Adafruit_BusIO_Register gcurr_reg =
      Adafruit_BusIO_Register(_i2c_dev, IS3741_FUNCREG_GCURRENT);
  return gcurr_reg.read();
}

/**************************************************************************/
/*!
    @brief    Allows changing of command register by writing 0xC5 to 0xFE.
    @returns  true if I2C command acknowledged, false on error.
*/
/**************************************************************************/
bool Adafruit_IS31FL3741::unlock(void) {
  Adafruit_BusIO_Register lock_reg =
      Adafruit_BusIO_Register(_i2c_dev, IS3741_COMMANDREGISTERLOCK);
  return lock_reg.write(0xC5);
}

/**************************************************************************/
/*!
    @brief    Select a given bank/page in the chip memory for subsequent
              reads/writes.
    @param    page  The IS41 page to switch to (0 to 4).
    @returns  true if I2C command acknowledged, false on invalid page or
              I2C error.
*/
/**************************************************************************/
bool Adafruit_IS31FL3741::selectPage(uint8_t page) {
  if (page < 5) {        // Valid page number?
    if (page == _page) { // If it matches the existing setting...
      return true;       // nice, we can skip re-setting the page!
    }
    _page = page; // Cache this page value

    unlock();
    Adafruit_BusIO_Register cmd_reg =
        Adafruit_BusIO_Register(_i2c_dev, IS3741_COMMANDREGISTER);
    return cmd_reg.write(page);
  }
  return false; // Invalid page
}

/**************************************************************************/
/*!
    @brief    Set either the PWM or scaling level for a single LED; used by
              the setLEDPWM() and setLEDscaling() functions, not directly.
    @param    first_page  Determines whether scaling or PWM will be set:
                          0 for PWM, 2 for scaling.
    @param    lednum      The individual LED to set: 0 to 350.
    @param    value       PWM or scaling value from 0 to 255.
    @returns  true if I2C command acknowledged, false if LED index is out
              of range or if I2C error.
    @note     This refers to individual LED registers as controlled by the
              driver, *not* RGB pixels -- that's a different thing handled
              in subclasses as needed.
*/
/**************************************************************************/
bool Adafruit_IS31FL3741::setLEDvalue(uint8_t first_page, uint16_t lednum,
                                      uint8_t value) {
  if (lednum < 351) {
    uint8_t cmd[2];
    cmd[1] = value;
    if (lednum < 180) {
      cmd[0] = (uint8_t)lednum;
      selectPage(first_page);
    } else {
      cmd[0] = (uint8_t)(lednum - 180);
      selectPage(first_page + 1);
    }
    return _i2c_dev->write(cmd, 2);
  }
  return false;
}

/**************************************************************************/
/*!
    @brief    Fill two pages of IS31FL3741 registers related to PWM levels
              or scaling; used by the fill() and setLEDscaling() functions,
              not directly.
    @param    first_page  First of two successive pages to fill; usually
                          0 or 2.
    @param    value       Setting from 0 to 255.
    @returns  true if I2C transfers completed successfully, false on error.
*/
/**************************************************************************/
bool Adafruit_IS31FL3741::fillTwoPages(uint8_t first_page, uint8_t value) {
  // The maximum I2C transfer size can be queried at run-time, but not
  // compile-time. Since we want a simple static-size buffer declared here,
  // we'll use the "safe bet" 32 byte transfer size, as requesting a large
  // chunk on the stack would be problematic for small devices like AVR.
  // So it's not completely optimal, but not pessimal either.
  uint8_t buf[32];
  // Buffer is initially filled with scale setting, then we'll replace
  // just the first byte with a register address on each pass.
  memset(buf, value, sizeof buf);

  uint8_t page_bytes = 180; // First of two pages is 180 bytes of stuff
  for (uint8_t page = 0; page < 2; page++) { // Two pages,
    selectPage(first_page + page);           // starting at first_page
    uint8_t addr = 0;    // Writes always start at reg 0 within page
    while (page_bytes) { // While there's data to write for page...
      uint8_t bytesThisPass = min((int)page_bytes, 31);
      buf[0] = addr;
      if (!_i2c_dev->write(buf, bytesThisPass + 1)) // +1 for addr
        return false;
      page_bytes -= bytesThisPass;
      addr += bytesThisPass;
    }
    page_bytes = 171; // Subsequent page is smaller
  }

  return true;
}

/**************************************************************************/
/*!
    @brief    Set the scaling level for a single LED.
    @param    lednum  The individual LED to set: 0 to 350.
    @param    scale   Level from 0 to 255.
    @returns  true if I2C command acknowledged, false if LED index is out
              of range or if I2C error.
    @note     This refers to individual LED registers as controlled by the
              driver, *not* RGB pixels -- that's a different thing handled
              in subclasses as needed.
*/
/**************************************************************************/
bool Adafruit_IS31FL3741::setLEDscaling(uint16_t lednum, uint8_t scale) {
  return setLEDvalue(2, lednum, scale); // Scaling is on pages 2/3
}

/**************************************************************************/
/*!
    @brief    Set the scaling level for all LEDs. Optimized for fewer
              I2C transfers vs. setting each individually.
    @param    scale  Level from 0 to 255.
    @returns  true if I2C transfers completed successfully, false on error.
*/
/**************************************************************************/
bool Adafruit_IS31FL3741::setLEDscaling(uint8_t scale) {
  return fillTwoPages(2, scale); // Fill pages 2 & 3 with value
}

/**************************************************************************/
/*!
    @brief    Set the PWM level for a single LED.
    @param    lednum  The individual LED to set: 0 to 350.
    @param    pwm     Level from 0 to 255.
    @returns  true if I2C command acknowledged, false if LED index is out
              of range or if I2C error.
    @note     This refers to individual LED registers as controlled by the
              driver, *not* RGB pixels -- that's a different thing handled
              in subclasses as needed.
*/
/**************************************************************************/
bool Adafruit_IS31FL3741::setLEDPWM(uint16_t lednum, uint8_t pwm) {
  return setLEDvalue(0, lednum, pwm); // PWM is on pages 0/1
}

/**************************************************************************/
/*!
    @brief    Set the PWM value for all LEDs - great for clearing the whole
              display at once. Optimized for fewer I2C transfers vs. setting
              each individually.
    @param    fillpwm   PWM level from 0 to 255, default is 0 (off).
    @returns  true if I2C transfers completed successfully, false on error.
*/
/**************************************************************************/
bool Adafruit_IS31FL3741::fill(uint8_t fillpwm) {
  return fillTwoPages(0, fillpwm); // Fill pages 0 & 1 with value
}

/*!
  @brief   Convert hue, saturation and value into a packed 32-bit RGB color
           that can be passed to setPixelColor() or Color565(). Swiped
           directly from Adafruit_NeoPixel.
  @param   hue  An unsigned 16-bit value, 0 to 65535, representing one full
                loop of the color wheel, which allows 16-bit hues to "roll
                over" while still doing the expected thing (and allowing
                more precision than the wheel() function that was common to
                prior NeoPixel examples).
  @param   sat  Saturation, 8-bit value, 0 (min or pure grayscale) to 255
                (max or pure hue). Default of 255 if unspecified.
  @param   val  Value (brightness), 8-bit value, 0 (min / black / off) to
                255 (max or full brightness). Default of 255 if unspecified.
  @return  Packed 32-bit RGB with the most significant byte set to 0.
           Result is linearly but not perceptually correct, so you may want
           to pass the result through a gamma function.
  @note    Yes, the name is unfortunate -- have uppercase ColorHSV() here,
           and lowercase color565() elsewhere. This is for compatibility
           with existing code from Adafruit_NeoPixel amd Adafruit_GFX, which
           were separately developed and used differing cases. The idea here
           is to help re-use existing Arduino sketch code, so don't "fix."
*/
uint32_t Adafruit_IS31FL3741::ColorHSV(uint16_t hue, uint8_t sat, uint8_t val) {

  uint8_t r, g, b;

  // Remap 0-65535 to 0-1529. Pure red is CENTERED on the 64K rollover;
  // 0 is not the start of pure red, but the midpoint...a few values above
  // zero and a few below 65536 all yield pure red (similarly, 32768 is the
  // midpoint, not start, of pure cyan). The 8-bit RGB hexcone (256 values
  // each for red, green, blue) really only allows for 1530 distinct hues
  // (not 1536, more on that below), but the full unsigned 16-bit type was
  // chosen for hue so that one's code can easily handle a contiguous color
  // wheel by allowing hue to roll over in either direction.
  hue = (hue * 1530L + 32768) / 65536;
  // Because red is centered on the rollover point (the +32768 above,
  // essentially a fixed-point +0.5), the above actually yields 0 to 1530,
  // where 0 and 1530 would yield the same thing. Rather than apply a
  // costly modulo operator, 1530 is handled as a special case below.

  // So you'd think that the color "hexcone" (the thing that ramps from
  // pure red, to pure yellow, to pure green and so forth back to red,
  // yielding six slices), and with each color component having 256
  // possible values (0-255), might have 1536 possible items (6*256),
  // but in reality there's 1530. This is because the last element in
  // each 256-element slice is equal to the first element of the next
  // slice, and keeping those in there this would create small
  // discontinuities in the color wheel. So the last element of each
  // slice is dropped...we regard only elements 0-254, with item 255
  // being picked up as element 0 of the next slice. Like this:
  // Red to not-quite-pure-yellow is:        255,   0, 0 to 255, 254,   0
  // Pure yellow to not-quite-pure-green is: 255, 255, 0 to   1, 255,   0
  // Pure green to not-quite-pure-cyan is:     0, 255, 0 to   0, 255, 254
  // and so forth. Hence, 1530 distinct hues (0 to 1529), and hence why
  // the constants below are not the multiples of 256 you might expect.

  // Convert hue to R,G,B (nested ifs faster than divide+mod+switch):
  if (hue < 510) { //         Red to Green-1
    b = 0;
    if (hue < 255) { //         Red to Yellow-1
      r = 255;
      g = hue;       //           g = 0 to 254
    } else {         //         Yellow to Green-1
      r = 510 - hue; //           r = 255 to 1
      g = 255;
    }
  } else if (hue < 1020) { // Green to Blue-1
    r = 0;
    if (hue < 765) { //         Green to Cyan-1
      g = 255;
      b = hue - 510;  //          b = 0 to 254
    } else {          //        Cyan to Blue-1
      g = 1020 - hue; //          g = 255 to 1
      b = 255;
    }
  } else if (hue < 1530) { // Blue to Red-1
    g = 0;
    if (hue < 1275) { //        Blue to Magenta-1
      r = hue - 1020; //          r = 0 to 254
      b = 255;
    } else { //                 Magenta to Red-1
      r = 255;
      b = 1530 - hue; //          b = 255 to 1
    }
  } else { //                 Last 0.5 Red (quicker than % operator)
    r = 255;
    g = b = 0;
  }

  // Apply saturation and value to R,G,B, pack into 32-bit result:
  uint32_t v1 = 1 + val;  // 1 to 256; allows >>8 instead of /255
  uint16_t s1 = 1 + sat;  // 1 to 256; same reason
  uint8_t s2 = 255 - sat; // 255 to 0
  return ((((((r * s1) >> 8) + s2) * v1) & 0xff00) << 8) |
         (((((g * s1) >> 8) + s2) * v1) & 0xff00) |
         (((((b * s1) >> 8) + s2) * v1) >> 8);
}

/**************************************************************************/
/*!
  @brief   A gamma-correction function for 32-bit packed RGB colors.
           Makes color transitions appear more perceptially correct.
  @param   x  Packed RGB color.
  @return  Gamma-adjusted packed color, can then be passed in one of the
           setPixelColor() functions. Like gamma8(), this uses a fixed
           gamma correction exponent of 2.6, which seems reasonably okay
           for average NeoPixels in average tasks. If you need finer
           control you'll need to provide your own gamma-correction
           function instead.
*/
/**************************************************************************/
uint32_t Adafruit_IS31FL3741::gamma32(uint32_t x) {
  uint8_t *y = (uint8_t *)&x;
  // All four bytes of a 32-bit value are filtered even if RGB (not WRGB),
  // to avoid a bunch of shifting and masking that would be necessary for
  // properly handling different endianisms (and each byte is a fairly
  // trivial operation, so it might not even be wasting cycles vs a check
  // and branch for the RGB case). In theory this might cause trouble *if*
  // someone's storing information in the unused most significant byte
  // of an RGB value, but this seems exceedingly rare and if it's
  // encountered in reality they can mask values going in or coming out.
  for (uint8_t i = 0; i < 4; i++)
    y[i] = gamma8(y[i]);
  return x; // Packed 32-bit return
}

// IS31FL3741 (BUFFERED) ---------------------------------------------------
// A few functions in the base glass get overloaded here, plus addition of
// buffered-specific show() function.

/**************************************************************************/
/*!
    @brief  Constructor for buffered IS31FL3741. LED data is stored in RAM
            and only pushed to device when show() is explicitly called.
*/
/**************************************************************************/
Adafruit_IS31FL3741_buffered::Adafruit_IS31FL3741_buffered()
    : Adafruit_IS31FL3741() {}

/**************************************************************************/
/*!
    @brief    Initialize I2C and IS31FL3741 hardware, clear LED buffer.
    @param    addr     I2C address where we expect to find the chip.
    @param    theWire  Pointer to TwoWire I2C bus to use, defaults to &Wire.
    @returns  true on success, false if chip isn't found.
*/
/**************************************************************************/
bool Adafruit_IS31FL3741_buffered::begin(uint8_t addr, TwoWire *theWire) {
  bool status = Adafruit_IS31FL3741::begin(addr, theWire);
  if (status) {                        // If I2C initialized OK,
    memset(ledbuf, 0, sizeof(ledbuf)); // clear the LED buffer
  }
  return status;
}

/**************************************************************************/
/*!
    @brief  Push buffered LED data from RAM to device.
    @note   This looks a lot like the base class' fillTwoPages() function,
            but works differently and they are not interchangeable or
            refactorable into a single function. This relies on the LED
            buffer that's part of the Adafruit_IS31FL3741_buffered object
            and does some temporary element swaps to make larger transfers
            if the host device allows. Really, don't.
*/
/**************************************************************************/
void Adafruit_IS31FL3741_buffered::show(void) {
  uint8_t *ptr = ledbuf;
  uint8_t chunk = _i2c_dev->maxBufferSize() - 1;

  uint8_t page_bytes = 180; // First page is 180 bytes of stuff
  for (uint8_t page = 0; page < 2; page++) {
    selectPage(page);
    uint8_t addr = 0;    // Writes always start at reg 0 within page
    while (page_bytes) { // While there's data to write for page...
      uint8_t bytesThisPass = min(page_bytes, chunk);
      // To avoid needing an extra I2C write buffer here (whose size may
      // vary by architecture, not knowable at compile-time), save the
      // ledbuf value at ptr, overwrite with the current register address,
      // write straight from ledbuf and then restore the saved value.
      // This is why there's an extra leading byte used in ledbuf.
      // All the LED-setting functions use getBuffer(), which returns
      // a pointer to the first LED at position #1, not #0.
      uint8_t save = *ptr;
      *ptr = addr;
      _i2c_dev->write(ptr, bytesThisPass + 1); // +1 for addr
      *ptr = save;
      page_bytes -= bytesThisPass;
      ptr += bytesThisPass;
      addr += bytesThisPass;
    }
    page_bytes = 171; // Subsequent page is smaller
  }
}

// INTERMEDIARY CLASSES FOR COLORS AND GFX ---------------------------------

/**************************************************************************/
/*!
    @brief  Constructor for GFX-subclassed IS31FL3741, so a common fill()
            function can be used across subclasses.
*/
/**************************************************************************/
Adafruit_IS31FL3741_colorGFX::Adafruit_IS31FL3741_colorGFX(uint8_t width,
                                                           uint8_t height,
                                                           IS3741_order order)
    : Adafruit_IS31FL3741(), Adafruit_IS31FL3741_ColorOrder(order),
      Adafruit_GFX(width, height) {}

/**************************************************************************/
/*!
    @brief  Sets all pixels of a GFX-subclassed, unbuffered object.
            Saves us implementing this in every subclass.
    @param  color  16-bit RGB565 packed color (expands to 888 for LEDs).
*/
/**************************************************************************/
void Adafruit_IS31FL3741_colorGFX::fill(uint16_t color) {
  // Fill must be done pixel-by-pixel due to different mappings & offsets
  // for each matrix type.
  for (uint8_t y = 0; y < height(); y++) {
    for (uint8_t x = 0; x < width(); x++) {
      drawPixel(x, y, color);
    }
  }
}

/**************************************************************************/
/*!
    @brief  Constructor for buffered-and-GFX-subclassed IS31FL3741,
            so a common fill() function can be used across subclasses.
*/
/**************************************************************************/
Adafruit_IS31FL3741_colorGFX_buffered::Adafruit_IS31FL3741_colorGFX_buffered(
    uint8_t width, uint8_t height, IS3741_order order)
    : Adafruit_IS31FL3741_buffered(), Adafruit_IS31FL3741_ColorOrder(order),
      Adafruit_GFX(width, height) {}

/**************************************************************************/
/*!
    @brief  Sets all pixels of a buffered-and-GFX-subclassed object.
            Saves us implementing this in every subclass.
    @param  color  16-bit RGB565 packed color (expands to 888 for LEDs).
*/
/**************************************************************************/
void Adafruit_IS31FL3741_colorGFX_buffered::fill(uint16_t color) {
  // If high and low bytes of color are the same...
  if ((color >> 8) == (color & 0xFF)) {
    // Can just memset the whole pixel buffer to that byte
    memset(&ledbuf[1], color & 0xFF, 351);
  } else {
    // Otherwise, fill must be done pixel-by-pixel due to
    // different mappings & offsets in parts of the matrix.
    for (uint8_t y = 0; y < height(); y++) {
      for (uint8_t x = 0; x < width(); x++) {
        drawPixel(x, y, color);
      }
    }
  }
}

// DEVICE-SPECIFIC SUBCLASSES ----------------------------------------------

// LUMISSIL EVAL BOARD (DIRECT, UNBUFFERED) --------------------------------

/**************************************************************************/
/*!
    @brief  Adafruit GFX low level accessor - sets an RGB pixel value,
            handles rotation and pixel arrangement.
    @param  x      The x position, starting with 0 for left-most side
    @param  y      The y position, starting with 0 for top-most side
    @param  color  16-bit RGB565 packed color (expands to 888 for LEDs).
*/
/**************************************************************************/
void Adafruit_IS31FL3741_EVB::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if ((x >= 0) && (y >= 0) && (x < width()) && (y < height())) {
    _IS31_ROTATE_(x, y);           // Handle GFX-style soft rotation
    _IS31_EXPAND_(color, r, g, b); // Expand GFX's RGB565 color to RGB888

    // Map x/y to device-specific pixel layout
    uint16_t offset = ((y > 2) ? (x * 10 + 12 - y) : (92 + x * 3 - y)) * 3;

    /*
    Serial.print("("); Serial.print(x);
    Serial.print(", "); Serial.print(y);
    Serial.print(") -> "); Serial.println(offset);
    */

    setLEDPWM(offset + rOffset, r);
    setLEDPWM(offset + gOffset, g);
    setLEDPWM(offset + bOffset, b);
  }
}

// LUMISSIL EVAL BOARD (BUFFERED) ------------------------------------------

/**************************************************************************/
/*!
    @brief  Adafruit GFX low level accessor - sets an RGB pixel value,
            handles rotation and pixel arrangement.
    @param  x      The x position, starting with 0 for left-most side
    @param  y      The y position, starting with 0 for top-most side
    @param  color  16-bit RGB565 packed color (expands to 888 for LEDs).
*/
/**************************************************************************/
void Adafruit_IS31FL3741_EVB_buffered::drawPixel(int16_t x, int16_t y,
                                                 uint16_t color) {
  if ((x >= 0) && (y >= 0) && (x < width()) && (y < height())) {
    _IS31_ROTATE_(x, y);           // Handle GFX-style soft rotation
    _IS31_EXPAND_(color, r, g, b); // Expand GFX's RGB565 color to RGB888

    // Map x/y to device-specific pixel layout
    uint16_t offset = ((y > 2) ? (x * 10 + 12 - y) : (92 + x * 3 - y)) * 3;

    /*
    Serial.print("("); Serial.print(x);
    Serial.print(", "); Serial.print(y);
    Serial.print(") -> "); Serial.println(offset);
    */

    uint8_t *ptr = &ledbuf[1 + offset];
    ptr[rOffset] = r;
    ptr[gOffset] = g;
    ptr[bOffset] = b;
  }
}

// STEMMA QT MATRIX (DIRECT) -----------------------------------------------

/**************************************************************************/
/*!
    @brief  Adafruit GFX low level accessor - sets an RGB pixel value,
            handles rotation and pixel arrangement.
    @param  x      The x position, starting with 0 for left-most side.
    @param  y      The y position, starting with 0 for top-most side.
    @param  color  16-bit RGB565 packed color (expands to 888 for LEDs).
*/
/**************************************************************************/
void Adafruit_IS31FL3741_QT::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if ((x >= 0) && (y >= 0) && (x < width()) && (y < height())) {
    _IS31_ROTATE_(x, y);           // Handle GFX-style soft rotation
    _IS31_EXPAND_(color, r, g, b); // Expand GFX's RGB565 color to RGB888

    /*
    Serial.print("("); Serial.print(x);
    Serial.print(", "); Serial.print(y);
    Serial.print(") -> 0x");
    */

    // Remap the row (y)
    static const uint8_t rowmap[] = {8, 5, 4, 3, 2, 1, 0, 7, 6};
    y = rowmap[y];

    uint16_t offset = (x + ((x < 10) ? (y * 10) : (80 + y * 3))) * 3;
    // Serial.println(offset, HEX);

    if ((x & 1) || (x == 12)) { // Odd columns + last column
      // Rearrange color order vs constructor. Not a simple swap,
      // needs to pass through table, or essentially (n + 2) % 3.
      static const uint8_t remap[] = {2, 0, 1};
      setLEDPWM(offset + remap[rOffset], r);
      setLEDPWM(offset + remap[gOffset], g);
      setLEDPWM(offset + remap[bOffset], b);
    } else {                          // Even columns
      setLEDPWM(offset + rOffset, r); // Color order follows constructor
      setLEDPWM(offset + gOffset, g);
      setLEDPWM(offset + bOffset, b);
    }
  }
}

// STEMMA QT MATRIX (BUFFERED) ---------------------------------------------

/**************************************************************************/
/*!
    @brief         Adafruit GFX low level accessor - sets an RGB pixel value,
                   handles rotation and pixel arrangement, unlike setLEDPWM.
    @param  x      The x position, starting with 0 for left-most side
    @param  y      The y position, starting with 0 for top-most side
    @param  color  16-bit RGB565 packed color (expands to 888 for LEDs).
*/
/**************************************************************************/
void Adafruit_IS31FL3741_QT_buffered::drawPixel(int16_t x, int16_t y,
                                                uint16_t color) {
  if ((x >= 0) && (y >= 0) && (x < width()) && (y < height())) {
    _IS31_ROTATE_(x, y);           // Handle GFX-style soft rotation
    _IS31_EXPAND_(color, r, g, b); // Expand GFX's RGB565 color to RGB888

    /*
    Serial.print("("); Serial.print(x);
    Serial.print(", "); Serial.print(y);
    Serial.print(") -> 0x");
    */

    // Remap the row (y)
    static const uint8_t rowmap[] = {8, 5, 4, 3, 2, 1, 0, 7, 6};
    y = rowmap[y];

    uint16_t offset = (x + ((x < 10) ? (y * 10) : (80 + y * 3))) * 3;
    // Serial.println(offset, HEX);

    uint8_t *ptr = &ledbuf[1 + offset];
    if ((x & 1) || (x == 12)) { // Odd columns + last column
      // Rearrange color order vs constructor. Not a simple swap,
      // needs to pass through table, or essentially (n + 2) % 3.
      static const uint8_t remap[] = {2, 0, 1};
      ptr[remap[rOffset]] = r;
      ptr[remap[gOffset]] = g;
      ptr[remap[bOffset]] = b;
    } else {            // Even columns
      ptr[rOffset] = r; // Color order follows constructor
      ptr[gOffset] = g;
      ptr[bOffset] = b;
    }
  }
}

// LED GLASSES -------------------------------------------------------------
// There are two implementations of this. First here are the EyeLights
// classes (direct and buffered versions), which are a little simpler to
// use. Later are the Glasses classes, an earlier implementation that
// required a few extra steps, but kept for compatibility with existing
// code. Consider the latter deprecated -- new projects should only use
// the EyeLights classes.

// Remap table for pixel (X,Y) positions to LED indices, for drawPixel()
// functions. LED indices are for blue, green, red for the EyeLights as
// they originally shipped, hence default IS3741_BGR order in constructor,
// to match the QT matrix LEDs. If there's a switch to different LEDs in
// the future, pass a different order to the constructor.
static const uint16_t PROGMEM glassesmatrix_ledmap[18 * 5 * 3] = {
    65535, 65535, 65535, // (0,0) (clipped, corner)
    10,    8,     9,     // (0,1) / right ring pixel 20
    13,    11,    12,    // (0,2) / 19
    16,    14,    15,    // (0,3) / 18
    4,     2,     3,     // (0,4) / 17
    217,   215,   216,   // (1,0) / right ring pixel #21
    220,   218,   219,   // (1,1)
    223,   221,   222,   // (1,2)
    226,   224,   225,   // (1,3)
    214,   212,   213,   // (1,4)
    187,   185,   186,   // (2,0)
    190,   188,   189,   // (2,1)
    193,   191,   192,   // (2,2)
    196,   194,   195,   // (2,3)
    184,   182,   183,   // (2,4)
    37,    35,    36,    // (3,0)
    40,    38,    39,    // (3,1)
    43,    41,    42,    // (3,2)
    46,    44,    45,    // (3,3)
    34,    32,    33,    // (3,4)
    67,    65,    66,    // (4,0)
    70,    68,    69,    // (4,1)
    73,    71,    72,    // (4,2)
    76,    74,    75,    // (4,3)
    64,    62,    63,    // (4,4)
    97,    95,    96,    // (5,0)
    100,   98,    99,    // (5,1)
    103,   101,   102,   // (5,2)
    106,   104,   105,   // (5,3)
    94,    92,    93,    // (5,4)
    127,   125,   126,   // (6,0) / right ring pixel 3
    130,   128,   129,   // (6,1)
    133,   131,   132,   // (6,2)
    136,   134,   135,   // (6,3)
    124,   122,   123,   // (6,4)
    157,   155,   156,   // (7,0)
    160,   158,   159,   // (7,1)
    163,   161,   162,   // (7,2) / right ring pixel 5
    166,   164,   165,   // (7,3) / 6
    244,   242,   243,   // (7,4) / 7
    247,   245,   246,   // (8,0)
    250,   248,   249,   // (8,1)
    253,   251,   252,   // (8,2)
    256,   254,   255,   // (8,3)
    65535, 65535, 65535, // (8,4) (clipped, nose bridge)
    345,   347,   346,   // (9,0)
    342,   344,   343,   // (9,1)
    267,   269,   268,   // (9,2)
    263,   265,   264,   // (9,3)
    65535, 65535, 65535, // (9,4) (clipped, nose bridge)
    336,   338,   337,   // (10,0)
    333,   335,   334,   // (10,1)
    237,   239,   238,   // (10,2) / left ring pixel 19
    233,   235,   234,   // (10,3) / 18
    348,   262,   349,   // (10,4) / 17
    327,   329,   328,   // (11,0) / left ring pixel 21
    324,   326,   325,   // (11,1)
    207,   209,   208,   // (11,2)
    203,   205,   204,   // (11,3)
    330,   202,   331,   // (11,4)
    318,   320,   319,   // (12,0)
    315,   317,   316,   // (12,1)
    177,   179,   178,   // (12,2)
    173,   175,   174,   // (12,3)
    321,   172,   322,   // (12,4)
    309,   311,   310,   // (13,0)
    306,   308,   307,   // (13,1)
    147,   149,   148,   // (13,2)
    143,   145,   144,   // (13,3)
    312,   142,   313,   // (13,4)
    300,   302,   301,   // (14,0)
    297,   299,   298,   // (14,1)
    117,   119,   118,   // (14,2)
    113,   115,   114,   // (14,3)
    303,   112,   304,   // (14,4)
    291,   293,   292,   // (15,0)
    288,   290,   289,   // (15,1)
    87,    89,    88,    // (15,2)
    83,    85,    84,    // (15,3)
    294,   82,    295,   // (15,4)
    282,   284,   283,   // (16,0) / left ring pixel 3
    279,   281,   280,   // (16,1)
    57,    59,    58,    // (16,2)
    53,    55,    54,    // (16,3)
    285,   52,    286,   // (16,4)
    65535, 65535, 65535, // (17,0) (clipped, corner)
    270,   272,   271,   // (17,1) / left ring pixel 4
    27,    29,    28,    // (17,2) / 5
    23,    25,    24,    // (17,3) / 6
    276,   22,    277,   // (17,4) / 7
};

// Remap tables for LED ring pixel positions to LED indices, for
// setPixelColor() functions.
static const uint16_t PROGMEM left_ring_map[24 * 3] = {
    341, 210, 211, // 0
    332, 180, 181, // 1
    323, 150, 151, // 2
    127, 125, 126, // 3
    154, 152, 153, // 4
    163, 161, 162, // 5
    166, 164, 165, // 6
    244, 242, 243, // 7
    259, 257, 258, // 8
    169, 167, 168, // 9
    139, 137, 138, // 10
    109, 107, 108, // 11
    79,  77,  78,  // 12
    49,  47,  48,  // 13
    199, 197, 198, // 14
    229, 227, 228, // 15
    19,  17,  18,  // 16
    4,   2,   3,   // 17
    16,  14,  15,  // 18
    13,  11,  12,  // 19
    10,  8,   9,   // 20
    217, 215, 216, // 21
    7,   5,   6,   // 22
    350, 240, 241, // 23
};
static const uint16_t PROGMEM right_ring_map[24 * 3] = {
    287, 30,  31,  // 0
    278, 0,   1,   // 1
    273, 275, 274, // 2
    282, 284, 283, // 3
    270, 272, 271, // 4
    27,  29,  28,  // 5
    23,  25,  24,  // 6
    276, 22,  277, // 7
    20,  26,  21,  // 8
    50,  56,  51,  // 9
    80,  86,  81,  // 10
    110, 116, 111, // 11
    140, 146, 141, // 12
    170, 176, 171, // 13
    200, 206, 201, // 14
    230, 236, 231, // 15
    260, 266, 261, // 16
    348, 262, 349, // 17
    233, 235, 234, // 18
    237, 239, 238, // 19
    339, 232, 340, // 20
    327, 329, 328, // 21
    305, 90,  91,  // 22
    296, 60,  61,  // 23
};

// GFXcanvas16 is RGB565 color while the LEDs are RGB888, so during 1:3
// downsampling we recover some intermediate shades and apply gamma
// correction for better linearity. Tables are used to avoid floating-point
// math. They appear large here but reside in flash, not too bad.
// To regenerate tables (e.g. different gamma), in Python:
// print(str([int((x / (31*9)) ** 2.6 * 255 + 0.5) for x in range(31*9+1)]))
// Then edit braces and clang-format result. For green, use 63 instead of 31.
// Red & blue table is 280 bytes, green table is 568 bytes. 848 total.
static const uint8_t PROGMEM gammaRB[] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,
    1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,
    2,   2,   2,   3,   3,   3,   3,   3,   3,   4,   4,   4,   4,   4,   4,
    5,   5,   5,   5,   6,   6,   6,   6,   6,   7,   7,   7,   8,   8,   8,
    8,   9,   9,   9,   10,  10,  10,  11,  11,  11,  12,  12,  12,  13,  13,
    13,  14,  14,  15,  15,  15,  16,  16,  17,  17,  18,  18,  19,  19,  20,
    20,  21,  21,  22,  22,  23,  23,  24,  24,  25,  25,  26,  27,  27,  28,
    28,  29,  30,  30,  31,  32,  32,  33,  34,  34,  35,  36,  36,  37,  38,
    39,  39,  40,  41,  42,  42,  43,  44,  45,  46,  47,  47,  48,  49,  50,
    51,  52,  53,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,
    65,  66,  67,  68,  69,  70,  71,  73,  74,  75,  76,  77,  78,  79,  80,
    82,  83,  84,  85,  86,  88,  89,  90,  91,  93,  94,  95,  97,  98,  99,
    100, 102, 103, 105, 106, 107, 109, 110, 112, 113, 114, 116, 117, 119, 120,
    122, 123, 125, 126, 128, 130, 131, 133, 134, 136, 137, 139, 141, 142, 144,
    146, 147, 149, 151, 153, 154, 156, 158, 160, 161, 163, 165, 167, 169, 171,
    172, 174, 176, 178, 180, 182, 184, 186, 188, 190, 192, 194, 196, 198, 200,
    202, 204, 206, 208, 210, 212, 214, 217, 219, 221, 223, 225, 227, 230, 232,
    234, 236, 239, 241, 243, 246, 248, 250, 253, 255};
static const uint8_t PROGMEM gammaG[] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,
    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
    1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,
    2,   2,   2,   2,   2,   2,   3,   3,   3,   3,   3,   3,   3,   3,   3,
    3,   3,   3,   3,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,
    4,   5,   5,   5,   5,   5,   5,   5,   5,   5,   6,   6,   6,   6,   6,
    6,   6,   6,   6,   7,   7,   7,   7,   7,   7,   7,   7,   8,   8,   8,
    8,   8,   8,   8,   9,   9,   9,   9,   9,   9,   10,  10,  10,  10,  10,
    10,  10,  11,  11,  11,  11,  11,  11,  12,  12,  12,  12,  12,  13,  13,
    13,  13,  13,  13,  14,  14,  14,  14,  14,  15,  15,  15,  15,  15,  16,
    16,  16,  16,  17,  17,  17,  17,  17,  18,  18,  18,  18,  19,  19,  19,
    19,  20,  20,  20,  20,  20,  21,  21,  21,  21,  22,  22,  22,  23,  23,
    23,  23,  24,  24,  24,  24,  25,  25,  25,  26,  26,  26,  26,  27,  27,
    27,  28,  28,  28,  28,  29,  29,  29,  30,  30,  30,  31,  31,  31,  32,
    32,  32,  33,  33,  33,  34,  34,  34,  35,  35,  35,  36,  36,  36,  37,
    37,  37,  38,  38,  38,  39,  39,  40,  40,  40,  41,  41,  41,  42,  42,
    43,  43,  43,  44,  44,  45,  45,  45,  46,  46,  47,  47,  47,  48,  48,
    49,  49,  50,  50,  50,  51,  51,  52,  52,  53,  53,  54,  54,  54,  55,
    55,  56,  56,  57,  57,  58,  58,  59,  59,  60,  60,  60,  61,  61,  62,
    62,  63,  63,  64,  64,  65,  65,  66,  66,  67,  67,  68,  69,  69,  70,
    70,  71,  71,  72,  72,  73,  73,  74,  74,  75,  75,  76,  77,  77,  78,
    78,  79,  79,  80,  81,  81,  82,  82,  83,  83,  84,  85,  85,  86,  86,
    87,  88,  88,  89,  89,  90,  91,  91,  92,  93,  93,  94,  94,  95,  96,
    96,  97,  98,  98,  99,  100, 100, 101, 102, 102, 103, 104, 104, 105, 106,
    106, 107, 108, 108, 109, 110, 110, 111, 112, 113, 113, 114, 115, 115, 116,
    117, 118, 118, 119, 120, 121, 121, 122, 123, 123, 124, 125, 126, 127, 127,
    128, 129, 130, 130, 131, 132, 133, 133, 134, 135, 136, 137, 137, 138, 139,
    140, 141, 141, 142, 143, 144, 145, 146, 146, 147, 148, 149, 150, 151, 151,
    152, 153, 154, 155, 156, 157, 157, 158, 159, 160, 161, 162, 163, 164, 164,
    165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 174, 175, 176, 177, 178,
    179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193,
    194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208,
    209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 224,
    225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 236, 237, 238, 239, 240,
    241, 242, 243, 245, 246, 247, 248, 249, 250, 252, 253, 254, 255};

/**************************************************************************/
/*!
    @brief  Constructor for EyeLights LED ring. This is a base class used
            by both the direct and buffered variants. Not invoked by user
            code.
    @param  parent   void* pointer to parent EyeLights object this is
                     attached to (may be direct or buffered, hence void*
                     rather than specific type).
    @param  isRight  true if right ring, false if left.
    @note   Constructor is here in the .cpp instead of the .h because it
            references the static tables above.
*/
/**************************************************************************/
Adafruit_EyeLights_Ring_Base::Adafruit_EyeLights_Ring_Base(void *parent,
                                                           bool isRight)
    : parent(parent), ring_map(isRight ? right_ring_map : left_ring_map) {}

// EYELIGHTS (DIRECT, UNBUFFERED) ------------------------------------------

/**************************************************************************/
/*!
    @brief  Set color of one pixel of one direct (unbuffered) EyeLights
            ring, from a single packed RGB value.
    @param  n      Index of pixel to set (0-23).
    @param  color  RGB888 (24-bit) color, a la NeoPixel.
*/
/**************************************************************************/
void Adafruit_EyeLights_Ring::setPixelColor(int16_t n, uint32_t color) {
  if ((n >= 0) && (n < 24)) {
    Adafruit_EyeLights *eyelights = (Adafruit_EyeLights *)parent;
    _IS31_SCALE_RGB_(color, r, g, b, _brightness);
    n *= 3;
    eyelights->setLEDPWM(pgm_read_word(&ring_map[n + eyelights->rOffset]), r);
    eyelights->setLEDPWM(pgm_read_word(&ring_map[n + eyelights->gOffset]), g);
    eyelights->setLEDPWM(pgm_read_word(&ring_map[n + eyelights->bOffset]), b);
  }
}

/**************************************************************************/
/*!
    @brief  Set color of one pixel of one direct (unbuffered) EyeLights
            ring, from separate R,G,B values.
    @param  n  Index of pixel to set (0-23).
    @param  r  Red component (0-255) of color, a la NeoPixel.
    @param  g  Green component (0-255) of color, a la NeoPixel.
    @param  b  Blue component (0-255) of color, a la NeoPixel.
*/
/**************************************************************************/
void Adafruit_EyeLights_Ring::setPixelColor(int16_t n, uint8_t r, uint8_t g,
                                            uint8_t b) {
  if ((n >= 0) && (n < 24)) {
    Adafruit_EyeLights *eyelights = (Adafruit_EyeLights *)parent;
    _IS31_SCALE_RGB_SEPARATE_(r, g, b, _brightness);
    n *= 3;
    eyelights->setLEDPWM(pgm_read_word(&ring_map[n + eyelights->rOffset]), r);
    eyelights->setLEDPWM(pgm_read_word(&ring_map[n + eyelights->gOffset]), g);
    eyelights->setLEDPWM(pgm_read_word(&ring_map[n + eyelights->bOffset]), b);
  }
}

/**************************************************************************/
/*!
    @brief  Fill all pixels of one direct (unbuffered) EyeLights ring to
            same color, from a single packed value.
    @param  color  RGB888 (24-bit) color, a la NeoPixel.
*/
/**************************************************************************/
void Adafruit_EyeLights_Ring::fill(uint32_t color) {
  Adafruit_EyeLights *eyelights = (Adafruit_EyeLights *)parent;
  _IS31_SCALE_RGB_(color, r, g, b, _brightness);
  for (uint8_t n = 0; n < 24 * 3; n += 3) {
    eyelights->setLEDPWM(pgm_read_word(&ring_map[n + eyelights->rOffset]), r);
    eyelights->setLEDPWM(pgm_read_word(&ring_map[n + eyelights->gOffset]), g);
    eyelights->setLEDPWM(pgm_read_word(&ring_map[n + eyelights->bOffset]), b);
  }
}

/**************************************************************************/
/*!
    @brief  Fill all pixels of one direct (unbuffered) EyeLights ring to
            same color, from separate R,G,B values.
    @param  r  Red component (0-255) of color, a la NeoPixel.
    @param  g  Green component (0-255) of color, a la NeoPixel.
    @param  b  Blue component (0-255) of color, a la NeoPixel.
*/
/**************************************************************************/
void Adafruit_EyeLights_Ring::fill(uint8_t r, uint8_t g, uint8_t b) {
  Adafruit_EyeLights *eyelights = (Adafruit_EyeLights *)parent;
  _IS31_SCALE_RGB_SEPARATE_(r, g, b, _brightness);
  for (uint8_t n = 0; n < 24 * 3; n += 3) {
    eyelights->setLEDPWM(pgm_read_word(&ring_map[n + eyelights->rOffset]), r);
    eyelights->setLEDPWM(pgm_read_word(&ring_map[n + eyelights->gOffset]), g);
    eyelights->setLEDPWM(pgm_read_word(&ring_map[n + eyelights->bOffset]), b);
  }
}

/**************************************************************************/
/*!
    @brief         Adafruit GFX low level accessor - sets an RGB pixel value
                   on direct (unbuffered) EyeLights LED matrix.
    @param  x      The x position, starting with 0 for left-most side
    @param  y      The y position, starting with 0 for top-most side
    @param  color  16-bit RGB565 packed color (expands to 888 for LEDs).
*/
/**************************************************************************/
void Adafruit_EyeLights::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if ((x >= 0) && (y >= 0) && (x < width()) && (y < height())) {
    _IS31_ROTATE_(x, y);           // Handle GFX-style soft rotation
    _IS31_EXPAND_(color, r, g, b); // Expand GFX's RGB565 color to RGB888
    x = (x * 5 + y) * 3;           // Starting index into the led table above
                                   // table is brg
    uint16_t ridx = pgm_read_word(&glassesmatrix_ledmap[x + rOffset]);
    if (ridx != 65535) {
      uint16_t gidx = pgm_read_word(&glassesmatrix_ledmap[x + gOffset]);
      uint16_t bidx = pgm_read_word(&glassesmatrix_ledmap[x + bOffset]);
      setLEDPWM(ridx, r);
      setLEDPWM(gidx, g);
      setLEDPWM(bidx, b);
    }
  }
}

/**************************************************************************/
/*!
    @brief  Scales associated canvas (if one was requested via constructor)
            1:3 with antialiasing & gamma correction. Note that this
            overwrites ALL pixels within the matrix area, including those
            shared with the rings. This is different than using normal
            drawing operations directly to the low-resolution matrix,
            where these ops are "transparent" and empty pixels don't
            overwrite the rings.
*/
/**************************************************************************/
void Adafruit_EyeLights::scale(void) {
  if (canvas) {
    uint16_t *src = canvas->getBuffer();
    // Outer x/y loops are column-major on purpose (less pointer math)
    for (int x = 0; x < 18; x++) {
      uint16_t *ptr = &src[x * 3]; // Entry along top scan line w/x offset
      for (int y = 0; y < 5; y++) {
        uint16_t rsum = 0, gsum = 0, bsum = 0;
        // Inner x/y loops are row-major on purpose (less pointer math)
        for (uint8_t yy = 0; yy < 3; yy++) {
          for (uint8_t xx = 0; xx < 3; xx++) {
            uint16_t rgb = ptr[xx];
            rsum += rgb >> 11;         // Accumulate 5 bits red,
            gsum += (rgb >> 5) & 0x3F; // 6 bits green,
            bsum += rgb & 0x1F;        // 5 bits blue
          }
          ptr += canvas->width(); // Advance one scan line
        }
        uint16_t base = (x * 5 + y) * 3; // Offset into ledmap
        uint16_t ridx = pgm_read_word(&glassesmatrix_ledmap[base + rOffset]);
        if (ridx != 65535) {
          uint16_t gidx = pgm_read_word(&glassesmatrix_ledmap[base + gOffset]);
          uint16_t bidx = pgm_read_word(&glassesmatrix_ledmap[base + bOffset]);
          setLEDPWM(ridx, pgm_read_byte(&gammaRB[rsum]));
          setLEDPWM(gidx, pgm_read_byte(&gammaG[gsum]));
          setLEDPWM(bidx, pgm_read_byte(&gammaRB[bsum]));
        }
      }
    }
  }
}

// EYELIGHTS (BUFFERED) ----------------------------------------------------

/**************************************************************************/
/*!
    @brief  Set color of one pixel of one buffered EyeLights ring, from a
            single packed RGB value.
            No immediate effect on LEDs; must follow up with show().
    @param  n      Index of pixel to set (0-23).
    @param  color  RGB888 (24-bit) color, a la NeoPixel.
*/
/**************************************************************************/
void Adafruit_EyeLights_Ring_buffered::setPixelColor(int16_t n,
                                                     uint32_t color) {
  if ((n >= 0) && (n < 24)) {
    Adafruit_EyeLights_buffered *eyelights =
        (Adafruit_EyeLights_buffered *)parent;
    uint8_t *ledbuf = eyelights->getBuffer();
    _IS31_SCALE_RGB_(color, r, g, b, _brightness);
    n *= 3;
    ledbuf[pgm_read_word(&ring_map[n + eyelights->rOffset])] = r;
    ledbuf[pgm_read_word(&ring_map[n + eyelights->gOffset])] = g;
    ledbuf[pgm_read_word(&ring_map[n + eyelights->bOffset])] = b;
  }
}

/**************************************************************************/
/*!
    @brief  Set color of one pixel of one buffered EyeLights ring, from
            separate R,G,B values. No immediate effect on LEDs; must
            follow up with show().
    @param  n  Index of pixel to set (0-23).
    @param  r  Red component (0-255) of color, a la NeoPixel.
    @param  g  Green component (0-255) of color, a la NeoPixel.
    @param  b  Blue component (0-255) of color, a la NeoPixel.
*/
/**************************************************************************/
void Adafruit_EyeLights_Ring_buffered::setPixelColor(int16_t n, uint8_t r,
                                                     uint8_t g, uint8_t b) {
  if ((n >= 0) && (n < 24)) {
    Adafruit_EyeLights_buffered *eyelights =
        (Adafruit_EyeLights_buffered *)parent;
    uint8_t *ledbuf = eyelights->getBuffer();
    _IS31_SCALE_RGB_SEPARATE_(r, g, b, _brightness);
    n *= 3;
    ledbuf[pgm_read_word(&ring_map[n + eyelights->rOffset])] = r;
    ledbuf[pgm_read_word(&ring_map[n + eyelights->gOffset])] = g;
    ledbuf[pgm_read_word(&ring_map[n + eyelights->bOffset])] = b;
  }
}

/**************************************************************************/
/*!
    @brief  Fill all pixels of one buffered EyeLights ring to same color,
            from a single packed R,G,B value. No immediate effect on LEDs;
            must follow up with show().
    @param  color  RGB888 (24-bit) color, a la NeoPixel.
*/
/**************************************************************************/
void Adafruit_EyeLights_Ring_buffered::fill(uint32_t color) {
  Adafruit_EyeLights_buffered *eyelights =
      (Adafruit_EyeLights_buffered *)parent;
  uint8_t *ledbuf = eyelights->getBuffer();
  _IS31_SCALE_RGB_(color, r, g, b, _brightness);
  for (uint8_t n = 0; n < 24 * 3; n += 3) {
    ledbuf[pgm_read_word(&ring_map[n + eyelights->rOffset])] = r;
    ledbuf[pgm_read_word(&ring_map[n + eyelights->gOffset])] = g;
    ledbuf[pgm_read_word(&ring_map[n + eyelights->bOffset])] = b;
  }
}

/**************************************************************************/
/*!
    @brief  Fill all pixels of one buffered EyeLights ring to same color,
            from separate R,G,B values. No immediate effect on LEDs; must
            follow up with show().
    @param  r  Red component (0-255) of color, a la NeoPixel.
    @param  g  Green component (0-255) of color, a la NeoPixel.
    @param  b  Blue component (0-255) of color, a la NeoPixel.
*/
/**************************************************************************/
void Adafruit_EyeLights_Ring_buffered::fill(uint8_t r, uint8_t g, uint8_t b) {
  Adafruit_EyeLights_buffered *eyelights =
      (Adafruit_EyeLights_buffered *)parent;
  uint8_t *ledbuf = eyelights->getBuffer();
  _IS31_SCALE_RGB_SEPARATE_(r, g, b, _brightness);
  for (uint8_t n = 0; n < 24 * 3; n += 3) {
    ledbuf[pgm_read_word(&ring_map[n + eyelights->rOffset])] = r;
    ledbuf[pgm_read_word(&ring_map[n + eyelights->gOffset])] = g;
    ledbuf[pgm_read_word(&ring_map[n + eyelights->bOffset])] = b;
  }
}

/**************************************************************************/
/*!
    @brief         Adafruit GFX low level accessor - sets an RGB pixel value
                   on buffered EyeLights LED matrix. No immediate effect on
                   LEDs, must follow up with show().
    @param  x      The x position, starting with 0 for left-most side
    @param  y      The y position, starting with 0 for top-most side
    @param  color  16-bit RGB565 packed color (expands to 888 for LEDs).
*/
/**************************************************************************/
void Adafruit_EyeLights_buffered::drawPixel(int16_t x, int16_t y,
                                            uint16_t color) {
  if ((x >= 0) && (x < width()) && (y >= 0) && (y < height())) {
    _IS31_ROTATE_(x, y); // Handle GFX-style soft rotation
    x = (x * 5 + y) * 3; // Base index into ledmap
    uint16_t ridx = pgm_read_word(&glassesmatrix_ledmap[x + rOffset]);
    if (ridx != 65535) {
      uint16_t gidx = pgm_read_word(&glassesmatrix_ledmap[x + gOffset]);
      uint16_t bidx = pgm_read_word(&glassesmatrix_ledmap[x + bOffset]);
      uint8_t *ledbuf = getBuffer();
      _IS31_EXPAND_(color, r, g, b); // Expand GFX's RGB565 color to RGB888
      ledbuf[ridx] = r;
      ledbuf[gidx] = g;
      ledbuf[bidx] = b;
    }
  }
}

/**************************************************************************/
/*!
    @brief  Scales associated canvas (if one was requested via constructor)
            1:3 with antialiasing & gamma correction. No immediate effect
            on LEDs; must follow up with show(). Note that this overwrites
            ALL pixels within the matrix area, including those shared with
            the rings. This is different than using normal drawing
            operations directly to the low-resolution matrix, where these
            ops are "transparent" and empty pixels don't overwrite the
            rings.
*/
/**************************************************************************/
void Adafruit_EyeLights_buffered::scale(void) {
  if (canvas) {
    uint16_t *src = canvas->getBuffer();
    uint8_t *ledbuf = getBuffer();
    // Outer x/y loops are column-major on purpose (less pointer math)
    for (int x = 0; x < 18; x++) {
      uint16_t *ptr = &src[x * 3]; // Entry along top scan line w/x offset
      for (int y = 0; y < 5; y++) {
        uint16_t rsum = 0, gsum = 0, bsum = 0;
        // Inner x/y loops are row-major on purpose (less pointer math)
        for (uint8_t yy = 0; yy < 3; yy++) {
          for (uint8_t xx = 0; xx < 3; xx++) {
            uint16_t rgb = ptr[xx];
            rsum += rgb >> 11;         // Accumulate 5 bits red,
            gsum += (rgb >> 5) & 0x3F; // 6 bits green,
            bsum += rgb & 0x1F;        // 5 bits blue
          }
          ptr += canvas->width(); // Advance one scan line
        }
        uint16_t base = (x * 5 + y) * 3; // Offset into ledmap
        uint16_t ridx = pgm_read_word(&glassesmatrix_ledmap[base + rOffset]);
        if (ridx != 65535) {
          uint16_t gidx = pgm_read_word(&glassesmatrix_ledmap[base + gOffset]);
          uint16_t bidx = pgm_read_word(&glassesmatrix_ledmap[base + bOffset]);
          ledbuf[ridx] = pgm_read_byte(&gammaRB[rsum]);
          ledbuf[gidx] = pgm_read_byte(&gammaG[gsum]);
          ledbuf[bidx] = pgm_read_byte(&gammaRB[bsum]);
        }
      }
    }
  }
}

// ORIGINAL LED GLASSES API (DIRECT, UNBUFFERED) ---------------------------
// These classes and functions are deprecated in favor of the EyeLights
// versions, which are a bit simpler to use. Code is kept around for
// compatibility but might be removed in the future.

/**************************************************************************/
/*!
    @brief         Adafruit GFX low level accessor - sets an RGB pixel value,
                   handles rotation and pixel arrangement, unlike setLEDPWM.
    @param  x      The x position, starting with 0 for left-most side
    @param  y      The y position, starting with 0 for top-most side
    @param  color  16-bit RGB565 packed color (expands to 888 for LEDs).
*/
/**************************************************************************/
void Adafruit_IS31FL3741_GlassesMatrix::drawPixel(int16_t x, int16_t y,
                                                  uint16_t color) {
  if ((x >= 0) && (y >= 0) && (x < width()) && (y < height())) {
    _IS31_ROTATE_(x, y);           // Handle GFX-style soft rotation
    _IS31_EXPAND_(color, r, g, b); // Expand GFX's RGB565 color to RGB888

    x = (x * 5 + y) * 3; // Starting index into the led table above
    uint16_t ridx = pgm_read_word(&glassesmatrix_ledmap[x + 2]);
    if (ridx != 65535) {
      uint16_t gidx = pgm_read_word(&glassesmatrix_ledmap[x + 1]);
      uint16_t bidx = pgm_read_word(&glassesmatrix_ledmap[x]);
      _is31->setLEDPWM(ridx, r);
      _is31->setLEDPWM(gidx, g);
      _is31->setLEDPWM(bidx, b);
    }

    /*
    Serial.print("("); Serial.print(x);
    Serial.print(", "); Serial.print(y);
    Serial.print(") -> [");
    Serial.print(ridx); Serial.print(", ");
    Serial.print(gidx); Serial.print(", ");
    Serial.print(bidx); Serial.println("]");
    */
  }
}

/**************************************************************************/
/*!
    @brief  Constructor for glasses LED ring. Not invoked by user code;
            use one of the subsequent subclasses for that.
    @param  controller  Pointer to Adafruit_IS31FL3741 object.
    @param  isRight     true if right ring, false if left.
    @note   Delcared here rather than in .h because it references static
            tables in this code.
*/
/**************************************************************************/
Adafruit_IS31FL3741_GlassesRing::Adafruit_IS31FL3741_GlassesRing(
    Adafruit_IS31FL3741 *controller, bool isRight)
    : _is31(controller), ring_map(isRight ? right_ring_map : left_ring_map) {}

/**************************************************************************/
/*!
    @brief  Set color of one pixel of one glasses ring.
    @param  n      Index of pixel to set (0-23).
    @param  color  RGB888 (24-bit) color, a la NeoPixel.
*/
/**************************************************************************/
void Adafruit_IS31FL3741_GlassesRing::setPixelColor(int16_t n, uint32_t color) {
  if ((n >= 0) && (n < 24)) {
    uint8_t r, g, b;
    r = (((uint16_t)((color >> 16) & 0xFF)) * _brightness) >> 8;
    g = (((uint16_t)((color >> 8) & 0xFF)) * _brightness) >> 8;
    b = (((uint16_t)(color & 0xFF)) * _brightness) >> 8;
    n *= 3;
    _is31->setLEDPWM(pgm_read_word(&ring_map[n]), r);
    _is31->setLEDPWM(pgm_read_word(&ring_map[n + 1]), g);
    _is31->setLEDPWM(pgm_read_word(&ring_map[n + 2]), b);
  }
}

/**************************************************************************/
/*!
    @brief Fill all pixels of one glasses ring to same color.
    @param  color  RGB888 (24-bit) color, a la NeoPixel.
*/
/**************************************************************************/
void Adafruit_IS31FL3741_GlassesRing::fill(uint32_t color) {
  uint8_t r, g, b;
  r = (((uint16_t)((color >> 16) & 0xFF)) * _brightness) >> 8;
  g = (((uint16_t)((color >> 8) & 0xFF)) * _brightness) >> 8;
  b = (((uint16_t)(color & 0xFF)) * _brightness) >> 8;

  for (uint8_t n = 0; n < 24 * 3; n += 3) {
    _is31->setLEDPWM(pgm_read_word(&ring_map[n]), r);
    _is31->setLEDPWM(pgm_read_word(&ring_map[n + 1]), g);
    _is31->setLEDPWM(pgm_read_word(&ring_map[n + 2]), b);
  }
}

// ORIGINAL LED GLASSES API (BUFFERED) -------------------------------------
// Again, these classes & functions are deprecated in favor of EyeLights.

/**************************************************************************/
/*!
    @brief  Constructor for buffered LED glasses (matrix portion, 18x5 LEDs)
    @param  controller  Pointer to Adafruit_IS31FL3741_buffered object.
    @param  withCanvas  If true, allocate an additional GFXcanvas16 object
                        that's 3X the size of the LED matrix -- using the
                        scale() function, anything drawn to the canvas can
                        be smoothly downsampled to the matrix resolution.
                        Good for scrolling things.
*/
/**************************************************************************/
Adafruit_IS31FL3741_GlassesMatrix_buffered::
    Adafruit_IS31FL3741_GlassesMatrix_buffered(
        Adafruit_IS31FL3741_buffered *controller, bool withCanvas)
    : Adafruit_GFX(18, 5), _is31(controller) {
  if (withCanvas) {
    canvas = new GFXcanvas16(18 * 3, 5 * 3); // 3X size canvas
  }
}

/**************************************************************************/
/*!
    @brief  Adafruit GFX low level accessor for buffered glasses matrix -
            sets an RGB pixel value, handles rotation and pixel arrangement.
            No immediate effect on LEDs; must follow up with show().
    @param  x      X position, starting with 0 for left-most side
    @param  y      Y position, starting with 0 for top-most side
    @param  color  16-bit RGB565 packed color (expands to 888 for LEDs).
*/
/**************************************************************************/
void Adafruit_IS31FL3741_GlassesMatrix_buffered::drawPixel(int16_t x, int16_t y,
                                                           uint16_t color) {
  if ((x >= 0) && (x < width()) && (y >= 0) && (y < height())) {
    _IS31_ROTATE_(x, y); // Handle GFX-style soft rotation
    x = (x * 5 + y) * 3; // Base index into ledmap
    uint16_t bidx = pgm_read_word(&glassesmatrix_ledmap[x]);
    if (bidx != 65535) { // Tables are BGR order
      uint16_t gidx = pgm_read_word(&glassesmatrix_ledmap[x + 1]);
      uint16_t ridx = pgm_read_word(&glassesmatrix_ledmap[x + 2]);
      uint8_t *ledbuf = _is31->getBuffer();
      _IS31_EXPAND_(color, r, g, b); // Expand GFX's RGB565 color to RGB888
      ledbuf[ridx] = r;
      ledbuf[gidx] = g;
      ledbuf[bidx] = b;
    }
  }
}

/**************************************************************************/
/*!
    @brief  Scales associated canvas (if one was requested via constructor)
            1:3 with antialiasing & gamma correction. No immediate effect
            on LEDs; must follow up with show(). Note that this overwrites
            ALL pixels within the matrix area, including those shared with
            the rings. This is different than using normal drawing
            operations directly to the low-resolution matrix, where these
            ops are "transparent" and empty pixels don't overwrite the
            rings.
*/
/**************************************************************************/
void Adafruit_IS31FL3741_GlassesMatrix_buffered::scale(void) {
  if (canvas) {
    uint16_t *src = canvas->getBuffer();
    uint8_t *ledbuf = _is31->getBuffer();
    // Outer x/y loops are column-major on purpose (less pointer math)
    for (int x = 0; x < 18; x++) {
      uint16_t *ptr = &src[x * 3]; // Entry along top scan line w/x offset
      for (int y = 0; y < 5; y++) {
        uint16_t rsum = 0, gsum = 0, bsum = 0;
        // Inner x/y loops are row-major on purpose (less pointer math)
        for (uint8_t yy = 0; yy < 3; yy++) {
          for (uint8_t xx = 0; xx < 3; xx++) {
            uint16_t rgb = ptr[xx];
            rsum += rgb >> 11;         // Accumulate 5 bits red,
            gsum += (rgb >> 5) & 0x3F; // 6 bits green,
            bsum += rgb & 0x1F;        // 5 bits blue
          }
          ptr += canvas->width(); // Advance one scan line
        }
        uint16_t base = (x * 5 + y) * 3; // Offset into ledmap
        uint16_t bidx = pgm_read_word(&glassesmatrix_ledmap[base]);
        if (bidx != 65535) { // Tables are BGR order
          uint16_t gidx = pgm_read_word(&glassesmatrix_ledmap[base + 1]);
          uint16_t ridx = pgm_read_word(&glassesmatrix_ledmap[base + 2]);
          ledbuf[ridx] = pgm_read_byte(&gammaRB[rsum]);
          ledbuf[gidx] = pgm_read_byte(&gammaG[gsum]);
          ledbuf[bidx] = pgm_read_byte(&gammaRB[bsum]);
        }
      }
    }
  }
}

/**************************************************************************/
/*!
    @brief  Constructor for buffered glasses LED ring. Not invoked by user
            code; use one of the subsequent subclasses for that.
    @param  controller  Pointer to Adafruit_IS31FL3741 object.
    @param  isRight     true if right ring, false if left.
*/
/**************************************************************************/
Adafruit_IS31FL3741_GlassesRing_buffered::
    Adafruit_IS31FL3741_GlassesRing_buffered(
        Adafruit_IS31FL3741_buffered *controller, bool isRight)
    : _is31(controller), ring_map(isRight ? right_ring_map : left_ring_map) {}

/**************************************************************************/
/*!
    @brief  Set color of one pixel of one buffered glasses ring.
            No immediate effect on LEDs; must follow up with show().
    @param  n      Index of pixel to set (0-23).
    @param  color  RGB888 (24-bit) color, a la NeoPixel.
*/
/**************************************************************************/
void Adafruit_IS31FL3741_GlassesRing_buffered::setPixelColor(int16_t n,
                                                             uint32_t color) {
  if ((n >= 0) && (n < 24)) {
    uint8_t *ledbuf = _is31->getBuffer();
    uint8_t r = (((uint16_t)((color >> 16) & 0xFF)) * _brightness) >> 8;
    uint8_t g = (((uint16_t)((color >> 8) & 0xFF)) * _brightness) >> 8;
    uint8_t b = (((uint16_t)(color & 0xFF)) * _brightness) >> 8;
    n *= 3;
    ledbuf[pgm_read_word(&ring_map[n + 2])] = r; // Tables are BGR order
    ledbuf[pgm_read_word(&ring_map[n + 1])] = g;
    ledbuf[pgm_read_word(&ring_map[n])] = b;
  }
}

/**************************************************************************/
/*!
    @brief  Fill all pixels of one glasses ring to same color.
            No immediate effect on LEDs; must follow up with show().
    @param  color  RGB888 (24-bit) color, a la NeoPixel.
*/
/**************************************************************************/
void Adafruit_IS31FL3741_GlassesRing_buffered::fill(uint32_t color) {
  uint8_t *ledbuf = _is31->getBuffer();
  uint8_t r = (((uint16_t)((color >> 16) & 0xFF)) * _brightness) >> 8;
  uint8_t g = (((uint16_t)((color >> 8) & 0xFF)) * _brightness) >> 8;
  uint8_t b = (((uint16_t)(color & 0xFF)) * _brightness) >> 8;
  for (uint8_t n = 0; n < 24 * 3; n += 3) {
    ledbuf[pgm_read_word(&ring_map[n + 2])] = r; // Tables are BGR order
    ledbuf[pgm_read_word(&ring_map[n + 1])] = g;
    ledbuf[pgm_read_word(&ring_map[n])] = b;
  }
}
