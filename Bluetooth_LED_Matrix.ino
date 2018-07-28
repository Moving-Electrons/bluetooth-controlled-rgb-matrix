/*********************************************************************

 The code below includes some sections from Adafruit's example sketches. 
 Therefore, the text below has been kept as reference:

 -----
 
 This is an example for our nRF52 based Bluefruit LE modules.

 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution

 -----
 
*********************************************************************/

#include <bluefruit.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include <Fonts/FreeSans9pt7b.h>

// OLED Display
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);
#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif


// Function prototypes for functions in other files (from Adafruit examples)

uint8_t readPacket (BLEUart *ble_uart, uint16_t timeout);
float   parsefloat (uint8_t *buffer);
void    printHex   (const uint8_t * data, const uint32_t numBytes);


// Bluetooth packet buffer
extern uint8_t packetbuffer[];
BLEUart bleuart;

// Voltage divider logic

int VBAT_PIN = A7;
float Vbat = 0.0;
// The default values for the ADC on the nRF52 are 10-bit resolution (0..1023) 
// with a 3.6V reference voltage, meaning that every digit returned 
// by the ADC should be multiplied by:
float mV_per_lsb = 3600.0/1024.0;

// Per Adafruit examples, the internal voltage divider on the nRF52 
// (pin A7) has R2 = 2M and R1 = 0.806M. Therefore, Vin = R2*Vbat/(R1 + R2).
// Since we need Vbat, Vbat = Vin*(R1+R2)/R2; VBat = Vin*1.403.
float VBAT_DIVIDER_CONST = 1.403;

// Rolling average logic

const int numReadings = 10; // It needs to be const to define array size

int readings[numReadings];      // Readings from the analog input
int readIndex = 0;              // Index of current reading
int totalReadings = 0;          // Running total
int averageReadings = 0; 

// RGB Matrix 

#ifdef __AVR__
  #include <avr/power.h>
#endif
#define MATRIX_PIN 7
// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
// Adafruit_NeoPixel strip = Adafruit_NeoPixel(64, PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel matrix = Adafruit_NeoPixel(64, MATRIX_PIN);

// Main global variables to be used on functions

uint8_t RCode, GCode, BCode;
int matrixBrightness = 100; // Initial matrix brightness. It ranges from 0 to 100
int brightStep = 10; // Brightness increment
int colorTemperature = 5000; // Initial color temperature value
int colorStep = 250; // Color temperature increment
int minColorTemp = 1000; // Minimum color temperature.
int maxColorTemp = 10000; // Maximum color temperature. Complete white is around 6500.

// Auxiliar global variables

int avgBatPercent = 0;
String oledValue = "0|0|0"; // Initial oled message
uint8_t firstLineIcon = 1; // First line icon. 1 = Paint/roller, 2 = Thermometer.



/*--------------------------------*/
void setup(void)
{
  Serial.begin(115200);
  
  /*
   * ----- Bluetooth Initialization -----
   */
  
  Serial.println(F("Bluetooth Controlled LED Matrix"));
  Serial.println(F("-------------------------------"));

  Bluefruit.begin();
  // Set max power. Accepted values are: -40, -30, -20, -16, -12, -8, -4, 0, 4
  Bluefruit.setTxPower(4);
  Bluefruit.setName("Bluetooth LED Matrix-1");

  // Configure and start the BLE Uart service
  bleuart.begin();

  // Set up and start advertising
  startAdv();

  Serial.println(F("Please use Adafruit Bluefruit LE app to connect in Controller mode"));
  Serial.println(F("Then activate/use the sensors, color picker, game controller, etc!"));
  Serial.println();  

  /* 
   *  ----- Screen Initialization -----
   */
  //delay(800);
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  
  display.clearDisplay(); 
  
  // display a pixel in each corner of the screen
  // display.drawPixel(0, 0, WHITE);
  // display.drawPixel(127, 0, WHITE);
  // display.drawPixel(0, 63, WHITE);
  // display.drawPixel(127, 63, WHITE);
  
  display.setTextColor(WHITE);

  // Rooling average - Array initialization
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }

  /*
   * ----- OLED Matrix Initialization -----
   */
  
  matrix.begin();
  matrix.show(); // Initialize all pixels to 'off'
  
}


void loop(void)
{
  
  calcBatteryLevel(); // Updates the global variable "avgBatPercent" which is
                      // read by the drawDisplay function.

  drawDisplay(); //Updates display based on global variables

  drawMatrix(); //Updates matrix based on global variables
  
  uint8_t len = readPacket(&bleuart, 500); // Last parameter = timeout value.
  
  if (len != 0) {

    // Got a packet!
    //printHex(packetbuffer, len);
    //Serial.print("Array Char 0:");
    //Serial.println((char) packetbuffer[0]);
    //Serial.print("Array Char 1:");
    //Serial.println((char) packetbuffer[1]);
    //Serial.print("Array 0:");
    //Serial.println(packetbuffer[0]);
    //Serial.print("Array 1:");
    //Serial.println(packetbuffer[1]);


    // ----- Color -----
    if (packetbuffer[1] == 'C') {
      
      // From Adafruit examples:
      //uint8_t red = packetbuffer[2];
      //uint8_t green = packetbuffer[3];
      //uint8_t blue = packetbuffer[4];
      
      //Serial.print ("RGB #");
      //if (red < 0x10) Serial.print("0");
      //Serial.print(red, HEX);
      //if (green < 0x10) Serial.print("0");
      //Serial.print(green, HEX);
      //if (blue < 0x10) Serial.print("0");
      //Serial.println(blue, HEX);

      firstLineIcon = 1;

      RCode = packetbuffer[2];
      GCode = packetbuffer[3];
      BCode = packetbuffer[4];
      oledValue = (String)RCode+"|"+(String)GCode+"|"+(String)BCode;
      
      
    }
  
    // ----- Buttons -----
    if (packetbuffer[1] == 'B') {
      
      // From Adafruit examples:
      uint8_t buttnum = packetbuffer[2] - '0';
      boolean pressed = packetbuffer[3] - '0';

      // Left arrow - decrease brightness
      if (buttnum == 7 && pressed) {
        
        // Important: In the line below the "address" of the variable is sent to updateVariable
        // so that the actual matrixBrightness value can be updated by the function.
        updateVariable(&matrixBrightness, brightStep, 0, 100, 0); 
      }

      // right arrow - increase brightness
      if (buttnum == 8 && pressed) {
      
        updateVariable(&matrixBrightness, 10, brightStep, 100, 1); 
      }

      // Up arrow - increase color temperature
      if (buttnum == 5 && pressed) {
        
        updateVariable(&colorTemperature, colorStep, minColorTemp, maxColorTemp, 1);
        kelvinToRGB(colorTemperature); // Assigns equivalent RGB codes to global variables
        oledValue = (String) colorTemperature;
        firstLineIcon = 0;
      }

      // Down arrow - decrease color temperature
      if (buttnum == 6 && pressed) {

        updateVariable(&colorTemperature, colorStep, 1000, maxColorTemp, 0);
        kelvinToRGB(colorTemperature);
        oledValue = (String) colorTemperature;
        firstLineIcon = 0;
      }

      // Button 1 - default settings (matrix turned off)
      if (buttnum == 1 && pressed) {

        RCode = GCode = BCode = 0;
        oledValue = (String)RCode+"|"+(String)GCode+"|"+(String)BCode;
        matrixBrightness = 100;
        firstLineIcon = 1;
      }

      // Button 2 - All white, full blast!
      if (buttnum == 2 && pressed) {

        RCode = GCode = BCode = 255;
        oledValue = (String)RCode+"|"+(String)GCode+"|"+(String)BCode;
        matrixBrightness = 100;
        firstLineIcon = 1;
      }

      // Button 3 - Theatre chase effect
      if (buttnum == 3 && pressed) {

        oledValue = "Effect";
        firstLineIcon = 1;
        
        drawDisplay(); // draws display now, otherwise it would show the OLED message after 
                       // the "effect" is run since drawDisplay() is called at the beginning 
                       // of loop()
        chaseEffect(50); // random colors. Changing every 50 ms
        
        RCode = GCode = BCode = 0; // turns the Matrix off after effect
   
      }
        
    }
    
  }


}


/*
----- Bluetooth Advertisement -----
*/
void startAdv(void)
{
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  
  // Include the BLE UART (AKA 'NUS') 128-bit UUID
  Bluefruit.Advertising.addService(bleuart);

  // Secondary Scan Response packet (optional)
  // Since there is no room for 'Name' in Advertising packet
  Bluefruit.ScanResponse.addName();

  /* Start Advertising
   * - Enable auto advertising if disconnected
   * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   * 
   * For recommended advertising interval
   * https://developer.apple.com/library/content/qa/qa1931/_index.html   
   */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds  
}


/*
 * ----- Battery Monitoring -----
 */

 void calcBatteryLevel() {

  int rawIn, batPercent;
  rawIn = analogRead(VBAT_PIN);

  Vbat = rawIn * mV_per_lsb * VBAT_DIVIDER_CONST; // Vbat in millivolts
  
  batPercent = map(int(Vbat), 3200, 4200, 0, 100); // Casting Vbat to int as map doesn' work with floats.
  //Serial.println(Vbat);
  //Serial.println(batPercentage);

  // Rolling average logic
  totalReadings = totalReadings - readings[readIndex]; // Substracts the last reading AT THE CURRENT POSITION of the array:
  readings[readIndex] = batPercent;
  totalReadings = totalReadings + readings[readIndex];

  readIndex = readIndex + 1;

  if (readIndex >= numReadings) {
    readIndex = 0;
  }

  avgBatPercent = totalReadings/numReadings;
 }


/*
 * ----- Helper Function increase/decrease numbers -----
 */

 // Updates variable passed as argument per delta, Max and Min values. Used for increasing/decreasing
 // brightness values and color temperatures.
 // The function receives a pointer to the variable (i.e. address in memory) so that the actual variable value
 // can be modified (and not just passed as an argument).
 // int variable used for pointer so it doesn't go to 65365 below zero (0)  like uint16_t.
  
 void updateVariable (int *variablePointer, uint16_t delta, uint16_t minValue, uint16_t maxValue, boolean increase) {


  int variableValue = *variablePointer;
  
  if (increase) {
    Serial.println("increase..");
    variableValue = variableValue + delta;
    //Serial.print("Updated variable value: ");Serial.println(variableValue);
    if (variableValue >= maxValue) {
      variableValue = maxValue; 
    }
    
  }
  else {
    variableValue = variableValue - delta;
    if (variableValue <= minValue) {
      variableValue = minValue;
    }
  }

  *variablePointer = variableValue;
 }


 /*
 * ----- Kelvin to RGB Transformation -----
 */

 // Updates RGB Global variables based on the color temperature selected.
 // Modified algorithm from: http://www.tannerhelland.com/4435/convert-temperature-rgb-algorithm-code/
 void kelvinToRGB (int kelvinTemp) {

  float calcRed, calcGreen, calcBlue, floatTemp = 0;
  

  kelvinTemp = kelvinTemp/100;
  floatTemp = (float) kelvinTemp;

  // Calculating Red:
  if (kelvinTemp <= 66) {
    calcRed = 255;
  }
  else {

  calcRed = 329.69872*pow((floatTemp-60),-0.1332);

    if (calcRed < 0 ) {
      calcRed = 0;
    }
    if (calcRed > 255 ) {
      calcRed = 255;
    }  
  }

  // Calculating Green:
  if (kelvinTemp <= 66) {
    calcGreen = (99.47080 * log(kelvinTemp)) - 161.11956;
  }
  else {
    calcGreen = 288.12217 * pow((kelvinTemp - 60), -0.07551);
  }
  
  if (calcGreen < 0 ) {
      calcGreen = 0;
    }
  if (calcGreen > 255 ) {
      calcGreen = 255;
    } 

    // Calculating Blue:
    if (kelvinTemp >= 66) {
      calcBlue = 255;
    }
    else {

       if (kelvinTemp <= 19) {
        calcBlue = 0;
       }
       else {
        calcBlue = (138.51773 * log(kelvinTemp - 10)) - 305.04479;
       }
    }

    if (calcBlue < 0 ) {
      calcBlue = 0;
    }
    if (calcBlue > 255 ) {
      calcBlue = 255;
    }


   RCode = (int) calcRed;
   GCode = (int) calcGreen;
   BCode = (int) calcBlue;
   
  
  //Serial.print("kelvin: ");Serial.println(kelvinTemp);
  //Serial.print("red float: ");Serial.println(calcRed);
  //Serial.print("red int: ");Serial.println((int) calcRed);
  //Serial.print("green int: ");Serial.println((int) calcGreen);
  //Serial.print("blue int: ");Serial.println((int) calcBlue);
 
  
 }


