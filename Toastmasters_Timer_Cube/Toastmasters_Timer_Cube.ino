#include <SoftwareSerial.h>
#include <ClickEncoder.h>
#include <TimerOne.h>

// LED Pins
#define RED 8
#define YELLOW 9
#define GREEN 10

// Define automatic time limits (in seconds)
const int tableTopics[3] = {60, 90, 120};
const int evaluation[3] = {120, 150, 180};
const int stdSpeech[3] = {300, 360, 420};
const int LEDArray[3] = {GREEN, YELLOW, RED};
const char* myStrings[]={"0000", "1--2", "2--3", "5--7"};

// Initiate Encoder
ClickEncoder *encoder;
int16_t last, value;

// These are the Arduino pins required to create a software seiral
//  instance. We'll actually only use the TX pin.
const int softwareTx = 12;
const int softwareRx = 1; // Not in use, though
SoftwareSerial s7s(softwareRx, softwareTx);

unsigned int counter = 0;  // This variable will count up to 65k
char tempString[10];  // Will be used with sprintf to create strings

void timerIsr() {
  encoder->service();
}

void setup()
{ 
  // Create the lights
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

  for(int x=0 ; x<3 ; x++) {
     showLED(x);
     delay(500);
  }

  // Do as I say, not as I do
  //delay(500); // This is just test code so we can see the display light up
  clearDisplay(); // Now let's get to work
  
}

void loop()
{
  
  value += encoder->getValue();
  if (value != last) {
    last = value;
  
    int choice = value % 4;
    //sprintf(tempString, "%4d", value);
    //s7s.print(tempString);
    s7s.print(myStrings[choice]);
    if(value % 4 == 0) {
      setDecimals(0b00010000);
    } else {
      setDecimals(0b00000000);
    }
    
    showLED(value % 3);
  }
  
}

////////////////
// Functions //
///////////////

void showLED(int whichOne)
{
  for(int i=0; i<3; i++) {
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

void origDispLoop()
{
  // Magical sprintf creates a string for us to send to the s7s.
  //  The %4d option creates a 4-digit integer.
  sprintf(tempString, "%4d", counter);

  // This will output the tempString to the S7S
  s7s.print(tempString);
  setDecimals(0b00000100);  // Sets digit 3 decimal on

  counter++;  // Increment the counter
  delay(100);  // This will make the display update at 10Hz.
}

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
