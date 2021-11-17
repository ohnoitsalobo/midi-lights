# midi-lights
The original purpose of this code was simply to [light an LED strip in response to MIDI](https://old.reddit.com/r/FastLED/comments/qtk6nd/my_second_midi_keyboard_led_installation/) received on the ESP8266 Serial port using [this schematic](https://upload.wikimedia.org/wikipedia/commons/a/a5/MIDI_IN_OUT_schematic.svg) and the [FortySevenEffects MIDI library](https://github.com/FortySevenEffects/arduino_midi_library).

However, after discovering the [Arduino AppleMIDI library](https://github.com/lathoub/Arduino-AppleMIDI-Library), this functionality can be extended; the libraries can be used to send or receive MIDI over a MIDI connection or a wired / wireless network connection.  
In my case, I can use it to turn my decades-old electric piano (Yamaha CLP-811) into a wireless MIDI keyboard if I so choose.  
Caveats for wireless MIDI: there is noticeable latency and a strong possibility of missed notes. Do not use wireless network MIDI for anything mission-critical.

# Required libraries: 

- various ESP8266 wireless libraries, including ArduinoOTA for wireless reprogramming. Replace with ESP32-specific libraries if using ESP32.

- [FastLED](https://github.com/FastLED/FastLED) for quick'n'easy LED control

- [FortySevenEffects MIDI library](https://github.com/FortySevenEffects/arduino_midi_library) if using with standard 5-pin DIN hardware MIDI

- [Arduino AppleMIDI library](https://github.com/lathoub/Arduino-AppleMIDI-Library) if using with network MIDI

- [VirtuinoCM](https://github.com/iliaslamprou/virtuinoCM) for phone app control 

- [Arduino Gaussian library](https://github.com/ivanseidel/Gaussian) for a couple of functions for smooth LED animation

# Steps to use:

Install all required libraries.

Change WiFi network details in `header.h`

(In order to receive hardware MIDI, use the ESP8266 RX pin `GPIO3` with the schematic linked above)

Change LED output pin and NUM_LEDS in `header.h` (and other LED strip details if you're not using WS2812B)

Change some of the MIDI note details in `MIDI.h` if you are using a larger or a smaller keyboard, or sending a larger range of MIDI over the network.  
The 61-key keyboard I have tested this on uses MIDI notes 36-96 for the full range. If the notes are transposed above or below that range, the LED strip adjusts to include them.

Upload to your device and it should work. On Apple devices it may be automatically detected as a network MIDI device. On Windows you will need to install [rtpMidi](http://www.tobias-erichsen.de/software/rtpmidi.html).

Download the Virtuino Android app [here](https://play.google.com/store/apps/details?id=com.virtuino_automations.virtuino) for wireless control.  
Open the file `src/midi-lights.vrt6` in the Android app and change the IP address to whatever your ESP8266 or ESP32 IP address is.

# Functions

- Device starts up in 'automatic LED patterns' mode, but is listening for input from the MIDI port (either wired MIDI or network)

- LED patterns will change every 60 seconds

- If MIDI is detected, the LED strip lights up in approximate relation to the keys that were pressed.

- If the sustain pedal is used, it triggers a 'firework' effect on the LED strip.