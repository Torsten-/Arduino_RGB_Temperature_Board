/*
  https://github.com/Torsten-/Arduino_RGB_Temperature_Board

  Copyright (C) 2014  Temperature Board
  Torsten Amshove <torsten@amshove.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

///////////////////////
//// Configuration ////
///////////////////////
// Pins for Shift-Registers
#define PIN_RCK A3 // Latch
#define PIN_SCK A4 // Clock
#define PIN_SI  A1 // Data

// Pins for 7-segment columns and RGB-LEDs
byte pins_cols[] = {2,4,7,8,12,13};
byte pins_rgb1[] = {3,6,5};
byte pins_rgb2[] = {9,11,10};

// Serial
#define SERIAL_BAUD 9600 // Baud-Rate of serial port

// Timeout for "no data received"
#define TIMEOUT 10 // in Minutes

/////////////////////////////////////
//// Initialise global variables ////
/////////////////////////////////////
String serialInputString = ""; // String/Command typed in serial console to Bunny
unsigned long last_data = 0;   // Millis-Timestamp of last time data was received
byte act_col = 0;              // Actual column in loop-cycle
uint8_t act_digit1;            // Value of actual first 7-segment-digit in loop-cycle
uint8_t act_digit2;            // Value of actual second 7-segment-digit in loop-cycle

// Array for translation of numbers [0-9] to 7-segment-bits
uint8_t numbers[] = {
  //HGFEDCBA
  0b10000001,
  0b10110111,
  0b11000010,
  0b10010010,
  0b10110100,
  0b10011000,
  0b10001000,
  0b10110011,
  0b10000000,
  0b10010000
};

// Values of other things to display with 7-segment display
uint8_t nothing = 0b11111111; // Turn off all segments
uint8_t minus   = 0b11111110; // Show a minus
uint8_t degree  = 0b11110000; // Show degree: °
uint8_t celsius = 0b11001001; // Show Celsius: C

// Initialize first values for 7-segment displays and RGB LEDs
uint8_t value1[] = {1,0,0,0};
uint8_t value2[] = {1,0,0,0};
uint8_t rgb1[] = {0,0,0};
uint8_t rgb2[] = {0,0,0};

///////////////
//// Setup ////
///////////////
void setup(){
  // Set all pins to output
  pinMode(PIN_RCK, OUTPUT);
  pinMode(PIN_SCK, OUTPUT);
  pinMode(PIN_SI,  OUTPUT);
  
  for (uint8_t i=0; i<6; i++){
    pinMode(pins_cols[i], OUTPUT);
  }
  
  for (uint8_t i=0; i<3; i++){
    pinMode(pins_rgb1[i], OUTPUT);
    pinMode(pins_rgb2[i], OUTPUT);
  }
  
  // Init Serial
  Serial.begin(SERIAL_BAUD);
  serialInputString.reserve(11); // Max. Size in byte of serial command
}

//////////////
//// Loop ////
//////////////
void loop(){
  /////////////
  // Timeout //
  /////////////
  // If data is not updated since 10 minutes, clear values to display the error
  if((last_data+(TIMEOUT*60000)) < millis()){
    for(uint8_t i=1; i<6; i++){
      value1[i] = 0;
      value2[i] = 0;
    }
    value1[0] = 1;
    value2[0] = 1;
    for(uint8_t i=0; i<3; i++){
      rgb1[i] = 0;
      rgb2[i] = 0;
    }
    changeRGB();
  }

  ///////////////////////////////////////
  // Show values on 7-segment displays //
  ///////////////////////////////////////
  if(act_col == 6) act_col = 0; // If last 7-segment column is reached start over  
  switch(act_col){
    case 5:
      // Left most column - show minus or nothing
      if(value1[0] == 0) act_digit1 = nothing;
      else act_digit1 = minus;
      
      if(value2[0] == 0) act_digit2 = nothing;
      else act_digit2 = minus;
      
      break;
    case 4:
      // Don't display leading zeros
      if(value1[1] == 0) act_digit1 = nothing;
      else act_digit1 = numbers[value1[1]];
      
      if(value2[1] == 0) act_digit2 = nothing;
      else act_digit2 = numbers[value2[1]];
      
      break;
    case 3:
      // Digit before decimal point
      act_digit1 = numbers[value1[2]];
      act_digit2 = numbers[value2[2]];
      
      // Add decimal point
      act_digit1 = act_digit1 & 0b01111111;
      act_digit2 = act_digit2 & 0b01111111;
      
      break;
    case 2:
      // Digit after decimal point
      act_digit1 = numbers[value1[3]];
      act_digit2 = numbers[value2[3]];
      break;
    case 1:
      // Show degree sign
      act_digit1 = degree;
      act_digit2 = degree;
      break;
    case 0:
      // Show Celsius C
      act_digit1 = celsius;
      act_digit2 = celsius;
      break;
  }
  
  // Shift out the digit of the actual column
  shiftOut(PIN_SI, PIN_SCK, LSBFIRST, act_digit1);
  shiftOut(PIN_SI, PIN_SCK, LSBFIRST, act_digit2);
  digitalWrite(PIN_RCK, LOW);
  digitalWrite(PIN_RCK, HIGH);

  // Display the digit of this column for a short time  
  digitalWrite(pins_cols[act_col], HIGH);
  delay(1);
  digitalWrite(pins_cols[act_col],LOW);

  // Go to next column
  act_col++;
}

//////////////////////
//// Serial Event ////
//////////////////////
// This function is called when data is available on serial
// http://arduino.cc/en/Reference/SerialEvent
void serialEvent(){
  while (Serial.available()){
    char inChar = (char)Serial.read();
    if((inChar >= 32 && inChar < 127) || inChar == 13 || inChar == 10){ // Ignore non-printable characters (beside CR/LF (ENTER))
      if (inChar == '\n'){                    // Input string is complete
        serialInputString.replace(",",".");   // Allow , and . as decimal point
        
        // Parse input and store to display values
        float val1 = serialInputString.substring(0,serialInputString.indexOf(";")).toFloat();
        parseInput(&val1,value1,rgb1);
        float val2 = serialInputString.substring(serialInputString.indexOf(";")+1).toFloat();
        parseInput(&val2,value2,rgb2);
        
        // Update last update timestamp
        last_data = millis();
        
        // Reset command-variable
        serialInputString = "";
      }else{
        // Collect serial input
        serialInputString += inChar;
      }
    }
  }
}

////////////////
// parseInput //
////////////////
// Parses the serial input and stores it in the 7-segment-array
// Also calculates RGB-Values and updates LEDs
void parseInput(float *input, uint8_t *value, uint8_t *rgb){
  // Strip down input to one value per column
  int tmp = *input * 10;                // make integer out of float
  if(*input < 0){                       // show minus or not
    value[0] = 1;
    tmp = tmp * -1;                     // make integer positive for next calculations
  }else value[0] = 0;
  if((tmp/10) > 9) value[1] = tmp/100;  // get the 10'er values
  else value[1] = 0;
  value[2] = (tmp / 10) % 10;           // get the value before decimal point
  value[3] = tmp % 10;                  // get the value after decimal point
  
  // Calculate RGB-Value
  rgb[0] =                map(constrain(*input,10,30),10,30,0,255);   // Red
  if(*input > 0) rgb[1] = map(constrain(*input,10,30),10,30,255,0);   // Green
  else rgb[1] =           map(constrain(*input,-10,10),-10,10,0,255); // Green
  rgb[2] =                map(constrain(*input,-10,10),-10,10,255,0); // Blue
  
  // Show new RGB-Values with LEDs
  changeRGB();
}

////////////////
// Change RGB //
////////////////
// Sets the current RGB values to LEDs
void changeRGB(){
  analogWrite(pins_rgb1[0], rgb1[0]);
  analogWrite(pins_rgb1[1], rgb1[1]);
  analogWrite(pins_rgb1[2], rgb1[2]);
  
  analogWrite(pins_rgb2[0], rgb2[0]);
  analogWrite(pins_rgb2[1], rgb2[1]);
  analogWrite(pins_rgb2[2], rgb2[2]);
}
