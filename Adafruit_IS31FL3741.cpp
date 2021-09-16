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
  Adafruit_GFX(18, 5)
  {
    _is31 = controller;
  }

static const uint16_t glassesmatrix_ledmap[18 * 5 * 3] PROGMEM = {
    65535, 65535, 65535, // (0,0) (clipped, corner)
    10,    9,     8,     // (0,0) / right ring pixel 20
    13,    12,    11,    // (0,2) / 19
    16,    15,    14,    // (0,3) / 18
    4,     3,     2,     // (0,4) / 17
    217,   216,   215,   // (1,0) / right ring pixel #21
    220,   219,   218,   // (1,1)
    223,   222,   221,   // (1,2)
    226,   225,   224,   // (1,3)
    214,   213,   212,   // (1,4)
    187,   186,   185,   // (2,0)
    190,   189,   188,   // (2,1)
    193,   192,   191,   // (2,2)
    196,   195,   194,   // (2,3)
    184,   183,   182,   // (2,4)
    37,    36,    35,    // (3,0)
    40,    39,    38,    // (3,1)
    43,    42,    41,    // (3,2)
    46,    45,    44,    // (3,3)
    34,    33,    32,    // (3,4)
    67,    66,    65,    // (4,0)
    70,    69,    68,    // (4,1)
    73,    72,    71,    // (4,2)
    76,    75,    74,    // (4,3)
    64,    63,    62,    // (4,4)
    97,    96,    95,    // (5,0)
    100,   99,    98,    // (5,1)
    103,   102,   101,   // (5,2)
    106,   105,   104,   // (5,3)
    94,    93,    92,    // (5,4)
    127,   126,   125,   // (6,0) / right ring pixel 3
    130,   129,   128,   // (6,1)
    133,   132,   131,   // (6,2)
    136,   135,   134,   // (6,3)
    124,   123,   122,   // (6,4)
    157,   156,   155,   // (7,0)
    160,   159,   158,   // (7,1)
    163,   162,   161,   // (7,2) / right ring pixel 5
    166,   165,   164,   // (7,3) / 6
    244,   243,   242,   // (7,4) / 7
    247,   246,   245,   // (8,0)
    250,   249,   248,   // (8,1)
    253,   252,   251,   // (8,2)
    256,   255,   254,   // (8,3)
    65535, 65535, 65535, // (8,4) (clipped, nose bridge)
    345,   346,   347,   // (9,0)
    342,   343,   344,   // (9,1)
    267,   268,   269,   // (9,2)
    263,   264,   265,   // (9,3)
    65535, 65535, 65535, // (9,4) (clipped, nose bridge)
    336,   337,   338,   // (10,0)
    333,   334,   335,   // (10,1)
    237,   238,   239,   // (10,2) / left ring pixel 19
    233,   234,   235,   // (10,3) / 18
    348,   349,   262,   // (10,4) / 17
    327,   328,   329,   // (11,0) / left ring pixel 21
    324,   325,   326,   // (11,1)
    207,   208,   209,   // (11,2)
    203,   204,   205,   // (11,3)
    330,   331,   202,   // (11,4)
    318,   319,   320,   // (12,0)
    315,   316,   317,   // (12,1)
    177,   178,   179,   // (12,2)
    173,   174,   175,   // (12,3)
    321,   322,   172,   // (12,4)
    309,   310,   311,   // (13,0)
    306,   307,   308,   // (13,1)
    147,   148,   149,   // (13,2)
    143,   144,   145,   // (13,3)
    312,   313,   142,   // (13,4)
    300,   301,   302,   // (14,0)
    297,   298,   299,   // (14,1)
    117,   118,   119,   // (14,2)
    113,   114,   115,   // (14,3)
    303,   304,   112,   // (14,4)
    291,   292,   293,   // (15,0)
    288,   289,   290,   // (15,1)
    87,    88,    89,    // (15,2)
    83,    84,    85,    // (15,3)
    294,   295,   82,    // (15,4)
    282,   283,   284,   // (16,0) / left ring pixel 3
    279,   280,   281,   // (16,1)
    57,    58,    59,    // (16,2)
    53,    54,    55,    // (16,3)
    285,   286,   52,    // (16,4)
    65535, 65535, 65535, // (17,0) (clipped, corner)
    270,   271,   272,   // (17,1) / left ring pixel 4
    27,    28,    29,    // (17,2) / 5
    23,    24,    25,    // (17,3) / 6
    276,   277,   22,    // (17,4) / 7
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

  x = (x * 5 + y) * 3;
  uint16_t ridx = pgm_read_word(&glassesmatrix_ledmap[x]);
  uint16_t gidx = pgm_read_word(&glassesmatrix_ledmap[x + 1]);
  uint16_t bidx = pgm_read_word(&glassesmatrix_ledmap[x + 2]);

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

static const uint16_t left_ring_map[24* 3] PROGMEM = {
    341, 211, 210, // 0
    332, 181, 180, // 1
    323, 151, 150, // 2
    127, 126, 125, // 3
    154, 153, 152, // 4
    163, 162, 161, // 5
    166, 165, 164, // 6
    244, 243, 242, // 7
    259, 258, 257, // 8
    169, 168, 167, // 9
    139, 138, 137, // 10
    109, 108, 107, // 11
    79, 78, 77, // 12
    49, 48, 47, // 13
    199, 198, 197, // 14
    229, 228, 227, // 15
    19, 18, 17, // 16
    4, 3, 2, // 17
    16, 15, 14, // 18
    13, 12, 11, // 19
    10, 9, 8, // 20
    217, 216, 215, // 21
    7, 6, 5, // 22
    350, 241, 240, // 23
  };

static const uint16_t right_ring_map[24 * 3] PROGMEM = {
    287, 31, 30, // 0
    278, 1, 0, // 1
    273, 274, 275, // 2
    282, 283, 284, // 3
    270, 271, 272, // 4
    27, 28, 29, // 5
    23, 24, 25, // 6
    276, 277, 22, // 7
    20, 21, 26, // 8
    50, 51, 56, // 9
    80, 81, 86, // 10
    110, 111, 116, // 11
    140, 141, 146, // 12
    170, 171, 176, // 13
    200, 201, 206, // 14
    230, 231, 236, // 15
    260, 261, 266, // 16
    348, 349, 262, // 17
    233, 234, 235, // 18
    237, 238, 239, // 19
    339, 340, 232, // 20
    327, 328, 329, // 21
    305, 91, 90, // 22
    296, 61, 60, // 23
};

Adafruit_IS31FL3741_GlassesRing::Adafruit_IS31FL3741_GlassesRing(
    Adafruit_IS31FL3741 *controller, bool isRight) {
  _is31 = controller;
  ring_map = isRight ? right_ring_map : left_ring_map;
}

void Adafruit_IS31FL3741_GlassesRing::fill(uint32_t color) {
  uint8_t r, g, b;
  r = (((uint16_t) ((color >> 16) & 0xFF)) * _brightness) >> 8;
  g = (((uint16_t) ((color >> 8) & 0xFF)) * _brightness) >> 8;
  b = (((uint16_t) (color & 0xFF)) * _brightness) >> 8;
  
  for (uint8_t n=0; n<24 * 3; n += 3) {
    _is31->setLEDPWM(pgm_read_word(&right_ring_map[n]), b);
    _is31->setLEDPWM(pgm_read_word(&right_ring_map[n + 1]), r);
    _is31->setLEDPWM(pgm_read_word(&right_ring_map[n + 2]), g);
  }
}

void Adafruit_IS31FL3741_GlassesRing::setPixelColor(int16_t n, uint32_t color) {
  if ((n >= 0) && (n < 24)) {
    uint8_t r, g, b;
    r = (((uint16_t) ((color >> 16) & 0xFF)) * _brightness) >> 8;
    g = (((uint16_t) ((color >> 8) & 0xFF)) * _brightness) >> 8;
    b = (((uint16_t) (color & 0xFF)) * _brightness) >> 8;
    n *= 3;
    _is31->setLEDPWM(pgm_read_word(&right_ring_map[n]), b);
    _is31->setLEDPWM(pgm_read_word(&right_ring_map[n + 1]), r);
    _is31->setLEDPWM(pgm_read_word(&right_ring_map[n + 2]), g);
  }
}

Adafruit_IS31FL3741_GlassesRightRing::Adafruit_IS31FL3741_GlassesRightRing(Adafruit_IS31FL3741 *controller) : Adafruit_IS31FL3741_GlassesRing(controller, true) {
}

Adafruit_IS31FL3741_GlassesLeftRing::Adafruit_IS31FL3741_GlassesLeftRing(Adafruit_IS31FL3741 *controller) : Adafruit_IS31FL3741_GlassesRing(controller, false) {
}


// -------------------------------------------------------------------------

Adafruit_IS31FL3741_buffered::Adafruit_IS31FL3741_buffered(uint8_t width,
                                                           uint8_t height)
    : Adafruit_IS31FL3741(width, height) {
}

Adafruit_IS31FL3741_buffered::~Adafruit_IS31FL3741_buffered(void) {
}

bool Adafruit_IS31FL3741_buffered::begin(uint8_t addr, TwoWire *theWire) {
  bool status = Adafruit_IS31FL3741::begin(addr, theWire);
  if (status) {                        // If I2C initialized OK,
    memset(ledbuf, 0, sizeof(ledbuf)); // clear the LED buffer
  }
  return status;
}

void Adafruit_IS31FL3741_buffered::drawPixel(int16_t x, int16_t y,
                                             uint16_t color) {
}

void Adafruit_IS31FL3741_buffered::show(void) {
  uint16_t total_bytes = 351;
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

Adafruit_IS31FL3741_buffered_GlassesRing::Adafruit_IS31FL3741_buffered_GlassesRing(Adafruit_IS31FL3741_buffered *controller, bool isRight) : _is31(controller) {
  ring_map = isRight ? right_ring_map : left_ring_map;
}

void Adafruit_IS31FL3741_buffered_GlassesRing::setPixelColor(int16_t n, uint32_t color) {
  if ((n >= 0) && (n < 24)) {
    uint8_t *ledbuf = &_is31->ledbuf[1];
    uint8_t r = (((uint16_t) ((color >> 16) & 0xFF)) * _brightness) >> 8;
    uint8_t g = (((uint16_t) ((color >> 8) & 0xFF)) * _brightness) >> 8;
    uint8_t b = (((uint16_t) (color & 0xFF)) * _brightness) >> 8;
    n *= 3;
    ledbuf[pgm_read_word(&right_ring_map[n])] =  b;
    ledbuf[pgm_read_word(&right_ring_map[n + 1])] = r;
    ledbuf[pgm_read_word(&right_ring_map[n + 2])] = g;
  }
}

void Adafruit_IS31FL3741_buffered_GlassesRing::fill(uint32_t color) {
  uint8_t *ledbuf = &_is31->ledbuf[1];
  uint8_t r = (((uint16_t) ((color >> 16) & 0xFF)) * _brightness) >> 8;
  uint8_t g = (((uint16_t) ((color >> 8) & 0xFF)) * _brightness) >> 8;
  uint8_t b = (((uint16_t) (color & 0xFF)) * _brightness) >> 8;
  
  for (uint8_t n=0; n<24 * 3; n += 3) {
    ledbuf[pgm_read_word(&right_ring_map[n])] =  b;
    ledbuf[pgm_read_word(&right_ring_map[n + 1])] = r;
    ledbuf[pgm_read_word(&right_ring_map[n + 2])] = g;
  }
}

Adafruit_IS31FL3741_buffered_GlassesLeftRing::Adafruit_IS31FL3741_buffered_GlassesLeftRing(Adafruit_IS31FL3741_buffered *controller) : Adafruit_IS31FL3741_buffered_GlassesRing(controller, false) {
}

Adafruit_IS31FL3741_buffered_GlassesRightRing::Adafruit_IS31FL3741_buffered_GlassesRightRing(Adafruit_IS31FL3741_buffered *controller) : Adafruit_IS31FL3741_buffered_GlassesRing(controller, true) {
}

Adafruit_IS31FL3741_buffered_GlassesMatrix::Adafruit_IS31FL3741_buffered_GlassesMatrix(Adafruit_IS31FL3741_buffered *controller) : Adafruit_GFX(18, 5), _is31(controller) {
}

void Adafruit_IS31FL3741_buffered_GlassesMatrix::drawPixel(int16_t x, int16_t y,
                                                       uint16_t color) {
  switch (getRotation()) {
  case 1:
    _swap_int16_t(x, y);
    x = WIDTH - 1 - x;
    break;
  case 2:
    x = WIDTH - 1 - x;
    y = HEIGHT - 1 - y;
    break;
  case 3:
    _swap_int16_t(x, y);
    y = HEIGHT - 1 - y;
    break;
  }

  if ((x >= 0) && (x < WIDTH) && (y >= 0) && (y < HEIGHT)) {
    x = (x * 5 + y) * 3; // Base index into ledmap
    uint16_t bidx = pgm_read_word(&glassesmatrix_ledmap[x]);
    if (bidx != 65535) {
      uint16_t ridx = pgm_read_word(&glassesmatrix_ledmap[x + 1]);
      uint16_t gidx = pgm_read_word(&glassesmatrix_ledmap[x + 2]);
      _is31->ledbuf[bidx + 1] =
          (color << 3) | ((color >> 2) & 0x07);          // 5->8 bits B
      _is31->ledbuf[ridx + 1] =
          ((color >> 8) & 0xF8) | (color >> 13);         // 5->8 bits R
      _is31->ledbuf[gidx + 1] =
          ((color >> 3) & 0xFC) | ((color >> 9) & 0x03); // 6->8 bits G
    }
  }
}

Adafruit_IS31FL3741_buffered_GlassesMatrix_smooth::Adafruit_IS31FL3741_buffered_GlassesMatrix_smooth(Adafruit_IS31FL3741_buffered *controller)
    : Adafruit_IS31FL3741_buffered_GlassesMatrix(controller) {
  canvas = new GFXcanvas16(18 * 3, 5 * 3); // 3X size canvas
  if (canvas == NULL) {
  }
}

void Adafruit_IS31FL3741_buffered_GlassesMatrix_smooth::drawPixel(
    int16_t x, int16_t y, uint16_t color) {
  canvas->drawPixel(x, y, color);
}

// GFXcanvas16 is RGB565 color while the LEDs are RGB888, so during 1:3
// downsampling we recover some intermediate shades and apply gamma
// correction for better linearity. Tables are used to avoid floating-point
// math. They appear large here but reside in flash, not too bad.
// To regenerate tables (e.g. different gamma), in Python:
// print(str([int((x / (31*9-1)) ** 2.2 * 255 + 0.5) for x in range(31*9)]))
// Then edit braces and clang-format result. For green, use 63 instead of 31.
// Red & blue table is 279 bytes, green table is 567 bytes. 846 total.
static const uint8_t gammaRB[] PROGMEM = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,
    2,   2,   2,   2,   3,   3,   3,   3,   3,   3,   4,   4,   4,   4,   4,
    5,   5,   5,   5,   6,   6,   6,   6,   7,   7,   7,   8,   8,   8,   8,
    9,   9,   9,   10,  10,  10,  11,  11,  12,  12,  12,  13,  13,  13,  14,
    14,  15,  15,  16,  16,  16,  17,  17,  18,  18,  19,  19,  20,  20,  21,
    21,  22,  22,  23,  23,  24,  25,  25,  26,  26,  27,  27,  28,  29,  29,
    30,  31,  31,  32,  33,  33,  34,  35,  35,  36,  37,  37,  38,  39,  39,
    40,  41,  42,  42,  43,  44,  45,  45,  46,  47,  48,  49,  50,  50,  51,
    52,  53,  54,  55,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,
    66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,
    81,  82,  83,  84,  85,  86,  88,  89,  90,  91,  92,  93,  94,  96,  97,
    98,  99,  100, 102, 103, 104, 105, 107, 108, 109, 110, 112, 113, 114, 116,
    117, 118, 120, 121, 122, 124, 125, 126, 128, 129, 130, 132, 133, 135, 136,
    138, 139, 140, 142, 143, 145, 146, 148, 149, 151, 152, 154, 155, 157, 159,
    160, 162, 163, 165, 166, 168, 170, 171, 173, 175, 176, 178, 180, 181, 183,
    185, 186, 188, 190, 191, 193, 195, 197, 198, 200, 202, 204, 205, 207, 209,
    211, 213, 215, 216, 218, 220, 222, 224, 226, 228, 229, 231, 233, 235, 237,
    239, 241, 243, 245, 247, 249, 251, 253, 255};
static const uint8_t gammaG[] PROGMEM = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,
    2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   3,   3,   3,   3,   3,
    3,   3,   3,   3,   3,   3,   4,   4,   4,   4,   4,   4,   4,   4,   4,
    4,   5,   5,   5,   5,   5,   5,   5,   5,   6,   6,   6,   6,   6,   6,
    6,   6,   7,   7,   7,   7,   7,   7,   7,   8,   8,   8,   8,   8,   8,
    8,   9,   9,   9,   9,   9,   9,   10,  10,  10,  10,  10,  10,  11,  11,
    11,  11,  11,  11,  12,  12,  12,  12,  12,  13,  13,  13,  13,  13,  14,
    14,  14,  14,  14,  15,  15,  15,  15,  15,  16,  16,  16,  16,  16,  17,
    17,  17,  17,  18,  18,  18,  18,  19,  19,  19,  19,  20,  20,  20,  20,
    21,  21,  21,  21,  22,  22,  22,  22,  23,  23,  23,  23,  24,  24,  24,
    24,  25,  25,  25,  26,  26,  26,  26,  27,  27,  27,  28,  28,  28,  28,
    29,  29,  29,  30,  30,  30,  31,  31,  31,  32,  32,  32,  33,  33,  33,
    34,  34,  34,  34,  35,  35,  36,  36,  36,  37,  37,  37,  38,  38,  38,
    39,  39,  39,  40,  40,  40,  41,  41,  42,  42,  42,  43,  43,  43,  44,
    44,  45,  45,  45,  46,  46,  46,  47,  47,  48,  48,  48,  49,  49,  50,
    50,  50,  51,  51,  52,  52,  53,  53,  53,  54,  54,  55,  55,  55,  56,
    56,  57,  57,  58,  58,  59,  59,  59,  60,  60,  61,  61,  62,  62,  63,
    63,  64,  64,  64,  65,  65,  66,  66,  67,  67,  68,  68,  69,  69,  70,
    70,  71,  71,  72,  72,  73,  73,  74,  74,  75,  75,  76,  76,  77,  77,
    78,  78,  79,  79,  80,  80,  81,  81,  82,  83,  83,  84,  84,  85,  85,
    86,  86,  87,  87,  88,  89,  89,  90,  90,  91,  91,  92,  93,  93,  94,
    94,  95,  95,  96,  97,  97,  98,  98,  99,  99,  100, 101, 101, 102, 102,
    103, 104, 104, 105, 106, 106, 107, 107, 108, 109, 109, 110, 110, 111, 112,
    112, 113, 114, 114, 115, 116, 116, 117, 118, 118, 119, 119, 120, 121, 121,
    122, 123, 123, 124, 125, 125, 126, 127, 127, 128, 129, 130, 130, 131, 132,
    132, 133, 134, 134, 135, 136, 136, 137, 138, 139, 139, 140, 141, 141, 142,
    143, 144, 144, 145, 146, 147, 147, 148, 149, 149, 150, 151, 152, 152, 153,
    154, 155, 155, 156, 157, 158, 159, 159, 160, 161, 162, 162, 163, 164, 165,
    165, 166, 167, 168, 169, 169, 170, 171, 172, 173, 173, 174, 175, 176, 177,
    177, 178, 179, 180, 181, 182, 182, 183, 184, 185, 186, 187, 187, 188, 189,
    190, 191, 192, 192, 193, 194, 195, 196, 197, 198, 198, 199, 200, 201, 202,
    203, 204, 205, 205, 206, 207, 208, 209, 210, 211, 212, 213, 213, 214, 215,
    216, 217, 218, 219, 220, 221, 222, 223, 223, 224, 225, 226, 227, 228, 229,
    230, 231, 232, 233, 234, 235, 236, 237, 237, 238, 239, 240, 241, 242, 243,
    244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255};

void Adafruit_IS31FL3741_buffered_GlassesMatrix_smooth::scale(void) {
  uint16_t *src = canvas->getBuffer();
  uint8_t *ledbuf = &_is31->ledbuf[1];
  // Outer x/y loops are column-major on purpose (less pointer math)
  for (int x = 0; x < 18; x++) {
    uint16_t *ptr = &src[x * 3]; // Entry along top scan line w/x offset
    for (int y = 0; y < 5; y++) {
      uint32_t rsum = 0;
      uint16_t gsum = 0, bsum = 0;
      // Inner x/y loops are row-major on purpose (less pointer math)
      for (uint8_t yy = 0; yy < 3; yy++) {
        for (uint8_t xx = 0; xx < 3; xx++) {
          uint16_t rgb = ptr[xx];
          rsum += (rgb >> 11) & 0x1F; // Accumulate 5 bits red,
          gsum += (rgb >> 5) & 0x3F;  // 6 bits green,
          bsum += rgb & 0x1F;         // 5 bits blue
        }
        ptr += canvas->width(); // Advance one scan line
      }
      uint16_t base = (x * 5 + y) * 3; // Offset into ledmap
      uint16_t bidx = pgm_read_word(&glassesmatrix_ledmap[base]);
      if (bidx != 65535) {
        uint16_t ridx = pgm_read_word(&glassesmatrix_ledmap[base + 1]);
        uint16_t gidx = pgm_read_word(&glassesmatrix_ledmap[base + 2]);
        ledbuf[bidx] = pgm_read_byte(&gammaRB[bsum]);
        ledbuf[ridx] = pgm_read_byte(&gammaRB[rsum]);
        ledbuf[gidx] = pgm_read_byte(&gammaG[gsum]);
      }
    }
  }
}
