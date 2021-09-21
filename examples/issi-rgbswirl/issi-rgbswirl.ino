// Example for ISSI AIS31FL3741 evaluation board. Fairly minimal, see the
// qtmatrix or ledglasses examples for more graphic-like examples.

#include <Adafruit_IS31FL3741.h>

Adafruit_IS31FL3741 ledmatrix;

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
  for (int i=0; i<351; i+=3) {
    uint32_t color = ledmatrix.ColorHSV(i * 65536 / 351 + hue_offset);
    ledmatrix.setLEDPWM(i+2, color>>16);
    ledmatrix.setLEDPWM(i+1, color>>8);
    ledmatrix.setLEDPWM(i, color);
  }
  hue_offset += 256;
  
  ledmatrix.setGlobalCurrent(hue_offset / 256); // have display dim up and down
}
