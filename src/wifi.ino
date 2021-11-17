void setupWifi(){
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    if (MDNS.begin(hostname.c_str())) {
        Serial.print("\r\nMDNS responder started, hostname: ");
        Serial.println(hostname);
    }
    MDNS.addService("http", "tcp", 80);
    
    setupOTA();
    
    setupVirtuino();

}

void runWifi(){
    if(WiFi.status() == WL_CONNECTED){
        ArduinoOTA.handle();
        virtuinoRun();        // Necessary function to communicate with Virtuino. Client handler
    }
    if(WiFi.status() != WL_CONNECTED){
        EVERY_N_SECONDS(10){
            WiFi.begin(ssid, password);
        }
    }
    yield();
}

void setupOTA(){
    ArduinoOTA.setHostname(hostname.c_str());
    
    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else { // U_FS
            type = "filesystem";
        }

        // NOTE: if updating FS this would be the place to unmount FS using FS.end()
        Serial.println("Start updating " + type);
        FastLED.setBrightness(100);
        FastLED.clear();
        FastLED.show();
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
        FastLED.clear();
        FastLED.show();
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        int temp = progress / (total / 100);
        Serial.printf("Progress: %u%%\r", temp);
        leds[map(temp, 0, 100, 0, NUM_LEDS)] = 0x111111;
        FastLED.show();
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
            Serial.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
            Serial.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
            Serial.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
            Serial.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
            Serial.println("End Failed");
        }
    });
    ArduinoOTA.begin();
}
