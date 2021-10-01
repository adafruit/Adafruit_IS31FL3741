// Rainbow swirl example for the Adafruit IS31FL3741 13x9 PWM RGB LED Matrix
// Driver w/STEMMA QT / Qwiic connector. This is nearly the same as the
// prior qtmatrix example, but animation is generally smoother as it
// buffers the LED states in RAM and updates everything at once via the
// show() function (a la NeoPixels). This requires a little more memory but
// should still fit on small microcontrollers like Arduino Uno, unless
// there's a lot of other stuff going on.

#include <Adafruit_IS31FL3741.h>

Adafruit_IS31FL3741_QT_buffered ledmatrix;
// If colors appear wrong on matrix, try invoking constructor like so:
// Adafruit_IS31FL3741_QT_buffered ledmatrix(IS3741_RBG);

// Some boards have just one I2C interface, but some have more...
TwoWire *i2c = &Wire; // e.g. change this to &Wire1 for QT Py RP2040

void setup() {
  Serial.begin(115200);
  Serial.println("Adafruit QT RGB Matrix Simple RGB Swirl Test");

  if (! ledmatrix.begin(IS3741_ADDR_DEFAULT, i2c)) {
    Serial.println("IS41 not found");
    while (1);
  }

  Serial.println("IS41 found!");

  // By default the LED controller communicates over I2C at 400 KHz.
  // Arduino Uno can usually do 800 KHz, and 32-bit microcontrollers 1 MHz.
  i2c->setClock(800000);

  // Set brightness to max and bring controller out of shutdown state
  ledmatrix.setLEDscaling(0xFF);
  ledmatrix.setGlobalCurrent(0xFF);
  Serial.print("Global current set to: ");
  Serial.println(ledmatrix.getGlobalCurrent());
  ledmatrix.enable(true); // bring out of shutdown
}

uint16_t hue_offset = 0;

void loop() {
  uint32_t i = 0;
  for (int y=0; y<ledmatrix.height(); y++) {
    for (int x=0; x<ledmatrix.width(); x++) {
      uint32_t color888 = ledmatrix.ColorHSV(i * 65536 / 117 + hue_offset);
      uint16_t color565 = ledmatrix.color565(color888);
      ledmatrix.drawPixel(x, y, color565);
      i++;
    }
  }

  ledmatrix.show(); // Buffered matrix MUST use show() to update!

  hue_offset += 256;

  ledmatrix.setGlobalCurrent(hue_offset / 256); // Demonstrate global current
}
