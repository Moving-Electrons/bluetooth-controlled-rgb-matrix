
/*
----- Icon Functions -----
*/


void batteryIcon (uint16_t x, uint16_t y) {

display.fillRect(x, y, 12, 5, WHITE);
display.fillRect(x+13, y+1, 1, 3, WHITE);
  
}

void bluetoothIcon (uint16_t x, uint16_t y) {

// 2 dots
display.drawPixel(x, y+3, WHITE);
display.drawPixel(x, y+6, WHITE);

// 2 triangles
display.drawPixel(x+3, y+1, WHITE);
display.drawPixel(x+3, y+3, WHITE);
display.drawPixel(x+4, y+2, WHITE);

display.drawPixel(x+3, y+6, WHITE);
display.drawPixel(x+3, y+8, WHITE);
display.drawPixel(x+4, y+7, WHITE);

// lines
display.drawFastVLine(x+2, y, 10, WHITE);
display.drawFastVLine(x+1, y+4, 2, WHITE);


  
}


void brightIcon (uint16_t x, uint16_t y) {

  // Vertical lines
  display.drawFastVLine(x+5, y, 2, WHITE);
  display.drawFastVLine(x+7, y+4, 3, WHITE);
  display.drawFastVLine(x+3, y+4, 3, WHITE);
  display.drawFastVLine(x+5, y+9, 2, WHITE);

  // Horizontal lines
  display.drawFastHLine(x+4, y+3, 3, WHITE);
  display.drawFastHLine(x+4, y+7, 3, WHITE);
  display.drawFastHLine(x, y+5, 2, WHITE);
  display.drawFastHLine(x+9, y+5, 2, WHITE);

  // Diagonal lines
  display.drawPixel(x+1, y+1, WHITE);
  display.drawPixel(x+2, y+2, WHITE);
  
  display.drawPixel(x+9, y+1, WHITE);
  display.drawPixel(x+8, y+2, WHITE);
  
  display.drawPixel(x+1, y+9 , WHITE);
  display.drawPixel(x+2, y+8 , WHITE);
  
  display.drawPixel(x+8, y+8 , WHITE);
  display.drawPixel(x+9, y+9 , WHITE);
  
}

void paintIcon (uint16_t x, uint16_t y) {

  // Main rectangle
  display.fillRect(x+3, y, 9, 3, WHITE);
  
  // Horizontal lines
  display.drawFastHLine(x, y+1, 3, WHITE);
  display.drawFastHLine(x, y+4, 8, WHITE);

  // Vertical lines
  display.drawFastVLine(x, y+2, 2, WHITE);
  display.drawFastVLine(x+7, y+4, 8, WHITE);
  display.drawFastVLine(x+8, y+8, 4, WHITE);
 
}

void thermoIcon (uint16_t x, uint16_t y) {

  // Horizontal lines
  display.drawFastHLine(x+2, y, 3, WHITE);
  display.drawFastHLine(x+2, y+11, 3, WHITE);
  
  
  // Vertical lines
  display.drawFastVLine(x+2, y, 7, WHITE);
  display.drawFastVLine(x+4, y, 7, WHITE);
  
  display.drawFastVLine(x, y+8, 2, WHITE);
  display.drawFastVLine(x+6, y+8, 2, WHITE);

  // Pixels
  // base
  display.drawPixel(x+1, y+7, WHITE);
  display.drawPixel(x+5, y+7, WHITE);

  display.drawPixel(x+1, y+10, WHITE);
  display.drawPixel(x+5, y+10, WHITE);

  // levels
  display.drawPixel(x+5, y+2, WHITE);
  display.drawPixel(x+5, y+4 , WHITE);
  
}


 /* 
  *  ----- Matrix LED Functions -----
  */


// Sets all LEDs on matrix to the colors defined on the global variables
void drawMatrix(){

  for(uint16_t p=0; p<matrix.numPixels(); p++) {
    // "brightness" is used as a factor to modify RGB codes: the closer the final number is
    // to zero, the less bright the LED is. Range goes from 0 to 100.
    // 100 is maximum brightness.
    
    matrix.setPixelColor(p, (matrixBrightness*RCode)/100, (matrixBrightness*GCode)/100, (matrixBrightness*BCode)/100);
    //matrix.setPixelColor(p, RCode, GCode, BCode);
  }
  matrix.show();
}


//Theatre-style crawling lights effect
void chaseEffect(uint8_t wait) {

  int color;
  int randomRGB[3]; // Array for holding random RGB values

  for (int j=0; j<20; j++) {  //Number of cycles for the effect
    for (int q=0; q < 3; q++) {

      // Generating random RGB values:
      for (int p=0; p<3; p++) {
        randomRGB[p] = random(10,200); // Limits RGB color range. Full range: 0-255
      }
      color = matrix.Color(randomRGB[0], randomRGB[1], randomRGB[2]);
      
      for (uint16_t i=0; i < matrix.numPixels(); i=i+3) {
        matrix.setPixelColor(i+q, color);    //turn every third pixel on
      }
      matrix.show();
      delay(wait);

      for (uint16_t i=0; i < matrix.numPixels(); i=i+3) {
        matrix.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}


 /* 
  *  ----- OLED Display Functions -----
  */
  
void drawDisplay() {

    
  // Cleans previous values:
  batteryIcon(0,0);
  display.fillRect(19, 0, 32, 12, BLACK); // battery
  display.fillRect(0, 14, 128, 28, BLACK); // oled value AND first line icon
  display.fillRect(24, 38, 100, 52, BLACK); // brightness value only (brightness icon always on)
  

  // Shows battery percentageon display
  display.setTextSize(1);
  display.setCursor(19,0);
  display.print(avgBatPercent);display.print("%");

  // Shows bluetooth icon if a device is connected:
  if (Bluefruit.connected()) {
    //Serial.println("Bluetooth device connected.");
    bluetoothIcon(115,0);  
  } 
  else {
    display.fillRect(115, 0, 120, 15, BLACK); 
  }

  if (firstLineIcon == 1) {
    paintIcon(7, 17);
  }
  else {
    thermoIcon(8,18);
  }  

  brightIcon(7,41); // Brightness icon (always shown).

  // RGB code text
  display.setFont(&FreeSans9pt7b);
  //display.setTextSize(0);
  display.setCursor(24,28);
  //display.print("255|255|255");
  display.print(oledValue); // First line message

  // Brightness text
  //display.setTextSize(2);
  display.setCursor(24,52);
  display.print(matrixBrightness); // Second line message

  display.setFont(); // Sets the Font back to default


  display.display();

  
}


