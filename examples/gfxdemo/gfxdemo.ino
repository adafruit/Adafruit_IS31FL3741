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
    uint32_t color888 = matrix.ColorHSV(65536 * i / strlen(adafruit));
    uint16_t color565 = matrix.color565(color888);
    
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
