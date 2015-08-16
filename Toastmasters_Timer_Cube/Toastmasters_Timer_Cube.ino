#include <SoftwareSerial.h>
#include <ClickEncoder.h>
#include <TimerOne.h>

// LED Pins
#define RED 8
#define YELLOW 9
#define GREEN 10
#define NULL 11 // Nothing on the pin, but only to be position zero in the arrray

// Define automatic time limits (in seconds)
const int tableTopics[3] = {60, 90, 120};
const int evaluation[3] = {120, 150, 180};
const int stdSpeech[3] = {300, 360, 420};
const int LEDArray[4] = {NULL, GREEN, YELLOW, RED};
const char* myStrings[]={"0000", "1--2", "2--3", "5--7"};

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
int presets[3];

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
}

void loop()
{
  
  // Check state (isRunning or not)
  if(isRunning)
  {
    setDecimals(0b00010000);
    
    // Increment time
    long currentTime = millis();
    long accSecs = (currentTime - startTime) / 100L;
    long minutes = accSecs / 60;
    long seconds = accSecs % 60;

    // If manual mode, knob changes LED
    // If preset mode, use time presets
    if(manualMode) {
      value += encoder->getValue();
      int whichOne = abs(value)%4;
      if (value != last) {
        showLED(whichOne);
      }
    } else {
      
      // Based on number of seconds,
      // which LED should be on.
      if(accSecs < presets[0]) { 
        showLED(0); // None
      } else if(accSecs < presets[1]) { 
        showLED(1); // Green
      } else if(accSecs < presets[2]) { 
        showLED(2); // Yellow
      } else { 
        showLED(3); // Red
      }
    } // if(manualMode)
    
    // Button stops and changes to control mode
    ClickEncoder::Button b = encoder->getButton();
    if (b == ClickEncoder::Clicked) { 
      isRunning = 0; 
    }
    
    // Display Time
    sprintf(tempString, "%02d%02d", (int)minutes, (int)seconds);
    s7s.print(tempString);
    

  } else { // not isRunning

    if(modeSelect) {      
      value += encoder->getValue();
      int whichOne = abs(value)%4;
      if (value != last) {
        manualMode = 0; // False
        
        // Knob controls mode (manual, TT, EV, SS)
        // Load timer presets
        switch(whichOne) {
          case 1: // Table Topics
            for(int i=0 ; i<3 ; i++) {
              presets[i] = tableTopics[i];
            }
            break;
          case 2: // Evaluation
            for(int i=0 ; i<3 ; i++) {
              presets[i] = evaluation[i];
            }
            break;
          case 3: // Standard Speech
            for(int i=0 ; i<3 ; i++) {
              presets[i] = stdSpeech[i];
            }
            break;
          default: // Manual
            manualMode = 1; // True
            break;
        } // switch(whichOne)
        
        // Display the option
        s7s.print(myStrings[whichOne]);
        
      } //  if (value != last)
    } // if(modeSelect)

    // Button starts timer
    ClickEncoder::Button b = encoder->getButton();
    
    if (b == ClickEncoder::Clicked) { 
      isRunning = 1;
      modeSelect = 0;
      startTime = millis();
    } // if (b == ClickEncoder::Clicked)
    
    // Resets on hold
    if(b == ClickEncoder::Held) { 
      startTime = millis();
      modeSelect = 1; 
      showLED(0);
      clearDisplay();  // Clears display
      setDecimals(0b000000);  // Turn on all options
    } // if(b == ClickEncoder::Held)

  } // End if/else for not isRunning

}

////////////////
// Functions //
///////////////

void showLED(int whichOne)
{
  for(int i=0; i<4; i++) {
    if(i == whichOne)
    {
      digitalWrite(LEDArray[i], HIGH);
    } 
    else
    {
      digitalWrite(LEDArray[i], LOW);
    }
  }
}

// Send the clear display command (0x76)
//  This will clear the display and reset the cursor
void clearDisplay()
{
  s7s.write(0x76);  // Clear display command
}

// Set the displays brightness. Should receive byte with the value
//  to set the brightness to
//  dimmest------------->brightest
//     0--------127--------255
void setBrightness(byte value)
{
  s7s.write(0x7A);  // Set brightness command byte
  s7s.write(value);  // brightness data byte
}

// Turn on any, none, or all of the decimals.
//  The six lowest bits in the decimals parameter sets a decimal 
//  (or colon, or apostrophe) on or off. A 1 indicates on, 0 off.
//  [MSB] (X)(X)(Apos)(Colon)(Digit 4)(Digit 3)(Digit2)(Digit1)
void setDecimals(byte decimals)
{
  s7s.write(0x77);
  s7s.write(decimals);
}

// This block is here for reference only.
// Once the code is complete and no longer needed
// for reference, it can be deleted.
void readButton()
{
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
}
