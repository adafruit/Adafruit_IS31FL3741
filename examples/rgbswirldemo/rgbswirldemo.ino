#include <Adafruit_IS31FL3741.h>

Adafruit_IS31FL3741 ledmatrix = Adafruit_IS31FL3741();

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

uint8_t wheel_offset = 0;

void loop() {
  for (int i=0; i<351; i+=3) {
    uint32_t color = Wheel(i + wheel_offset);
    ledmatrix.setLEDPWM(i+2, color>>16);
    ledmatrix.setLEDPWM(i+1, color>>8);
    ledmatrix.setLEDPWM(i, color);
  }
  wheel_offset++;
  
  ledmatrix.setGlobalCurrent(wheel_offset);  // have the display dim up and down
}


// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  uint32_t c = 0;
  if (WheelPos < 85) {
     c = (255 - WheelPos*3) & 0xFF;
     c <<= 8;
     c |= (WheelPos * 3) & 0xFF;
     c <<= 8;
  } else if (WheelPos < 170) {
     WheelPos -= 85;
     c <<= 8;
     c |= (255 - WheelPos*3) & 0xFF;
     c <<= 8;
     c |= (WheelPos * 3) & 0xFF;
  } else {
     WheelPos -= 170;
     c = (WheelPos * 3) & 0xFF;
     c <<= 16;
     c |= (255 - WheelPos*3) & 0xFF;
  }
  return c;
}