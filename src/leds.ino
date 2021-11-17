#define TIMES_PER_SECOND(x) EVERY_N_MILLISECONDS(1000/x)
#define TIMES_PER_MINUTE(x) EVERY_N_SECONDS(60/x)
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

#include <Gaussian.h>

FASTLED_USING_NAMESPACE

typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { fireworks, confetti, ripple_blur, fire, cylon, cylon1, sinelon, juggle, bpm, rainbow, rainbowWithGlitter, rainbow_scaling };
uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current

void setupLeds(){
    FastLED.addLeds< LED_TYPE, LED_PINS, COLOR_ORDER >( leds, NUM_LEDS ).setCorrection( TypicalLEDStrip );
    FastLED.setBrightness(currentBrightness);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);
    
    fill_solid (leds, NUM_LEDS, CRGB::Black);
    FastLED.show();

}

void runLeds(){
    EVERY_N_SECONDS(1){
        checkMIDI();
    }
    adjustBrightness();
    EVERY_N_MILLISECONDS(50){ gHue++; gHue1++; gHue2--;}
    
    switch(_mode){
        case _midi:
            runMIDI();
            displayMIDI();
            
        break;
        case _auto:
            EVERY_N_MILLISECONDS( 1000 / 60 ){  // Call the current pattern function once, updating the 'leds' array
                gPatterns[gCurrentPatternNumber]();
            }
            if(auto_advance) {
                EVERY_N_SECONDS( 40 ) { 
                    nextPattern(); 
                }
            }   // change patterns periodically
        break;
        case _manual:
            adjustColors();
        break;
        default:
        break;
    }
    adjustBrightness();
    
    FastLED.show();
    yield();
    
}

void adjustBrightness(){
         if(currentBrightness < _setBrightness) FastLED.setBrightness(++currentBrightness);
    else if(currentBrightness > _setBrightness) FastLED.setBrightness(--currentBrightness);
}
void adjustColors(){
    for(int i = 0; i < NUM_LEDS/2; i++){
        for(int j = 0; j < 3; j++){
                 if(LEFT [i][j] < manualColor_LEFT [j]) LEFT [i][j]++;
            else if(LEFT [i][j] > manualColor_LEFT [j]) LEFT [i][j]--;
                 if(RIGHT[i][j] < manualColor_RIGHT[j]) RIGHT[i][j]++;
            else if(RIGHT[i][j] > manualColor_RIGHT[j]) RIGHT[i][j]--;
        }
    }
}

void nextPattern(){
    currentBrightness = 0;
    uint8_t temp = 0;
    temp = ARRAY_SIZE( gPatterns );
    gCurrentPatternNumber = (gCurrentPatternNumber + 1) % temp;
} // advance to the next pattern

void previousPattern(){
    currentBrightness = 0;
    uint8_t temp = 0;
    temp = ARRAY_SIZE( gPatterns );
    gCurrentPatternNumber = (gCurrentPatternNumber + (temp-1)) % temp;
} // advance to the previous pattern

void rainbow() {
    // FastLED's built-in rainbow generator
    fill_rainbow( leds, NUM_LEDS, gHue);
} // rainbow

void rainbowWithGlitter() {
    // built-in FastLED rainbow, plus some random sparkly glitter
    rainbow();
    addGlitter();
} // rainbow with glitter

void addGlitter() {
    EVERY_N_MILLISECONDS(1000/30){
        if( random8() < 80) {
            leds[ random16(NUM_LEDS) ] += CRGB::White;
        }
    }
}

void rainbow_scaling(){
    for(int i = 0; i <= NUM_LEDS/2; i++){
        RIGHT [i] = CHSV((millis()/77*i+1)%255 + gHue1, 255, 255);
        int j = NUM_LEDS/2-1-i;
        LEFT[j] = RIGHT[i];
    }
} // rainbow scaling

void cylon(){
    float width = 2.0f, res = 3.0f;
    static Gaussian _cylon(0, width*width);
    // float pos = beatsin16(_BPM/2, width*5, (NUM_LEDS-width)*5);
    // _cylon.mean = pos/5.0;
    float pos = beatsin16(_BPM/2, width*res, (NUM_LEDS-1-width)*res);
    for (int i = 0; i < NUM_LEDS; i++){
        float _val;
        _cylon.mean = pos/res;
        _val = _cylon.plot(i)*(width/0.399);
        leds[i] = CHSV(0, 255, _val*255);
    }
    yield();
}


uint8_t fadeval = 235, frameRate = 20; // 45
void fire(){ // my own simpler 'fire' code - randomly generate fire and move it up the strip while fading
    EVERY_N_MILLISECONDS(1000/frameRate){
        uint8_t _hue = 0, _sat = 255, _val = 0;                       // 
        _val = random(100, 255);                                      // generate a random brightness value between 100-255 (never zero)
        _sat = 255 - (_val/255.0 * 60);                               // brighter = less saturated / more white-ish
        _hue = _val/255.0 *_val/255.0 * 55;                           // keep hue in the red/yellow/orange range, but nonlinear scaling
        RIGHT[NUM_LEDS/2-1] = CHSV( _hue, _sat, _val*_val/255);       // LEDs in the center get this new random color
        LEFT [0] = RIGHT[NUM_LEDS/2-1];                               // 
    }
    for(int i = 0; i < NUM_LEDS/2-1; i++){
        RIGHT[i] = RIGHT[i+1].nscale8(fadeval); // shift the color to the next LED, dim the brightness very slightly,
        if(RIGHT[i].g > 0) RIGHT[i].g--;        // and reduce green in particular to fade the yellow faster than the red
        int j = NUM_LEDS/2-1-i;
        LEFT [j] = RIGHT[i];
    }
        yield();
}

void fireworks(){
    const int sparks = 15;
    static FireWork _firework[sparks];
    fadeToBlackBy(leds, NUM_LEDS, 100);
    for (int i = 0; i < sparks; i++){
        _firework[i].draw();
    }
    EVERY_N_SECONDS(2){
        if(random8() < 150){
            int temp = random16(NUM_LEDS/8, 7*NUM_LEDS/4);
            for (int i = 0; i < sparks; i++){
                // _pos = NUM_LEDS/2 + random16(0, (NUM_LEDS/2-1)*3.0f)/3.0f;
                _firework[i].init(temp);
            }
        }
    }
}

void confetti() 
{    // random colored speckles that blink in and fade smoothly
    EVERY_N_MILLISECONDS(1000/40){
        fadeToBlackBy( leds, NUM_LEDS, 20);
        if(random8() < 50){
            int pos = random16(NUM_LEDS);
            // leds[pos] += CHSV( random8(255), 255, 255);
            leds[pos] += CHSV( gHue + random8(64), 190+random8(65), 255);
        }
    }
}

uint8_t blurval = 100;
void ripple_blur(){ // randomly drop a light somewhere and blur it using blur1d
    EVERY_N_MILLISECONDS(1000/30){
        blur1d(leds, NUM_LEDS, blurval);
    }
    EVERY_N_MILLISECONDS(30){
        if( random8() < 15) {
            uint8_t pos = random(NUM_LEDS/2);
            LEFT [pos] = CHSV(random(0, 64)+gHue1, random(250, 255), 255);
        }
        if( random8() < 15) {
            uint8_t pos = random(NUM_LEDS/2);
            RIGHT [pos] = CHSV(random(0, 64)-gHue2, random(250, 255), 255);
        }
    }
}

double temp = 5.0; // set this for the "resolution" of the bell curve temp*NUM_LEDS
double width = 10; // set this for the "width" of the bell curve (how many LEDs to light)
double _smear = 4.0*sqrt(1.0/(width*width*width)); // this calculates the necessary coefficient for a width of w
void cylon1(){
    EVERY_N_MILLISECONDS( _BPM   ) { gHue1++; }
    EVERY_N_MILLISECONDS( _BPM-2 ) { gHue2--; }
    nscale8( leds, NUM_LEDS, 20);
    int posL, posR, val;
    posL = beatsin16(_BPM/2  , 0.9*width*temp, (NUM_LEDS-0.9*width)*temp);  // range of input. the 'w' prevents it from hitting the sides
    posR = beatsin16(_BPM/2+1, 0.9*width*temp, (NUM_LEDS-0.9*width)*temp);  // range of input. the 'w' prevents it from hitting the sides
    double a;
    double scaledposL = posL/(NUM_LEDS*temp) * NUM_LEDS/2; // range scaled down to working length
    double scaledposR = posR/(NUM_LEDS*temp) * NUM_LEDS/2; // range scaled down to working length
    for(int i = 0; i < NUM_LEDS/2; i++){
        a = i-scaledposL;       // 
        a = -_smear*a*a;        // generate bell curve with coefficient calculated above for chosen width
        val = 255.0*pow(2, a);  // 
        LEFT [i] |= CHSV(gHue2, 255, val);
        a = i-scaledposR;       // 
        a = -_smear*a*a;        // generate bell curve with coefficient calculated above for chosen width
        val = 255.0*pow(2, a);  // 
        RIGHT[i] |= CHSV(gHue1, 255, val);
    }
    yield();
}

void juggle() {
    // colored dots, weaving in and out of sync with each other
    fadeToBlackBy( leds, NUM_LEDS, 30);
    byte dothue = 0;
    for( int i = 0; i < 6; i++) {
        float _pos = beatsin16(i+7,0,NUM_LEDS-1);
        // leds[beatsin16(i+7,0,NUM_LEDS-1)] |= CHSV(dothue, 200, 255);
        DrawPixels(_pos, 1.5, CHSV(dothue, 200, 255));
        dothue += 32;
        yield();
    }
}

void bpm()
{
    // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
    // uint8_t BeatsPerMinute = 62;
    uint8_t BeatsPerMinute = _BPM;
    // CRGBPalette16 palette = PartyColors_p;
    CRGBPalette16 palette = RainbowColors_p;
    uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
    for( int i = 0; i < NUM_LEDS; i++) { //9948
        leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
        yield();
    }
}

void blendwave() {
    CRGB clr1, clr2;
    uint8_t speed, loc1;

    speed = beatsin8(6,0,255);

    clr1 = blend(CHSV(beatsin8(3,0,255),255,255), CHSV(beatsin8(4,0,255),255,255), speed);
    clr2 = blend(CHSV(beatsin8(4,0,255),255,255), CHSV(beatsin8(3,0,255),255,255), speed);
    loc1 = beatsin8(13,0,NUM_LEDS-1);

    fill_gradient_RGB(leds, 0, clr2, loc1, clr1);
    fill_gradient_RGB(leds, loc1, clr2, NUM_LEDS-1, clr1);
    
    yield();
} // blendwave()

void sinelon()
{
    // a colored dot sweeping back and forth, with fading trails
    fadeToBlackBy( leds, NUM_LEDS, 20);
    float pos = beatsin16(_BPM/3, 0, (NUM_LEDS-1)*5.0)/5.0;
    DrawPixels(pos, 1.5, ColorFromPalette(RainbowColors_p, pos, 255, LINEARBLEND));
    // leds [pos] = ColorFromPalette(RainbowColors_p, pos, 255, LINEARBLEND);   // Use that value for both the location as well as the palette index colour for the pixel.
    yield();
}

CRGB ColorFraction(CRGB colorIn, float fraction){
    fraction = min(1.0f, fraction);
    return CRGB(colorIn).fadeToBlackBy(255 * (1.0f - fraction));
}

void DrawPixels(float fPos, float count, CRGB color){
    // Calculate how much the first pixel will hold
    float availFirstPixel = 1.0f - (fPos - (long)(fPos));
    float amtFirstPixel = min(availFirstPixel, count);
    float remaining = min(count, FastLED.size()-fPos);
    int iPos = fPos;

    // Blend (add) in the color of the first partial pixel

    if (remaining > 0.0f){
        leds[iPos++] += ColorFraction(color, amtFirstPixel);
        remaining -= amtFirstPixel;
    }

    // Now draw any full pixels in the middle

    while (remaining > 1.0f){
        leds[iPos++] += color;
        remaining--;
    }

    // Draw tail pixel, up to a single full pixel

    if (remaining > 0.0f){
        leds[iPos] += ColorFraction(color, remaining);
    }
}