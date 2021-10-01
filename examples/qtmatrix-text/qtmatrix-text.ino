// Scrolling text example for the Adafruit IS31FL3741 13x9 PWM RGB LED
// Matrix Driver w/STEMMA QT / Qwiic connector. This is the simplest
// version and should fit on small microcontrollers like Arduino Uno.
// Tradeoff is that animation isn't always as smooth as seen in the
// buffered example. Each LED changes state immediately when accessed,
// there is no show() or display() function as with NeoPixels or some
// OLED screens.

#include <Adafruit_IS31FL3741.h>

Adafruit_IS31FL3741_QT matrix;
// If colors appear wrong on matrix, try invoking constructor like so:
// Adafruit_IS31FL3741_QT matrix(IS3741_RBG);

// Some boards have just one I2C interface, but some have more...
TwoWire *i2c = &Wire; // e.g. change this to &Wire1 for QT Py RP2040

char text[] = "ADAFRUIT!";   // A message to scroll
int text_x = matrix.width(); // Initial text position = off right edge
int text_y = 1;
int text_min;                // Pos. where text resets (calc'd later)

void setup() {
  Serial.begin(115200);
  Serial.println("Adafruit QT RGB Matrix Scrolling Text Test");

  if (! matrix.begin(IS3741_ADDR_DEFAULT, i2c)) {
    Serial.println("IS41 not found");
    while (1);
  }

  Serial.println("IS41 found!");

  // By default the LED controller communicates over I2C at 400 KHz.
  // Arduino Uno can usually do 800 KHz, and 32-bit microcontrollers 1 MHz.
  i2c->setClock(800000);

  // Set brightness to max and bring controller out of shutdown state
  matrix.setLEDscaling(0xFF);
  matrix.setGlobalCurrent(0xFF);
  Serial.print("Global current set to: ");
  Serial.println(matrix.getGlobalCurrent());

  matrix.fill(0);
  matrix.enable(true); // bring out of shutdown
  matrix.setRotation(0);
  matrix.setTextWrap(false);

  // Get text dimensions to determine X coord where scrolling resets
  uint16_t w, h;
  int16_t ignore;
  matrix.getTextBounds(text, 0, 0, &ignore, &ignore, &w, &h);
  text_min = -w; // Off left edge this many pixels
}

void loop() {
  matrix.setCursor(text_x, text_y);
  for (int i = 0; i < (int)strlen(text); i++) {
    // set the color thru the rainbow
    uint32_t color888 = matrix.ColorHSV(65536 * i / strlen(text));
    uint16_t color565 = matrix.color565(color888);
    matrix.setTextColor(color565, 0); // backound is '0' to erase previous text!
    matrix.print(text[i]); // write the letter
  }

  if (--text_x < text_min) {
    text_x = matrix.width();
  }

  delay(25);
}
