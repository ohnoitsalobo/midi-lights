#define wireless_MIDI 1    // If BOTH are enabled, wireless input is disabled as hardware MIDI is more reliable. However, received notes will be mirrored on the network conection.
#define wired_MIDI 0       // 

uint32_t launchFirework = 0;
void checkMIDI(){
    if((_mode == _midi) && (millis() - timeSinceLastMIDI > 10000)){     // if no MIDI detected for 10 seconds
        _mode = _auto;                                                  //     switch to auto mode
        _setBrightness = 200*200/255.0;                                 //
    }else if((_mode != _midi) && (MIDI_W.read())){                      // if MIDI detected
        _mode = _midi;                                                  //     switch to MIDI mode
        FastLED.clear();
        _setBrightness = 255;
    }
}

void setupMIDI(){
#if wired_MIDI                                       // MIDI event handler functions for hardware MIDI connection
    MIDI.setHandleNoteOn(handleNoteOn);                 //         
    MIDI.setHandleNoteOff(handleNoteOff);               // 
    MIDI.setHandlePitchBend(handlePitchBend);           // 
    MIDI.setHandleControlChange(handleControlChange);   // 
    MIDI.setHandleClock(handleClock);                   // 
                                                        // 
    MIDI.begin(MIDI_CHANNEL_OMNI);                      // 
#endif

#if wireless_MIDI
    AppleMIDI_W.setHandleConnected([](const APPLEMIDI_NAMESPACE::ssrc_t & ssrc, const char* name) {
        isConnected++;
        DBG(F("Connected to session"), ssrc, name);
    });
    AppleMIDI_W.setHandleDisconnected([](const APPLEMIDI_NAMESPACE::ssrc_t & ssrc) {
        isConnected--;
        DBG(F("Disconnected"), ssrc);
    });

#if !wired_MIDI                                           // MIDI event handler functions for wireless MIDI connection
    MIDI_W.setHandleNoteOn(handleNoteOn);                 //    ONLY IF wired MIDI is disabled
    MIDI_W.setHandleNoteOff(handleNoteOff);               //    to avoid confusion
    MIDI_W.setHandlePitchBend(handlePitchBend);           // 
    MIDI_W.setHandleControlChange(handleControlChange);   // 
    MIDI_W.setHandleClock(handleClock);                   // 
#endif

    MIDI_W.begin(MIDI_CHANNEL_OMNI);
#endif
    
    for (int i = 0; i < 127; i++){
        midiNotes[i] = 0;
    }
}

void runMIDI(){
#if wired_MIDI
    MIDI.read();
#endif

#if wireless_MIDI
#if !wired_MIDI
    MIDI_W.read();
#endif
#endif
    yield();

}

const int sparks = 20;
FireWork _firework[sparks];
const uint8_t min_min = 0, max_min = 36, min_max = 96, max_max = 127;
uint8_t _min = max_min, _max = min_max;

void displayMIDI(){
    EVERY_N_MILLISECONDS(20){ 
        fadeToBlackBy( leds, NUM_LEDS, ( sustain ? 10 : 20) );
    }

    if(_lastPressed < _min) { _min = _lastPressed; }    // adjust note range in case octaves are changed
    if(_lastPressed > _max) { _max = _lastPressed; }    // 
                                                        // 
    float factor = 1.0f * (NUM_LEDS-1)/(_max-_min);     // 
    for (int i = 0; i < 127; i++){
        float _pos = factor*(_lastPressed-_min);
        DrawPixels(_pos, 2, CHSV(map(_pos, 0, NUM_LEDS-1, 0, 224)+gHue, 255-midiNotes[_lastPressed], midiNotes[_lastPressed]/127.0*255)); // thanks to https://github.com/davepl/DavesGarageLEDSeries/blob/e725a11c81bd9bf3aa74da791622979a482d5425/LED%20Episode%2009/src/main.cpp#L50
        midiNotes[_lastPressed] = 0;
    }
    EVERY_N_MILLISECONDS(1000/5){
        if (_min < max_min) _min++;
        if (_max > min_max) _max--;
    }
    if(millis() - launchFirework < 1000){
        fadeToBlackBy(leds, NUM_LEDS, 80);
        for (int i = 0; i < sparks; i++){
            _firework[i].draw();
        }
    }
}

// do X when a key is pressed
void handleNoteOn(byte channel, byte pitch, byte velocity) {
    midiNotes[pitch] = velocity;
    _lastPressed = pitch;
    timeSinceLastMIDI = millis();
    
#if wireless_MIDI
    MIDI_W.sendNoteOn(pitch, velocity, channel);
#endif
    yield();
}

// do X when a key is released
void handleNoteOff(byte channel, byte pitch, byte velocity) {
    midiNotes[pitch] = 0;

#if wireless_MIDI
    MIDI_W.sendNoteOff(pitch, velocity, channel);
#endif
}


// do X when pitch bend is used
void handlePitchBend(byte channel, int bend) {
    // fill strip with solid color based on pitch bend amount
    fill_solid(leds, NUM_LEDS, CHSV(map(bend, -8192, 8192, 0, 224), 255, 125)); // 0  8192  16383
}

// do X when control channels are used
void handleControlChange(byte channel, byte number, byte value){
    // channel 1 = modulation
    if( number == 1 ){
        fill_solid( leds, NUM_LEDS, 0x222222 );
    }
    // channel 64 = damper / sustain pedal
    if( number == 64 ){
        if( value >= 64 ){
            launchFirework = millis();
            for (int i = 0; i < sparks; i++){
                _firework[i].init(random16(NUM_LEDS/3, 2*NUM_LEDS/3));
            }
            sustain = true;
        } else {
            sustain = false;
        }
    }
}

void handleClock() {
    
    if(ticks < 24){
        if(ticks == 0){
            lastClock = millis();
        }
        ticks++;
    }
    else{
        ticks = 0;
        currentClock = millis();
        
        uint16_t temp = currentClock - lastClock;
        _BPM = (1000.0 / temp) * 60.0;
        // Serial.print("Quarter note: ");
        // Serial.print(temp);
        // Serial.print(" ms\tFrequency: ");
        // Serial.print(1000.0 / temp);
        // Serial.print(" Hz\tBPM: ");
        // Serial.println(_BPM);
    }
}

// ====================================================================================
// Event handlers for incoming MIDI messages
// ====================================================================================

/*
    inline void setHandleMessage(void (*fptr)(const MidiMessage&)) { mMessageCallback = fptr; };
    inline void setHandleError(ErrorCallback fptr) { mErrorCallback = fptr; }
    inline void setHandleNoteOff(NoteOffCallback fptr) { mNoteOffCallback = fptr; }
    inline void setHandleNoteOn(NoteOnCallback fptr) { mNoteOnCallback = fptr; }
    inline void setHandleAfterTouchPoly(AfterTouchPolyCallback fptr) { mAfterTouchPolyCallback = fptr; }
    inline void setHandleControlChange(ControlChangeCallback fptr) { mControlChangeCallback = fptr; }
    inline void setHandleProgramChange(ProgramChangeCallback fptr) { mProgramChangeCallback = fptr; }
    inline void setHandleAfterTouchChannel(AfterTouchChannelCallback fptr) { mAfterTouchChannelCallback = fptr; }
    inline void setHandlePitchBend(PitchBendCallback fptr) { mPitchBendCallback = fptr; }
    inline void setHandleSystemExclusive(SystemExclusiveCallback fptr) { mSystemExclusiveCallback = fptr; }
    inline void setHandleTimeCodeQuarterFrame(TimeCodeQuarterFrameCallback fptr) { mTimeCodeQuarterFrameCallback = fptr; }
    inline void setHandleSongPosition(SongPositionCallback fptr) { mSongPositionCallback = fptr; }
    inline void setHandleSongSelect(SongSelectCallback fptr) { mSongSelectCallback = fptr; }
    inline void setHandleTuneRequest(TuneRequestCallback fptr) { mTuneRequestCallback = fptr; }
    inline void setHandleClock(ClockCallback fptr) { mClockCallback = fptr; }
    inline void setHandleStart(StartCallback fptr) { mStartCallback = fptr; }
    inline void setHandleTick(TickCallback fptr) { mTickCallback = fptr; }
    inline void setHandleContinue(ContinueCallback fptr) { mContinueCallback = fptr; }
    inline void setHandleStop(StopCallback fptr) { mStopCallback = fptr; }
    inline void setHandleActiveSensing(ActiveSensingCallback fptr) { mActiveSensingCallback = fptr; }
    inline void setHandleSystemReset(SystemResetCallback fptr) { mSystemResetCallback = fptr; }

    inline void disconnectCallbackFromType(MidiType inType);
    
    void handleNoteOff(byte channel, byte note, byte velocity);
    void handleNoteOn(byte channel, byte note, byte velocity);
    void handleAfterTouchPoly(byte channel, byte note, byte pressure);
    void handleControlChange(byte channel, byte number, byte value);
    void handleProgramChange(byte channel, byte number);
    void handleAfterTouchChannel(byte channel, byte pressure);
    void handlePitchBend(byte channel, int bend);
    void handleSystemExclusive(byte* array, unsigned size);
    void handleTimeCodeQuarterFrame(byte data);
    void handleSongPosition(unsigned int beats);
    void handleSongSelect(byte songnumber);
    void handleTuneRequest(void);
    void handleClock(void);
    void handleStart(void);
    void handleContinue(void);
    void handleStop(void);
    void handleActiveSensing(void);
    void handleSystemReset(void);
*/

