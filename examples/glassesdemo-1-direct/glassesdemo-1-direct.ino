// Example for Adafruit LED glasses. This is the simplest version and
// should fit on small microcontrollers like Arduino Uno. Tradeoff is
// that animation isn't always as smooth as seen in the other examples.
// Each LED changes state immediately when accessed, there is no show()
// or display() function as with NeoPixels or some OLED screens.

#include <Adafruit_IS31FL3741.h>
#include <Fonts/TomThumb.h>

// Some boards have just one I2C interface, but some have more...
TwoWire *i2c = &Wire; // e.g. change this to &Wire1 for QT Py RP2040

// EyeLights glasses object in simplest form
Adafruit_EyeLights glasses;

char text[] = "ADAFRUIT!";    // A message to scroll
int text_x = glasses.width(); // Initial text position = off right edge
int text_min;                 // Pos. where text resets (calc'd later)
int text_y = 5;               // Text base line at bottom of matrix
uint16_t ring_hue = 0;        // For ring animation

void setup() {
  Serial.begin(115200);
  Serial.println("ISSI3741 LED Glasses Adafruit GFX Test");

  if (! glasses.begin(IS3741_ADDR_DEFAULT, i2c)) {
    Serial.println("IS41 not found");
    for (;;);
  }

  Serial.println("IS41 found!");

  // By default the LED controller communicates over I2C at 400 KHz.
  // Arduino Uno can usually do 800 KHz, and 32-bit microcontrollers 1 MHz.
  i2c->setClock(800000);

  // Set brightness to max and bring controller out of shutdown state
  glasses.setLEDscaling(0xFF);
  glasses.setGlobalCurrent(0xFF);
  glasses.enable(true);

  // Clear all LEDs, set to normal upright orientation
  glasses.fillScreen(0);
  glasses.setRotation(0);

  glasses.setFont(&TomThumb); // Tom Thumb 3x5 font, ideal for small matrix
  glasses.setTextWrap(false); // Allow text to extend off edges

  glasses.right_ring.setBrightness(50);  // Turn down the LED rings brightness,
  glasses.left_ring.setBrightness(50);   // 0 = off, 255 = max

  // Get text dimensions to determine X coord where scrolling resets
  uint16_t w, h;
  int16_t ignore;
  glasses.getTextBounds(text, 0, 0, &ignore, &ignore, &w, &h);
  text_min = -w; // Off left edge this many pixels
}

void loop() {
  // Because the LEDs are updated as drawing commands occur, we don't
  // want to clear the whole matrix, as that's relatively slow. Instead,
  // erase just the pixels of the last-drawn text. There's a tiny bit of
  // flicker, but less noticeable than a fillRect().
  glasses.setCursor(text_x, text_y);
  glasses.setTextColor(0);
  glasses.print(text);
  // See the other glasses examples for a smoother approach, RAM permitting.

  // Update text to new position, and draw
  if (--text_x < text_min) {    // If text scrolls off left edge,
    text_x = glasses.width(); // reset position off right edge
  }
  glasses.setCursor(text_x, text_y);
  for (int i = 0; i < (int)strlen(text); i++) {
    // Get 24-bit color for this character, cycling through color wheel
    uint32_t color888 = glasses.ColorHSV(65536 * i / strlen(text));
    // Remap 24-bit color to '565' color used by Adafruit_GFX
    uint16_t color565 = glasses.color565(color888);
    glasses.setTextColor(color565); // Set text color
    glasses.print(text[i]);         // and print one character
  }

  // The matrix and rings share some pixels in common. Drawing the
  // rings next makes them appear "on top" of the text on those pixels.
  // If this part is moved up above the text-drawing, the text will
  // appear on top of the rings.

  // Animate the LED rings with a color wheel. Unlike the matrix (which uses
  // GFX library's "565" color, the rings use NeoPixel-style RGB colors
  // (three 8-bit values, or one 24-bit value as returned here by ColorHSV()).
  for (int i=0; i < glasses.left_ring.numPixels(); i++) {
    glasses.left_ring.setPixelColor(i, glasses.ColorHSV(
      ring_hue + i * 65536 / glasses.left_ring.numPixels()));
  }
  for (int i=0; i < glasses.right_ring.numPixels(); i++) {
    glasses.right_ring.setPixelColor(i, glasses.ColorHSV(
      ring_hue - i * 65536 / glasses.right_ring.numPixels()));
  }
  ring_hue += 2000; // Shift color a bit on next frame - makes it spin

  // Pause briefly. Limits scrolling speed and keeps text lit for a moment.
  delay(75);
}
