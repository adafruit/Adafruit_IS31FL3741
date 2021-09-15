#include <Adafruit_IS31FL3741.h>

#ifndef _swap_int16_t
#define _swap_int16_t(a, b)                                                    \
  {                                                                            \
    int16_t t = a;                                                             \
    a = b;                                                                     \
    b = t;                                                                     \
  }
#endif

/**************************************************************************/
/*!
    @brief Constructor for breakout version
    @param width Desired width of led display
    @param height Desired height of led display
*/
/**************************************************************************/

Adafruit_IS31FL3741::Adafruit_IS31FL3741(uint8_t width, uint8_t height)
    : Adafruit_GFX(width, height) {}

/**************************************************************************/
/*!
    @brief Initialize hardware and clear display
    @param addr The I2C address we expect to find the chip at
    @param theWire The TwoWire I2C bus device to use, defaults to &Wire
    @returns True on success, false if chip isnt found
*/
/**************************************************************************/
bool Adafruit_IS31FL3741::begin(uint8_t addr, TwoWire *theWire) {
  if (_i2c_dev) {
    delete _i2c_dev;
  }
  _i2c_dev = new Adafruit_I2CDevice(addr, theWire);

  if (!_i2c_dev->begin()) {
    return false;
  }

  _i2c_dev->setSpeed(400000);

  Adafruit_BusIO_Register id_reg =
      Adafruit_BusIO_Register(_i2c_dev, IS3741_IDREGISTER);
  if (id_reg.read() != addr * 2) {
    return false;
  }

  if (!reset()) {
    return false;
  }
  setRotation(0);
  return true;
}

/**************************************************************************/
/*!
    @brief Perform a software reset, will update all registers to POR values
    @returns False if I2C command not acknowledged
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
    @brief Enables or disables via the shutdown register bit
    @param en Set to true to enable
    @returns False if I2C command not acknowledged
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
    @brief Set the global current-mirror from 0 (off) to 255 (brightest)
    @param current Scaler from 0 to 255
    @returns False if I2C command not acknowledged
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
    @brief Get the global current-mirror from 0 (off) to 255 (brightest)
    @returns Current scaler from 0 to 255
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
    @brief Allows changing of command register by writing 0xC5 to 0xFE
    @returns True if I2C command succeeded.
*/
/**************************************************************************/
bool Adafruit_IS31FL3741::unlock(void) {
  Adafruit_BusIO_Register lock_reg =
      Adafruit_BusIO_Register(_i2c_dev, IS3741_COMMANDREGISTERLOCK);
  return lock_reg.write(0xC5);
}

/**************************************************************************/
/*!
    @brief Switch to a given bank/page in the chip memory for future reads
    @param page The IS41 page to switch to
    @returns False if I2C command failed to be ack'd or invalid page
*/
/**************************************************************************/
bool Adafruit_IS31FL3741::selectPage(uint8_t page) {
  if (page > 4)
    return false;

  if (_page == page) {
    // nice, we can skip re-setting the page!
    return true;
  }

  _page = page; // cache this value

  unlock();
  Adafruit_BusIO_Register cmd_reg =
      Adafruit_BusIO_Register(_i2c_dev, IS3741_COMMANDREGISTER);
  return cmd_reg.write(page);
}

/**************************************************************************/
/*!
    @brief Set the PWM scaling for a single LED
    @param lednum The individual LED to scale - from 0 to 351
    @param scale Scaler from 0 to 255
    @returns False if I2C command not acknowledged
*/
/**************************************************************************/
bool Adafruit_IS31FL3741::setLEDscaling(uint16_t lednum, uint8_t scale) {
  uint8_t cmd[2] = {(uint8_t)lednum, scale}; // we'll fix the lednum later

  if (lednum < 180) {
    selectPage(2);
    return _i2c_dev->write(cmd, 2);
  } else if (lednum < 351) {
    selectPage(3);
    cmd[0] = lednum - 180; // fix it for higher numbers!
    return _i2c_dev->write(cmd, 2);
  }
  return false; // failed
}

/**************************************************************************/
/*!
    @brief Optimized function to set the PWM scaling for ALL the LEDs
    @param scale Scaler from 0 to 255
    @returns False if I2C command not acknowledged
*/
/**************************************************************************/
bool Adafruit_IS31FL3741::setLEDscaling(uint8_t scale) {
  uint16_t ledaddr = 0;

  // write 30 bytes of scale at a time
  uint8_t buff[31];
  memset(buff, scale, 31);

  selectPage(2);
  while (ledaddr < 180) {
    buff[0] = (uint8_t)ledaddr;
    if (!_i2c_dev->write(buff, 31))
      return false;

    ledaddr += 30;
  }

  selectPage(3);
  while (ledaddr < 351) {
    buff[0] = (uint8_t)(ledaddr - 180);
    if (!_i2c_dev->write(buff, 31))
      return false;
    ledaddr += 30;
  }
  return true;
}

/**************************************************************************/
/*!
    @brief Sets all LEDs PWM to the same value - great for clearing the whole
    display at once!
    @param fillpwm The PWM value to set all LEDs to, defaults to 0
    @returns False if I2C command not acknowledged
*/
/**************************************************************************/
bool Adafruit_IS31FL3741::fill(uint8_t fillpwm) {
  uint16_t ledaddr = 0;

  // write 30 bytes of scale at a time
  uint8_t buff[31];
  memset(buff, fillpwm, 31);

  selectPage(0);
  while (ledaddr < 180) {
    buff[0] = (uint8_t)ledaddr;
    if (!_i2c_dev->write(buff, 31))
      return false;

    ledaddr += 30;
  }

  selectPage(1);
  while (ledaddr < 351) {
    buff[0] = (uint8_t)(ledaddr - 180);
    if (!_i2c_dev->write(buff, 31))
      return false;
    ledaddr += 30;
  }
  return true;
}

/**************************************************************************/
/*!
    @brief Low level accesssor - sets a 8-bit PWM pixel value to the internal
    memory location. Does not handle rotation, x/y or any rearrangements!
    @param lednum The offset from 0 to 350 that corresponds to the LED
    @param pwm brightness, from 0 (off) to 255 (max on)
    @returns False if I2C command not acknowledged
*/
/**************************************************************************/
bool Adafruit_IS31FL3741::setLEDPWM(uint16_t lednum, uint8_t pwm) {
  uint8_t cmd[2] = {(uint8_t)lednum, pwm}; // we'll fix the lednum later

  // Serial.print("Setting led 0x"); Serial.print(lednum, HEX); Serial.print("
  // -> "); Serial.println(pwm);

  if (lednum < 180) {
    selectPage(0);
    return _i2c_dev->write(cmd, 2);
  } else if (lednum < 351) {
    selectPage(1);
    cmd[0] = lednum - 180; // fix it for higher numbers!
    return _i2c_dev->write(cmd, 2);
  }
  return false; // failed
}

/**************************************************************************/
/*!
    @brief Adafruit GFX low level accesssor - sets a 8-bit PWM pixel value
    handles rotation and pixel arrangement, unlike setLEDPWM
    @param x The x position, starting with 0 for left-most side
    @param y The y position, starting with 0 for top-most side
    @param color Despite being a 16-bit value, takes 0 (off) to 255 (max on)
*/
/**************************************************************************/
void Adafruit_IS31FL3741::drawPixel(int16_t x, int16_t y, uint16_t color) {
  // check rotation, move pixel around if necessary
  switch (getRotation()) {
  case 1:
    _swap_int16_t(x, y);
    x = WIDTH - x - 1;
    break;
  case 2:
    x = WIDTH - x - 1;
    y = HEIGHT - y - 1;
    break;
  case 3:
    _swap_int16_t(x, y);
    y = HEIGHT - y - 1;
    break;
  }

  if ((x < 0) || (x >= WIDTH))
    return;
  if ((y < 0) || (y >= HEIGHT))
    return;

  // extract RGB
  uint8_t r, g, b;
  r = (color >> 8) & 0xF8;
  r |= (r >> 5) & 0x07; // dup the top 3 bits to make 5 + 3 = 8 bits
  g = (color >> 3) & 0xFC;
  g |= (g >> 6) & 0x03; // dup the top 2 bits to make 6 + 2 = 8 bits
  b = (color << 3) & 0xFF;
  b |= (b >> 5) & 0x07; // dup the top 3 bits to make 5 + 3 = 8 bits
  uint16_t offset = (x + WIDTH * y) * 3;

  setLEDPWM(offset, b);
  setLEDPWM(offset + 1, g);
  setLEDPWM(offset + 2, r);

  return;
}

/**************************************************************************/
/*!
    @brief Converter for RGB888-format color to RGB565-format
    @param red 8-bit red color
    @param green 8-bit green color
    @param blue 8-bit blue color
    @returns Packed 16-bit RGB565 c
*/
/**************************************************************************/
uint16_t Adafruit_IS31FL3741::color565(uint8_t red, uint8_t green,
                                       uint8_t blue) {
  return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3);
}

/**************************************************************************/
/*!
    @brief Constructor for EVB version (9 x 13 LEDs)
*/
/**************************************************************************/
Adafruit_IS31FL3741_EVB::Adafruit_IS31FL3741_EVB(void)
    : Adafruit_IS31FL3741(13, 9) {}

/**************************************************************************/
/*!
    @brief Adafruit GFX low level accesssor - sets a 8-bit PWM pixel value
    handles rotation and pixel arrangement, unlike setLEDPWM
    @param x The x position, starting with 0 for left-most side
    @param y The y position, starting with 0 for top-most side
    @param color Despite being a 16-bit value, takes 0 (off) to 255 (max on)
*/
/**************************************************************************/
void Adafruit_IS31FL3741_EVB::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if ((x < 0) || (x >= width()))
    return;
  if ((y < 0) || (y >= height()))
    return;

  // check rotation, move pixel around if necessary

  switch (getRotation()) {
  case 1:
    _swap_int16_t(x, y);
    x = WIDTH - x - 1;
    break;
  case 2:
    x = WIDTH - x - 1;
    y = HEIGHT - y - 1;
    break;
  case 3:
    _swap_int16_t(x, y);
    y = HEIGHT - y - 1;
    break;
  }

  // extract RGB
  uint8_t r, g, b;
  r = (color >> 8) & 0xF8;
  r |= (r >> 5) & 0x07; // dup the top 3 bits to make 5 + 3 = 8 bits
  g = (color >> 3) & 0xFC;
  g |= (g >> 6) & 0x03; // dup the top 2 bits to make 6 + 2 = 8 bits
  b = (color << 3) & 0xFF;
  b |= (b >> 5) & 0x07; // dup the top 3 bits to make 5 + 3 = 8 bits

  uint16_t offset;
  if (x > 9) {
    offset = (x + 80 + y * 3) * 3;
  } else {
    offset = (x + y * 10) * 3;
  }

  /*
  Serial.print("("); Serial.print(x);
  Serial.print(", "); Serial.print(y);
  Serial.print(") -> "); Serial.println(offset);
  */

  setLEDPWM(offset, b);
  setLEDPWM(offset + 1, g);
  setLEDPWM(offset + 2, r);

  return;
}

///////////////

/**************************************************************************/
/*!
    @brief Constructor for QT version (13 x 9 LEDs)
*/
/**************************************************************************/
Adafruit_IS31FL3741_QT::Adafruit_IS31FL3741_QT(void)
    : Adafruit_IS31FL3741(13, 9) {}

/**************************************************************************/
/*!
    @brief Adafruit GFX low level accesssor - sets a 8-bit PWM pixel value
    handles rotation and pixel arrangement, unlike setLEDPWM
    @param x The x position, starting with 0 for left-most side
    @param y The y position, starting with 0 for top-most side
    @param color Despite being a 16-bit value, takes 0 (off) to 255 (max on)
*/
/**************************************************************************/
void Adafruit_IS31FL3741_QT::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if ((x < 0) || (x >= width()))
    return;
  if ((y < 0) || (y >= height()))
    return;

  // check rotation, move pixel around if necessary

  switch (getRotation()) {
  case 1:
    _swap_int16_t(x, y);
    x = WIDTH - x - 1;
    break;
  case 2:
    x = WIDTH - x - 1;
    y = HEIGHT - y - 1;
    break;
  case 3:
    _swap_int16_t(x, y);
    y = HEIGHT - y - 1;
    break;
  }

  // extract RGB
  uint8_t r, g, b;
  r = (color >> 8) & 0xF8;
  r |= (r >> 5) & 0x07; // dup the top 3 bits to make 5 + 3 = 8 bits
  g = (color >> 3) & 0xFC;
  g |= (g >> 6) & 0x03; // dup the top 2 bits to make 6 + 2 = 8 bits
  b = (color << 3) & 0xFF;
  b |= (b >> 5) & 0x07; // dup the top 3 bits to make 5 + 3 = 8 bits

  /*
  Serial.print("("); Serial.print(x);
  Serial.print(", "); Serial.print(y);
  Serial.print(") -> 0x");
  */

  uint8_t col = x;
  uint8_t row = y;

  // remap the row
  uint8_t rowmap[] = {8, 5, 4, 3, 2, 1, 0, 7, 6};
  row = rowmap[y];

  uint16_t offset = 0;

  if (row <= 5) {
    if (col < 10) {
      offset = 0x1E * row + col * 3;
    } else {
      offset = 0xB4 + 0x5A + 9 * row + (col - 10) * 3;
    }
  } else {
    if (col < 10) {
      offset = 0xB4 + (row - 6) * 0x1E + col * 3;
    } else {
      offset = 0xB4 + 0x5A + 9 * row + (col - 10) * 3;
    }
  }

  int8_t r_off = 0, g_off = 1, b_off = 2;
  if ((col == 12) || (col % 2 == 1)) { // odds + last col
    r_off = 2;
    g_off = 1;
    b_off = 0;
  } else { // evens;
    r_off = 0;
    g_off = 2;
    b_off = 1;
  }

  // Serial.println(offset, HEX);

  setLEDPWM(offset + r_off, r);
  setLEDPWM(offset + g_off, g);
  setLEDPWM(offset + b_off, b);

  return;
}


///////////////

/**************************************************************************/
/*!
    @brief Constructor for QT version (13 x 9 LEDs)
*/
/**************************************************************************/
Adafruit_IS31FL3741_GlassesMatrix::Adafruit_IS31FL3741_GlassesMatrix(Adafruit_IS31FL3741 *controller) :
  Adafruit_GFX(16, 5)
  {
    _is31 = controller;
  }

const PROGMEM uint16_t glassesmatrix_ledmap[16*5*3] = {
    // col 0
      217, 216, 215,
      220, 219, 218,
      223, 222, 221,
      226, 225, 224,
      214, 213, 212,
    
     // col 1
      187, 186, 185,
      190, 189, 188,
      193, 192, 191,
      196, 195, 194,
      184, 183, 182,

     // col 2
      37, 36, 35,
      40, 39, 38,
      43, 42, 41,
      46, 45, 44,
      34, 33, 32,

     // col 3
      67, 66, 65,
      70, 69, 68,
      73, 72, 71,
      76, 75, 74,
      64, 63, 62,

     // col 4
      97, 96, 95,
      100, 99, 98,
      103, 102, 101,
      106, 105, 104,
      94, 93, 92,

     // col 5
      127, 126, 125,
      130, 129, 128,
      133, 132, 131,
      136, 135, 134,
      124, 123, 122,

     // col 6
      157, 156, 155,
      160, 159, 158,
      163, 162, 161,
      166, 165, 164,
      244, 243, 242,

     // col 7
      247, 246, 245,
      250, 249, 248,
      253, 252, 251,
      256, 255, 254,
      65535, 65535, 65535, // not avail

     // col 8
      345, 346, 347,
      342, 343, 344,
      267, 268, 269,
      263, 264, 265,
      65535, 65535, 65535, // not avail

     // col 9
      336, 337, 338,
      333, 334, 335,
      237, 238, 239,
      233, 234, 235,
      348, 349, 262,

    // col 10
      327, 328, 329,
      324, 325, 326,
      207, 208, 209,
      203, 204, 205,
      330, 331, 202,

     // col 11
      318, 319, 320,
      315, 316, 317,
      177, 178, 179,
      173, 174, 175,
      321, 322, 172,

     // col 12
      309, 310, 311,
      306, 307, 308,
      147, 148, 149,
      143, 144, 145,
      312, 313, 142,

     // col 13
      300, 301, 302,
      297, 298, 299,
      117, 118, 119,
      113, 114, 115,
      303, 304, 112,

     // col 14
      291, 292, 293,
      288, 289, 290,
      87, 88, 89,
      83, 84, 85,
      294, 295, 82,

     // col 15
      282, 283, 284,
      279, 280, 281,
      57, 58, 59,
      53, 54, 55,
      285, 286, 52
};

/**************************************************************************/
/*!
    @brief Adafruit GFX low level accesssor - sets a 8-bit PWM pixel value
    handles rotation and pixel arrangement, unlike setLEDPWM
    @param x The x position, starting with 0 for left-most side
    @param y The y position, starting with 0 for top-most side
    @param color Despite being a 16-bit value, takes 0 (off) to 255 (max on)
*/
/**************************************************************************/
void Adafruit_IS31FL3741_GlassesMatrix::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if ((x < 0) || (x >= width()))
    return;
  if ((y < 0) || (y >= height()))
    return;

  // check rotation, move pixel around if necessary

  switch (getRotation()) {
  case 1:
    _swap_int16_t(x, y);
    x = WIDTH - x - 1;
    break;
  case 2:
    x = WIDTH - x - 1;
    y = HEIGHT - y - 1;
    break;
  case 3:
    _swap_int16_t(x, y);
    y = HEIGHT - y - 1;
    break;
  }

  // extract RGB
  uint8_t r, g, b;
  r = (color >> 8) & 0xF8;
  r |= (r >> 5) & 0x07; // dup the top 3 bits to make 5 + 3 = 8 bits
  g = (color >> 3) & 0xFC;
  g |= (g >> 6) & 0x03; // dup the top 2 bits to make 6 + 2 = 8 bits
  b = (color << 3) & 0xFF;
  b |= (b >> 5) & 0x07; // dup the top 3 bits to make 5 + 3 = 8 bits

  uint16_t ridx = pgm_read_word(glassesmatrix_ledmap + x*5*3 + y*3 + 0);
  uint16_t gidx = pgm_read_word(glassesmatrix_ledmap + x*5*3 + y*3 + 1);
  uint16_t bidx = pgm_read_word(glassesmatrix_ledmap + x*5*3 + y*3 + 2);

  /*
  Serial.print("("); Serial.print(x);
  Serial.print(", "); Serial.print(y);
  Serial.print(") -> [");
  Serial.print(ridx); Serial.print(", ");
  Serial.print(gidx); Serial.print(", ");
  Serial.print(bidx); Serial.println("]");
  */

  if (ridx != 65535) {
    _is31->setLEDPWM(ridx, b);
    _is31->setLEDPWM(gidx, r);
    _is31->setLEDPWM(bidx, g);
  }
  return;
}


Adafruit_IS31FL3741_GlassesRightRing::Adafruit_IS31FL3741_GlassesRightRing(Adafruit_IS31FL3741 *controller)
  {
    _is31 = controller;
  }


void Adafruit_IS31FL3741_GlassesRightRing::fill(uint32_t color) {
  // extract RGB
  uint8_t r, g, b;
  r = (color >> 16) & 0xFF;
  g = (color >> 8) & 0xFF;
  b = color & 0xFF;
  
  for (uint8_t n=0; n<24; n++) {
    _is31->setLEDPWM(ledmap[n][0], b);
    _is31->setLEDPWM(ledmap[n][1], r);
    _is31->setLEDPWM(ledmap[n][2], g);
  }

  return;
}

void Adafruit_IS31FL3741_GlassesRightRing::setPixelColor(int16_t n, uint32_t color) {
  if ((n < 0) || (n >= 24))
    return;

  // extract RGB
  uint8_t r, g, b;
  r = (((uint16_t) ((color >> 16) & 0xFF)) * (uint16_t)_brightness) >> 8;
  g = (((uint16_t) ((color >> 8) & 0xFF)) * (uint16_t)_brightness) >> 8;
  b = (((uint16_t) (color & 0xFF)) * (uint16_t)_brightness) >> 8;

  _is31->setLEDPWM(ledmap[n][0], b);
  _is31->setLEDPWM(ledmap[n][1], r);
  _is31->setLEDPWM(ledmap[n][2], g);

  return;
}

Adafruit_IS31FL3741_GlassesLeftRing::Adafruit_IS31FL3741_GlassesLeftRing(Adafruit_IS31FL3741 *controller)
  {
    _is31 = controller;
  }

void Adafruit_IS31FL3741_GlassesLeftRing::fill(uint32_t color) {
  // extract RGB
  uint8_t r, g, b;
  r = (color >> 16) & 0xFF;
  g = (color >> 8) & 0xFF;
  b = color & 0xFF;
  
  for (uint8_t n=0; n<24; n++) {
    _is31->setLEDPWM(ledmap[n][0], b);
    _is31->setLEDPWM(ledmap[n][1], r);
    _is31->setLEDPWM(ledmap[n][2], g);
  }

  return;
}

void Adafruit_IS31FL3741_GlassesLeftRing::setPixelColor(int16_t n, uint32_t color) {
  if ((n < 0) || (n >= 24))
    return;

  // extract RGB
  uint8_t r, g, b;
  r = (((uint16_t) ((color >> 16) & 0xFF)) * (uint16_t)_brightness) >> 8;
  g = (((uint16_t) ((color >> 8) & 0xFF)) * (uint16_t)_brightness) >> 8;
  b = (((uint16_t) (color & 0xFF)) * (uint16_t)_brightness) >> 8;

  _is31->setLEDPWM(ledmap[n][0], b);
  _is31->setLEDPWM(ledmap[n][1], r);
  _is31->setLEDPWM(ledmap[n][2], g);

  return;
}

// -------------------------------------------------------------------------

Adafruit_IS31FL3741_buffered::Adafruit_IS31FL3741_buffered(uint8_t width,
                                                           uint8_t height,
                                                           uint8_t *buf)
    : Adafruit_IS31FL3741(width, height), ledbuf(buf), ledbuf_passed_in((buf)) {
}

Adafruit_IS31FL3741_buffered::~Adafruit_IS31FL3741_buffered(void) {
  if (!ledbuf_passed_in && ledbuf) {
    delete ledbuf;
  }
}

bool Adafruit_IS31FL3741_buffered::begin(uint8_t addr, TwoWire *theWire) {
  // If a preallocated buffer was not passed to the constructor, do that
  // allocation here. In addition to the LED PWM data, there are two extra
  // bytes. First byte is a "bit bucket" destination for clipping (certain
  // (X,Y) coords can be table-mapped here so conditional branches aren't
  // needed). Second byte is the first Page 0 register address (0) so a
  // single I2C write() call can be used for reg + data. LED PWM data then
  // starts at index 2. Buffer is NOT cleared yet.
  if (!ledbuf_passed_in && !((ledbuf = new uint8_t[WIDTH * HEIGHT * 3 + 2]))) {
    return false; // alloc fail before even getting to the hard stuff
  }

  // LED buffer is valid. Initialize underlying I2C magic...
  bool status = Adafruit_IS31FL3741::begin(addr, theWire);

  if (status) {
    // Good I2C init, clear the LED buffer (whether passed in or alloc'd)
    memset(ledbuf, 0, WIDTH * HEIGHT * 3 + 2);
  } else if (!ledbuf_passed_in) {
    // If I2C init failed AND we allocated ledbuf above, delete it
    delete ledbuf;
  }

  return status;
}

// This doesn't actually make sense and I’ll probably make it an empty
// function (to meet GFX virtual func requirements), then have a separate
// thing (with LED index remapping) to actually draw on the glasses matrix.
void Adafruit_IS31FL3741_buffered::drawPixel(int16_t x, int16_t y,
                                             uint16_t color) {
  // check rotation, move pixel around if necessary
  switch (getRotation()) {
  case 1:
    _swap_int16_t(x, y);
    x = WIDTH - x - 1;
    break;
  case 2:
    x = WIDTH - x - 1;
    y = HEIGHT - y - 1;
    break;
  case 3:
    _swap_int16_t(x, y);
    y = HEIGHT - y - 1;
    break;
  }

  if ((x >= 0) && (x < WIDTH) && (y >= 0) && (y < HEIGHT)) {
    uint16_t offset = (y * WIDTH + x) * 3 + 2;
    ledbuf[offset] = ((color >> 8) & 0xF8) | (color >> 13);    // 5->8 bits R
    ledbuf[offset + 1] = ((color >> 3) & 0xFC) | (color >> 9); // 6->8 bits G
    ledbuf[offset + 2] = (color << 3) | (color >> 2);          // 5->8 bits B
  }
}

void Adafruit_IS31FL3741_buffered::show(void) {
  uint16_t total_bytes = WIDTH * HEIGHT * 3; // Amount of LED data
  uint8_t *ptr = &ledbuf[1];                 // Page 0 Reg 0 in ledbuf
  uint8_t chunk = _i2c_dev->maxBufferSize(); // I2C xfer limit

  // Page 0 is always written
  selectPage(0);
  uint8_t page_bytes = min(total_bytes, 180) + 1; // +1 for Page 0 Reg 0
  while (page_bytes) { // While there's data to write for page 0...
    uint8_t bytesThisPass = min(page_bytes, chunk);
    _i2c_dev->write(ptr, bytesThisPass);
    page_bytes -= bytesThisPass;
    ptr += bytesThisPass;
  }

  // Page 1 is written only if device has a generous complement of LEDs
  if (total_bytes > 180) {
    // The "write one big addr+data packet rather than separate reg address
    // and data packets" trick requires shenanigans for Page 1. With Page 0
    // we could maintain a permanently-stuffed reg addr byte just before
    // the LED data. But we DON'T want a similar byte mid-buffer because
    // that would add a conditional check on every single pixel-drawing
    // call. Instead, the byte just before the first Page 1 data is saved,
    // modified with the reg address (0), and restored later. This only
    // happens once per show() so it's not bad.
    uint8_t save = ledbuf[181]; // Remember the last Page 0 LED in ledbuf
    ledbuf[181] = 0;            // Set that spot to Page 1 Reg 0 plz
    ptr = &ledbuf[181];         // And start transfer there
    selectPage(1);
    page_bytes = total_bytes - 180 + 1; // +1 for Page 1 Reg 0
    while (page_bytes) {
      uint8_t bytesThisPass = min(page_bytes, chunk);
      _i2c_dev->write(ptr, bytesThisPass);
      page_bytes -= bytesThisPass;
      ptr += bytesThisPass;
    }
    ledbuf[181] = save; // Restore the saved pixel byte
  }
}
