#include <Adafruit_IS31FL3741.h>
Adafruit_IS31FL3741_QT matrix;


void setup() {
  Serial.begin(115200);
  Serial.println("ISSI 3741 QT Adafruit GFX Test");

  if (! matrix.begin()) {
    Serial.println("IS41 not found");
    while (1);
  }
  
  Serial.println("IS41 found!");

  // speed it up if you can!
  Wire.setClock(800000);

  matrix.setLEDscaling(0xFF); 
  matrix.setGlobalCurrent(0xFF);
  Serial.print("Global current set to: ");
  Serial.println(matrix.getGlobalCurrent());

  matrix.fill(0);
  matrix.enable(true); // bring out of shutdown
  matrix.setRotation(0);
  matrix.setTextWrap(false);
}

int text_x = matrix.width();
int text_y = 1;
uint8_t color;
char adafruit[] = "ADAFRUIT!";

void loop() {
  matrix.setCursor(text_x, text_y);
  for (int i = 0; i < (int)strlen(adafruit); i++) {
    // set the color thru the rainbow
    uint32_t color888 = Wheel((256UL * i) / strlen(adafruit));  // Wheel gives us 888 color
    uint16_t color565 = matrix.Color565(color888>>16, color888>>8, color888); // so we have to convert it to 565 here!
    
    matrix.setTextColor(color565, 0); // backound is '0' to erase previous text!
    
    // write the letter
    matrix.print(adafruit[i]);
  }

  if (--text_x < -50) {
    text_x = matrix.width();
    matrix.fillRect(0, 0, matrix.width(), text_y + 8, 0); // erase old text
  }

  delay(25);
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
