//
// Toastmasters Timer Cube
//
// Chad Lawson <cdlawson@coldshadow.com>
// Gabe Becker
//

#include <SoftwareSerial.h>
#include <ClickEncoder.h>
#include <TimerOne.h>

// LED Pins
#define RED 8
#define YELLOW 9
#define GREEN 10
#define NULL 11 // Nothing on the pin, but only to be position zero in the arrray

// Define automatic time limits (in seconds)
const int presets[4][3] = {
  {0, 0, 0},        // Null. Just holding for manual mode.
  {60, 90, 120},    // Table Topics
  {120, 150, 180},  // Evaluation
  {300, 360, 420}   // Standard Speech
};
int whichPreset = 0; // Index of which preset based on mode
const int LEDArray[4] = {NULL, GREEN, YELLOW, RED};
//const char* myStrings[]={"0000", "1--2", "2--3", "5--7"};
const char* myStrings[]={"0000", "1  2", "2  3", "5  7"};

// Initiate Encoder
ClickEncoder *encoder;
int16_t last, value;

// These are the Arduino pins required to create a software serial
//  instance. We'll actually only use the TX pin.
const int softwareTx = 12;
const int softwareRx = 1; // Not in use, though
SoftwareSerial s7s(softwareRx, softwareTx);

unsigned int counter = 0;  // This variable will count up to 65k
char tempString[10];  // Will be used with sprintf to create strings

// Other variables
boolean isRunning;
boolean modeSelect;
boolean manualMode;
long startTime;

void timerIsr() {
  encoder->service();
}

void setup()
{ 
  // Create the lights and turn them off
  pinMode(GREEN, OUTPUT);
  pinMode(YELLOW, OUTPUT);
  pinMode(RED, OUTPUT);
  digitalWrite(GREEN, LOW);
  digitalWrite(YELLOW, LOW);
  digitalWrite(RED, LOW);
 
  // Setup the rotary encoder
  Serial.begin(9600);
  encoder = new ClickEncoder(A2, A1, A0, 4);
  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr);   
  last = -1;
  
  // Setup the display over serial
  s7s.begin(9600);
  clearDisplay();  // Clears display
  setBrightness(255);  // Full brightness
  s7s.print("8888");  // Show all digit segments and...
  setDecimals(0b111111);  // Turn on all options

  // Set initial isRunning state
  isRunning = 0; // False
  
  // set modeSelect
  modeSelect = 1; // True
} // SETUP

void loop() {
  
  // Check the Button
  ClickEncoder::Button b = encoder->getButton();
  switch(b) {
    case ClickEncoder::Held: // Reset
      startTime = millis();
      modeSelect = 1; 
      //clearDisplay();  // Clears display
      //setDecimals(0b000000);  // Turn off all options
      break;
       
    case ClickEncoder::Clicked: // Start & Stop
      if(isRunning) { // Stop
        isRunning = 0;
        showLED(0); 
      } else { // Start
        isRunning = 1;
        modeSelect = 0;
        startTime = millis();
        setDecimals(0b00010000);
      } // if(isRunning)
      break;
  } // switch(b)
    
  // Check state (isRunning or not)
  if(isRunning)
  {
    // Increment time
    long currentTime = millis();
    long accSecs = (currentTime - startTime) / 1000L;
    long minutes = accSecs / 60;
    long seconds = accSecs % 60;

    // If manual mode, knob changes LED
    // If preset mode, use time presets
    if(manualMode) {
      value += encoder->getValue();
      int whichOne = abs(value)%4; // TODO: Clear up this repeated bit
      if (value != last) {
        showLED(whichOne);
      }
    } else { // Using presets
      // Based on number of seconds,
      // which LED should be on.
      if(accSecs < presets[whichPreset][0]) { 
        showLED(0); // None
      } else if(accSecs < presets[whichPreset][1]) { 
        showLED(1); // Green
      } else if(accSecs < presets[whichPreset][2]) { 
        showLED(2); // Yellow
      } else { 
        showLED(3); // Red
      } // if(accSecs < presets[0])
    } // if(manualMode)
    
    // Display Time
    sprintf(tempString, "%02d%02d", (int)minutes, (int)seconds);
    s7s.print(tempString);

  } else { // not isRunning

    if(modeSelect) {      
      value += encoder->getValue();
      int whichOne = abs(value)%4; // TODO: There are too many of this line doing abs/mod math
      if (value != last) {
        
        // Knob controls mode (manual, TT, EV, SS)
        // Load timer presets
        if(whichOne == 0) {
          manualMode = 1;
          setDecimals(0b00010000); // Turn on the colon
        } else {
          manualMode = 0;
          whichPreset = whichOne;
          setDecimals(0b00000000); // All off
        }
        
        // Display the option
        s7s.print(myStrings[whichOne]);
        
      } //  if (value != last)
    } // if(modeSelect)

  } // End if/else for not isRunning

} // LOOP

////////////////
// Functions //
///////////////

void showLED(int whichOne) {
  
  // Light the right LED and turn off the others
  for(int i=0; i<4; i++) {
    if(i == whichOne) {
      digitalWrite(LEDArray[i], HIGH);
    } else {
      digitalWrite(LEDArray[i], LOW);
    } // if
  } // for
  
  // Light the correct decimal on the front
  // to match the LED
  switch(whichOne) {
    case 0:
      setDecimals(0b00010000);
      break;
    case 1:
      setDecimals(0b00010001);
      break;
    case 2:
      setDecimals(0b00010010);
      break;
    case 3:
      setDecimals(0b00010100);
      break;
  } // switch(whichOne)
  
} // showLED

// Send the clear display command (0x76)
//  This will clear the display and reset the cursor
void clearDisplay() {
  s7s.write(0x76);  // Clear display command
} // void clearDisplay

// Set the displays brightness. Should receive byte with the value
//  to set the brightness to
//  dimmest------------->brightest
//     0--------127--------255
void setBrightness(byte value) {
  s7s.write(0x7A);  // Set brightness command byte
  s7s.write(value);  // brightness data byte
} // setBrightness

// Turn on any, none, or all of the decimals.
//  The six lowest bits in the decimals parameter sets a decimal 
//  (or colon, or apostrophe) on or off. A 1 indicates on, 0 off.
//  [MSB] (X)(X)(Apos)(Colon)(Digit 4)(Digit 3)(Digit2)(Digit1)
void setDecimals(byte decimals) {
  s7s.write(0x77);
  s7s.write(decimals);
} // setDecimals

// This block is here for reference only.
// Once the code is complete and no longer needed
// for reference, it can be deleted.
void readButton() {
  ClickEncoder::Button b = encoder->getButton();
  if (b != ClickEncoder::Open) {
    Serial.print("Button: ");
    #define VERBOSECASE(label) case label: Serial.println(#label); break;
    switch (b) {
      VERBOSECASE(ClickEncoder::Pressed);
      VERBOSECASE(ClickEncoder::Held)
      VERBOSECASE(ClickEncoder::Released)
      VERBOSECASE(ClickEncoder::Clicked)
      case ClickEncoder::DoubleClicked:
          Serial.println("ClickEncoder::DoubleClicked");
          encoder->setAccelerationEnabled(!encoder->getAccelerationEnabled());
          Serial.print("  Acceleration is ");
          Serial.println((encoder->getAccelerationEnabled()) ? "enabled" : "disabled");
        break;
    } // switch (b) {
  } // if (b != ClickEncoder::Open) {
} // readButton
