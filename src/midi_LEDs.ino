#if __cplusplus > 199711L  // 
    #define register       // I don't really know why this was needed but
#endif                     // it stops the warning messages about 'register'

#include "header.h" // all the gory libraries and global details go here

void setup(){
    Serial.begin(115200);
    Serial.print("Booting ...");
    
    setupWifi();
    
    setupMIDI();
    
    setupLeds();
    
    Serial.println(". Done!");
}

void loop(){
    runWifi();
    
    runLeds();
}