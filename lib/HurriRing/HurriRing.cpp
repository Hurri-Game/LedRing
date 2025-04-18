#include <HurriRing.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

//{"state":"STATIC","parameter":[{"color":"255,0,0"}]}

// Forward declaration of HurriRing class if needed
class HurriRing;

// Move HurriRingCallbacks after HurriRing class is fully defined
class HurriRingCallbacks : public BLECharacteristicCallbacks {
private:
    HurriRing* ring;
    std::string buffer; 
public:
    HurriRingCallbacks(HurriRing* ring) : ring(ring) {}


    void onWrite(BLECharacteristic* pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        
        // Debug print the received chunk
        Serial.print("Received chunk: ");
        Serial.println(value.c_str());
        Serial.print("Chunk length: ");
        Serial.println(value.length());

        // Append the new chunk to our buffer
        buffer += value;

        // Check if we have a complete JSON message (ends with '}')
        if (!buffer.empty() && buffer.back() == '}') {
            Serial.print("Complete message: ");
            Serial.println(buffer.c_str());
            Serial.print("Total length: ");
            Serial.println(buffer.length());

            // Process the complete message
            ring->handleBleCommand((uint8_t*)buffer.c_str(), buffer.length());
            
            // Clear the buffer for the next message
            buffer.clear();
        }
    }
};

const char* HurriRing::SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
const char* HurriRing::CHARACTERISTIC_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8";

HurriRing* HurriRing::instance = nullptr;

HurriRing::HurriRing() : hurriRingLeds(HURRI_RING_LED_COUNT, HURRI_RING_PIN, NEO_GRB + NEO_KHZ800) {
    HurriRing::instance = this;
}

void HurriRing::init(const char* deviceName) {
    this->hurriRingLeds.begin();
    this->hurriRingLeds.show();
    this->hurriRingLeds.setBrightness(this->brightness);

    // Initialize BLE
    BLEDevice::init(deviceName);
    BLEServer *pServer = BLEDevice::createServer();
    
    // Create the BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Create BLE Characteristic
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );

    pCharacteristic->setCallbacks(new HurriRingCallbacks(this));
    pCharacteristic->addDescriptor(new BLE2902());

    // Start the service
    pService->start();

    // Start advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    // Initial animation to show device is ready
    for(uint8_t i = 0; i <= 2; i++) {
        this->_pulse(1, HurriRing::GREEN);
    }
    this->_setColor(HurriRing::BLACK, this->brightness);
}

void HurriRing::loop() {
    switch (this->_state) {
        case HurriRing::State::IDLE:
            this->_setColor(HurriRing::BLACK, this->brightness);
            break;

        case HurriRing::State::RAINBOW:
            this->_theaterChaseRainbow(5);
            break;

        case HurriRing::State::RAINBOW_WIPE:
            this->_rainbowWipe(5);
            break;

        case HurriRing::State::FREEZE:
            break;

        case HurriRing::State::PULSE:
            this->_pulse(5,this->colorParam);
            break;

        case HurriRing::State::ROULETTE:
            this->_roulette(this->colorParam);
            break;

        case HurriRing::State::RANDOM_NUMBER:
            this->_randomNumber(this->numberParam, this->colorParam);
            break;

        case HurriRing::State::STATIC:
            this->_static(this->colorParam);
            break;

        case HurriRing::State::SHOW_SECTION:
            this->_showSection(this->numberParam, this->colorParam);
            break;

        case HurriRing::State::SHUFFLE_SECTIONS:
            this->_shuffleSections();
            break;

        case HurriRing::State::BRIGHTNESS:
            this->setBrightness(this->numberParam);
            break;

        default:
            this->_setColor(HurriRing::BLACK, this->brightness);
            break;
    }
}
    
void HurriRing::setState(HurriRing::State state){
    this->_printd("Functioncall: setState \n");
    this->_state = state;
}

void HurriRing::setBrightness(uint8_t brightness){
    this->brightness = brightness;
}

void HurriRing::setDebugFlag(bool debugEnable){
    this->_printd("Functioncall: setDebugFlag \n");
    this->debugEnabled = debugEnable;
    if(this->debugEnabled && !Serial){
        Serial.begin(9600);
    }
}

HurriRing::State HurriRing::_stringToState(const std::string& stateStr) {
    this->_printd("Functioncall: _stringToState \n");
    auto it = stringToStateMap.find(stateStr);
    if (it != stringToStateMap.end()) {

        this->_printd("State: %d", it->second);
        return it->second;
    }
    return this->_state;
}

template<typename... Args>
void HurriRing::_printd(const char *msg, Args... args) {
    if (this->debugEnabled){
        Serial.printf(msg, args...);
    }
    else {
        (void)msg;
    }
}

void HurriRing::_printd(const char *msg) {
    if (this->debugEnabled){
        Serial.printf(msg);
    }
    else {
        (void)msg;
    }
}

void HurriRing::_theaterChaseRainbow(int wait){
    if(this->_stateChange) this->_printd("Functioncall: _theaterChaseRainbow \n");
    this->_stateChange = false;
    int firstPixelHue = 0;     // First pixel starts at red (hue 0)
    for(int a=0; a<30; a++) {  // Repeat 30 times...
        for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
        this->hurriRingLeds.clear();         //   Set all pixels in RAM to 0 (off)
        // 'c' counts up from 'b' to end of strip in increments of 3...
        for(int c=b; c<this->hurriRingLeds.numPixels(); c += 3) {
            // hue of pixel 'c' is offset by an amount to make one full
            // revolution of the color wheel (range 65536) along the length
            // of the strip (strip.numPixels() steps):
            int      hue   = firstPixelHue + c * 65536L / this->hurriRingLeds.numPixels();
            uint32_t color = this->hurriRingLeds.gamma32(this->hurriRingLeds.ColorHSV(hue)); // hue -> RGB
            this->hurriRingLeds.setPixelColor(c, color); // Set pixel 'c' to value 'color'
        }
        this->hurriRingLeds.show();                // Update strip with new contents
        delay(wait);                 // Pause for a moment
        firstPixelHue += 65536 / 90; // One cycle of color wheel over 90 frames
        }
    }  
}

void HurriRing::_setColor(uint32_t color, uint8_t brightness, bool once){
    if(this->_stateChange) this->_printd("Functioncall: _setColor \n");
    if(!once) this->_stateChange = false;
    uint8_t red = (color >> 16) & 0xFF;
    uint8_t green = (color >> 8) & 0xFF;
    uint8_t blue = color & 0xFF;
    uint32_t adjustedColor = this->hurriRingLeds.Color(red * brightness / 255, green * brightness / 255, blue * brightness / 255);
    for (int i = 0; i < this->hurriRingLeds.numPixels(); i++) {
        this->hurriRingLeds.setPixelColor(i, adjustedColor);
    }    
    this->hurriRingLeds.show();
}

void HurriRing::_fadeOut(uint8_t stepDelay) {
    this->_printd("Functioncall: _fadeOut \n");
    bool allLedsOff = true;

    for (uint8_t brightness = 255; brightness > 0; brightness--) {
        allLedsOff = true;
        for (uint16_t i = 0; i < hurriRingLeds.numPixels(); i++) {
            uint32_t color = hurriRingLeds.getPixelColor(i);
            uint8_t r = (color >> 16) & 0xFF;
            uint8_t g = (color >> 8) & 0xFF;
            uint8_t b = color & 0xFF;

            // Scale the color components based on the brightness
            r = (r * brightness) / 255;
            g = (g * brightness) / 255;
            b = (b * brightness) / 255;
            hurriRingLeds.setPixelColor(i, r, g, b);

            if (r > 0 || g > 0 || b > 0){
                allLedsOff = false;
            }
        }
        this->hurriRingLeds.show();
        delay(uint8_t(stepDelay));

        if(allLedsOff){
            break;
        }
    }
}

void HurriRing::_pulse(int wait, uint32_t color){
    static int pulseBrightness = 0;
    static bool directionToggle = true;
    if (this->_stateChange) {
        this->_printd("Functioncall: _pulse \n");
        pulseBrightness = 255;
        directionToggle = true;
    }
    if(directionToggle){
        pulseBrightness--;
        if(pulseBrightness == 0){
            directionToggle = false;
        }
    } else {
        pulseBrightness++;
        if(pulseBrightness == 255){
            directionToggle = true;
        }
    }
    this->_setColor(color, pulseBrightness);
    delay(int(wait*255/this->brightness));
}

void HurriRing::_rainbowWipe(int wait) {
    if(this->_stateChange) this->_printd("Functioncall: _rainbowWipe \n");
    this->_stateChange = false;
    int firstPixelHue = 0;     // First pixel starts at red (hue 0)
    for(int i=0; i<this->hurriRingLeds.numPixels(); i++) { // For each pixel in strip...
        int hue   = firstPixelHue + i * 65536L / this->hurriRingLeds.numPixels();
        uint32_t color = this->hurriRingLeds.gamma32(this->hurriRingLeds.ColorHSV(hue)); // hue -> RGB
        this->hurriRingLeds.setPixelColor(i, color);         //  Set pixel's color (in RAM)
        this->hurriRingLeds.show();                          //  Update strip to match
        delay(wait);                           //  Pause for a moment
    }
    this->hurriRingLeds.clear(); 
}

void HurriRing::_roulette(uint32_t color) {
    if(this->_stateChange) this->_printd("Functioncall: _roulette \n");
    this->_stateChange = false;
    this->hurriRingLeds.clear();
    this->hurriRingLeds.setPixelColor(this->rouletteIdx, color);   
    this->hurriRingLeds.show();                                                         
    delay(this->rouletteSpeed); 
    this->rouletteIdx++; 
    if (this->rouletteIdx >= HURRI_RING_LED_COUNT)
    {
        this->rouletteIdx = 0; 
    }
}

void HurriRing::_randomNumber(uint8_t randomNumber, uint32_t color){
    if(this->_stateChange) this->_printd("Functioncall: _randomNumber \n");
    if(this->_stateChange){
        this->_setColor(HurriRing::BLACK,this->brightness);
        int array[HURRI_RING_LED_COUNT];
        for (int i = 0; i < HURRI_RING_LED_COUNT; i++) {
            array[i] = i + 1;  // Fill the array with numbers 1 to 60
        }

        this->_shuffleArray(array, HURRI_RING_LED_COUNT); 
        

        for(int index = 0; index < randomNumber; index++)
        {
            this->hurriRingLeds.setPixelColor(array[index], color); 
        }
        this->hurriRingLeds.show();
    }
    this->_stateChange = false;
}

void HurriRing::_static(uint32_t color){
    this->_setColor(color, this->brightness);
}

void HurriRing::_showSection(uint8_t section, uint32_t color){
    switch(section){
        case HurriRing::Section::LEFT:
            this->_showRingPart(2, 1, color);
            break;
        case HurriRing::Section::RIGHT:
            this->_showRingPart(2, 2, color);
            break;
        case HurriRing::Section::FIRST_QUARTER:
            this->_showRingPart(4, 1, color);
            break;
        case HurriRing::Section::SECOND_QUARTER:
            this->_showRingPart(4, 2, color);
            break;
        case HurriRing::Section::THIRD_QUARTER:
            this->_showRingPart(4, 3, color);
            break;
        case HurriRing::Section::FOURTH_QUARTER:
            this->_showRingPart(4, 4, color);
            break;
    }
}

void HurriRing::_showRingPart(uint8_t total_parts, uint8_t part, u_int32_t color)
{
  uint8_t part_size= uint8_t(this->hurriRingLeds.numPixels()/total_parts); 

  for(int i=(part-1)*part_size; i<part_size*part; i++) 

    { // For each pixel in strip...
      this->hurriRingLeds.setPixelColor(i, color);         //  Set pixel's color (in RAM)
      this->hurriRingLeds.show();                          //  Update strip to match                       //  Pause for a moment
    }
}

void HurriRing::_shuffleSections(){
    uint8_t randomSection = random(0, HurriRing::Section::NUM_OF_SECTIONS);
    this->_showSection(randomSection, HurriRing::Color::RED);
    delay(500);
    this->_fadeOut(5);
}

void HurriRing::_shuffleArray(int* array, int size) {
  // Shuffle the array using Fisher-Yates algorithm
  for (int i = size - 1; i > 0; i--) {
    int j = random(0, i + 1);  // Generate a random index
    // Swap array[i] with the element at the random index
    int temp = array[i];
    array[i] = array[j];
    array[j] = temp;
  }
}

void HurriRing::handleBleCommand(const uint8_t* data, size_t length) {
    if (length < 1) return;
    
    //Serial.print(data);
    // JSON-Daten parsen
    DeserializationError error = deserializeJson(this->jsonBuffer, data);
    uint8_t numberParam = 0;
    uint32_t colorParam = 0;
    HurriRing::HurriRing::State state = this->_lastState;
    bool stateChange = false;

    // Fehlerbehandlung
    if (error) {
        this->_printd("deserializeJson() failed: ");
        this->_printd(error.c_str());
    }
    else{
        this->_lastState = this->_state;
        String jsonState = jsonBuffer["state"].as<String>();

        state = this->_stringToState(jsonState.c_str());

        for(uint8_t paramIdx=0; paramIdx<jsonBuffer["parameter"].size(); paramIdx++){
            JsonObject item = jsonBuffer["parameter"][paramIdx];
            this->_printd("paramIdx: %d \n",paramIdx);
            for (JsonObject::iterator it = item.begin(); it != item.end(); ++it) {
                const char* key = it->key().c_str();
                this->_printd("Key: %s \n",key);
                if(strcmp(key,"color")==0){
                    const char* colorString = jsonBuffer["parameter"][paramIdx]["color"];
                    uint8_t r, g, b;
                    sscanf(colorString, "%hhu,%hhu,%hhu", &r, &g, &b);
                    colorParam = (r << 16) | (g << 8) | b;
                    this->_printd("Color Parameter: %d\n",colorParam);
                    stateChange = true;
                }
                else if (strcmp(key,"number")==0){
                    numberParam = jsonBuffer["parameter"][paramIdx]["number"].as<uint8_t>();
                    this->_printd("Number Parameter: %d\n",numberParam);
                    stateChange = true;
                }
            }
        }

        if((state != this->_lastState) || stateChange){
            stateChange = true;
            if(state != HurriRing::State::FREEZE && this->_lastState != HurriRing::State::IDLE){
                //this->_fadeOut();
                //this->_setColor(HurriRing::Color::BLACK, this->brightness, true);
            }
        }

        this->_state = state;
        this->colorParam = colorParam;
        this->numberParam = numberParam;
        this->_stateChange = stateChange;
    }
}

