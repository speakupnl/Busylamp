//
// Arduino Nano USB desktop busylight
//

/* Application notes
 * The Nano is not capable of emulating HID (keyboard)
 */
 
// Define connections
#define ONBOARD_LED     LED_BUILTIN
#define BUTTON1         2
#define DEBOUNCE        80     // milliseconds to wait for debouncing

#define PIXELS          5
#define NUMPIXELS       4
#define BRIGHTNESS      50     // 0-255
#define SPEED           100     // For transitions

#define PXL_RED       0xFF0000
#define PXL_GREEN     0x00FF00
#define PXL_BLUE      0x0000FF
#define PXL_MAGENTA   0x990099

// FX
#define PXL_ORANGE    0xFF4500
#define PXL_SOFTORANGE    0x0C0300  // 5%

#define STATE_AVAILABLE 0
#define STATE_BUSY      1
#define STATE_MEETING   2
#define STATE_WAVE      3

#include <Adafruit_NeoPixel.h>    // this lib is pretty big, may need to look for something more lightweight https://blog.podkalicki.com/attiny13-controlling-leds-ws2811ws2812/ perhaps?

// State
int state = STATE_AVAILABLE;

// Buttons
byte button1_state;
byte button2_state;
unsigned long debouncetimer;

// Serial comm
int serialbyte = 0;
String serialcmd = "";

// FX
byte pos;

// Pixels
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, PIXELS, NEO_GRB + NEO_KHZ800);
unsigned long ledtimer;

void setup() {
  pinMode(ONBOARD_LED, OUTPUT);
  pinMode(BUTTON1, INPUT_PULLUP);  

  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  // Banner
  Serial.println("BUSYLAMP NANO - Booting");   // send an initial string
  
  // Give a visual indication of program start - flash a few times
  for (int i = 0; i < 3; i++) {
    digitalWrite(ONBOARD_LED, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(100);               // wait for a second
    digitalWrite(ONBOARD_LED, LOW);    // turn the LED off by making the voltage LOW
    delay(100);
  }

  // initialize WS2812 LED pixels
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  setAVAILABLE();
}

void processcommand(String cmd) {
  if(cmd.substring(0,1).equalsIgnoreCase("A")) {  // A for AVAILABLE (shortest combo possible)
    // Set state to AVAILABLE
    setAVAILABLE();
  } else if(cmd.substring(0,1).equalsIgnoreCase("B")) { // B for BUSY (shortest combo possible)
    // Set state to BUSY
    setBUSY();
  } else if(cmd.substring(0,1).equalsIgnoreCase("M")) { // M for MEETING (shortest combo possible)
    // Set state to MEETING
    setMEETING();
  } else if(cmd.substring(0,3).equalsIgnoreCase("\\O/")) { // W for WAVE (shortest combo possible)
    // Set state to WAVE
    setWAVE();

  // Fallthrough
  } else {
    Serial.println("ERR - Unhandled input: " + cmd);
  }
}

void setAVAILABLE() {
  digitalWrite(ONBOARD_LED, LOW);
  strip.fill(PXL_GREEN, 0, NUMPIXELS);
  strip.show();
  Serial.println("OK - AVAILABLE");  
  state = STATE_AVAILABLE;
}

void setBUSY() {
  digitalWrite(ONBOARD_LED, HIGH);
  strip.fill(PXL_RED, 0, NUMPIXELS);
  strip.show();
  Serial.println("OK - BUSY");  
  state = STATE_BUSY;
}

void setMEETING() {
  digitalWrite(ONBOARD_LED, HIGH);
  strip.fill(PXL_MAGENTA, 0, NUMPIXELS);
  strip.show();
  Serial.println("OK - MEETING");  
  state = STATE_MEETING;
}

void setWAVE() {
  digitalWrite(ONBOARD_LED, HIGH);
  Serial.println("OK - WAVE");
  strip.setBrightness(255);
  pos = 0;
  state = STATE_WAVE;
  LEDstep();
}

void LEDstep() {  
  if(state == STATE_WAVE) {
    strip.fill(0, 0, NUMPIXELS);
    strip.fill(PXL_SOFTORANGE, pos, 1);
    int nxt = pos + 1;
    if(nxt > NUMPIXELS-1) nxt = 0;
    strip.fill(PXL_ORANGE, nxt, 1);
    nxt = nxt + 1;
    if(nxt > NUMPIXELS-1) nxt = 0;
    strip.fill(PXL_SOFTORANGE, nxt, 1);
    strip.show();
  }
  pos++;
  if(pos > NUMPIXELS-1) pos = 0;
} 

void loop() {

  // Check for serial input
  while(Serial.available() > 0) {
    serialbyte = Serial.read();// read the incoming data byte
    if(((serialbyte==10) || (serialbyte==13)) && (serialcmd!="")) {
      // Process a complete command
      processcommand(serialcmd);
      serialcmd = "";
    } else {
      serialcmd += char(serialbyte);
    }
  }

  // Check for button activity
  if (digitalRead(BUTTON1) == LOW && button1_state == 0) {
    // check if enough time has passed to consider it a switch press
    if ((millis() - debouncetimer) > DEBOUNCE) {
      button1_state=1;
      if(state != STATE_BUSY) {
        setBUSY();
      } else {
        setAVAILABLE();
      }
      debouncetimer = millis();
    }
  } else {
    if (button1_state == 1 && digitalRead(BUTTON1) == HIGH){
      button1_state=0;
    }
  }

  // Perform LED transition if needed
  if ((millis() - ledtimer) > SPEED) {
    LEDstep();
    ledtimer = millis();  
  }  

}
