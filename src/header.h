#define wifissid "SSID"
#define wifipassword "PASSWORD"
#define host_name "midi-leds"

#include <ESP8266WiFi.h>               // 
#include <ESP8266mDNS.h>               // 
#include <WiFiUdp.h>                   // necessary WIFI stuff
#include <ArduinoOTA.h>                // 
const char* ssid = wifissid;             // wifi SSID
const char* password = wifipassword;     // wifi password

String hostname = host_name;          // device name on mDNS and wireless MIDI


// #define FASTLED_ALLOW_INTERRUPTS 0
// #define FASTLED_INTERRUPT_RETRY_COUNT 1
#define FASTLED_INTERNAL        // suppress #pragma messages
#include <Adafruit_I2CDevice.h>
#include <FastLED.h>

#define LED_TYPE    WS2812B    // LED strip type
#define COLOR_ORDER GRB        // order of color in data stream
#define LED_PINS    13         // data output pin
#define NUM_LEDS    56         // number of LEDs in strip
#define BRIGHTNESS  255
// scale the brightness non-linearly which looks better because our eyes detect light intensity logarithmically
CRGBArray<NUM_LEDS> _leds_;
CRGBSet leds(_leds_, NUM_LEDS);
CRGBSet RIGHT (leds (0,            NUM_LEDS/2-1) );  // < subset containing only right LEDs
CRGBSet LEFT  (leds (NUM_LEDS/2,   NUM_LEDS-1)   );  // < subset containing only left  LEDs
uint8_t _hue = 0;             // modifier for color cycling
uint8_t gHue = 0, gHue1 = 0, gHue2 = 0; // rotating "base color" used by many of the patterns
CRGB manualColor = 0x000000, manualColor_LEFT = 0x000000, manualColor_RIGHT = 0x000000;
CHSV manualHSV (0, 255, 255);
uint8_t currentBrightness = BRIGHTNESS, _setBrightness = BRIGHTNESS;
bool auto_advance = true;
#include "fireworks.h"

enum mode_select {   // self-explanatory
    _midi,              // midi responsiveness mode
    _auto,              // automatic patterns mode
    _manual             // manual control mode
};
mode_select _mode = _auto; // initialize with MIDI control


#include <MIDI.h>
MIDI_CREATE_INSTANCE(HardwareSerial, Serial,  MIDI);   // create MIDI instance
// MIDI_CREATE_DEFAULT_INSTANCE();

#include <AppleMIDI.h>                                                               
// create and name wireless-MIDI instance explicitly or else it will try to use the default name "MIDI" which is used by hardware MIDI
APPLEMIDI_CREATE_INSTANCE(WiFiUDP, MIDI_W, hostname.c_str(), DEFAULT_CONTROL_PORT);  // APPLEMIDI_CREATE_DEFAULTSESSION_INSTANCE();
unsigned long t0 = millis();
int8_t isConnected = 0;  // counts how many wireless MIDI clients are connected
uint8_t midiNotes [127];
uint8_t _lastPressed;             // holder for last-detected key color
CRGB lastPressed;             // holder for last-detected key color
bool sustain = false;         // is sustain pedal on?

uint32_t lastClock = 0, currentClock = 0, _BPM = 60;  // counter to detect tempo
byte ticks = 0;                                       // 24 ticks per 1/4 note "crotchet"

uint32_t timeSinceLastMIDI = 0; // amount of time since the last MIDI input was detected
                                // used to switch to automatic patterns when no input

////////////////////////////  VIRTUINO FUNCTIONS  //////////////////////////////
// Download Android app here:     https://play.google.com/store/apps/details?id=com.virtuino_automations.virtuino
// Download Arduino library here: https://github.com/iliaslamprou/virtuinoCM

// I used to use Blynk, but 
// 1) Blynk has stopped supporting their local-server option and moved to a subscription model
// 2) virtuino requires no external server - direct communication to device, which Blynk did not support

#include <VirtuinoCM.h>
VirtuinoCM virtuino;               
#define V_memory_count 32          // the size of V memory. You can change it to a number <=255)
float V[V_memory_count];           // This array is synchronized with Virtuino V memory. You can change the type to int, long etc.
float V_prev[V_memory_count];
// #define debug 1
WiFiServer virtuinoServer(8000);                   // Default Virtuino Server port 


////////////////////////////  END VIRTUINO FUNCTIONS  //////////////////////////////

float map_float(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}