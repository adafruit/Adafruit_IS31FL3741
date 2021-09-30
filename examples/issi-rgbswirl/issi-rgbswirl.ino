// Example for ISSI AIS31FL3741 evaluation board. Fairly minimal, see the
// qtmatrix or ledglasses examples for more graphic-like examples.

#include <Adafruit_IS31FL3741.h>

Adafruit_IS31FL3741_EVB ledmatrix;

void setup() {
  Serial.begin(115200);
  Serial.println("ISSI 3741 Simple RGB Swirl Test");

  if (! ledmatrix.begin()) {
    Serial.println("IS41 not found");
    while (1);
  }

  Serial.println("IS41 found!");

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

  hue_offset += 256;

  ledmatrix.setGlobalCurrent(hue_offset / 256); // have display dim up and down
}
