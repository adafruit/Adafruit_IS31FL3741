#include <Adafruit_IS31FL3741.h>
#include <Fonts/TomThumb.h>
Adafruit_IS31FL3741 ledcontroller = Adafruit_IS31FL3741();
Adafruit_IS31FL3741_GlassesMatrix ledmatrix = Adafruit_IS31FL3741_GlassesMatrix(&ledcontroller);
Adafruit_IS31FL3741_GlassesLeftRing leftring = Adafruit_IS31FL3741_GlassesLeftRing(&ledcontroller);
Adafruit_IS31FL3741_GlassesRightRing rightring = Adafruit_IS31FL3741_GlassesRightRing(&ledcontroller);

void setup() {
  Serial.begin(115200);
  Serial.println("ISSI3741 LED Glasses Adafruit GFX Test");

  if (! ledcontroller.begin()) {
    Serial.println("IS41 not found");
    while (1);
  }
  
  Serial.println("IS41 found!");

  // speed it up if you can!
  Wire.setClock(800000);

  ledcontroller.setLEDscaling(0xFF); 
  ledcontroller.setGlobalCurrent(0xFF);
  ledcontroller.enable(true); // bring out of shutdown

  ledmatrix.fillRect(0, 0, ledmatrix.width(), ledmatrix.height(), 0x0);
  ledmatrix.setRotation(0);
  ledmatrix.setFont(&TomThumb);
  ledmatrix.setTextWrap(false);

  rightring.setBrightness(50);
  leftring.setBrightness(50);
}

int text_x = ledmatrix.width();
int text_y = 5;
uint8_t color;
uint8_t ring_wheel = 0;

char text[] = "ADAFRUIT!";


void loop() {
  //ledmatrix.fillRect(0, 0, ledmatrix.width(), text_y, 0x0000); // erase old text
  ledmatrix.setCursor(text_x, text_y);
  for (int i = 0; i < (int)strlen(text); i++) {
    // set the color thru the rainbow
    uint32_t color888 = Wheel((256UL * i) / strlen(text));  // Wheel gives us 888 color
    uint16_t color565 = ledcontroller.color565(color888>>16, color888>>8, color888); // so we have to convert it to 565 here!

    ledmatrix.setTextColor(color565);
    // write the letter
    ledmatrix.print(text[i]);
  }
  
  delay(75);

  // erase the letters
  ledmatrix.setCursor(text_x, text_y);
  for (int i = 0; i < (int)strlen(text); i++) {
    ledmatrix.setTextColor(0); // '0' to erase previous text!    
    ledmatrix.print(text[i]);
  }

  // if we scroll past, start over!
  if (--text_x < -50) {
    text_x = ledmatrix.width();
  }

  for (int i=0; i < leftring.numPixels(); i++) {
    leftring.setPixelColor(i, Wheel(((i * 256 / leftring.numPixels()) + ring_wheel) & 255));
  }
  for (int i=0; i < rightring.numPixels(); i++) {
    rightring.setPixelColor(rightring.numPixels() - i - 1, 
                            Wheel(((i * 256 / rightring.numPixels()) + ring_wheel) & 255));
  }
  ring_wheel += 5;  
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
