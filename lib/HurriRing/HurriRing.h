#ifndef HURRI_RING
#define HURRI_RING

#define HURRI_RING_PIN          12
#define HURRI_RING_LED_COUNT    60

#include <Adafruit_NeoPixel.h>
#include <unordered_map>
#include <cstdio>
#include <ArduinoJson.h>
#include <BLECharacteristic.h>

// Add this forward declaration at the top of the file, after includes
class HurriRingCallbacks;

class HurriRing
{
    public:

        enum State {
            IDLE,
            RAINBOW,
            RAINBOW_WIPE,
            FREEZE,
            PULSE,
            ROULETTE,
            RANDOM_NUMBER,
            STATIC,
            SHUFFLE_SECTIONS,
            SHOW_SECTION,
            BRIGHTNESS,
            UNKNOWN
        };

        enum Section {
            LEFT,
            RIGHT,
            FIRST_QUARTER,
            SECOND_QUARTER,
            THIRD_QUARTER,
            FOURTH_QUARTER,
            NUM_OF_SECTIONS
        };

        const std::unordered_map<std::string, HurriRing::State> stringToStateMap = {
            {"IDLE", HurriRing::IDLE},
            {"RAINBOW", HurriRing::RAINBOW},
            {"RAINBOW_WIPE", HurriRing::RAINBOW_WIPE},
            {"FREEZE", HurriRing::FREEZE},
            {"PULSE", HurriRing::PULSE},
            {"ROULETTE", HurriRing::ROULETTE},
            {"RANDOM_NUMBER", HurriRing::RANDOM_NUMBER},
            {"STATIC", HurriRing::STATIC},
            {"SHUFFLE_SECTIONS", HurriRing::SHUFFLE_SECTIONS},
            {"SHOW_SECTION", HurriRing::SHOW_SECTION},
            {"BRIGHTNESS", HurriRing::BRIGHTNESS}
        };

        enum Color {
            RED = 0xFF0000,
            GREEN = 0x00FF00,
            YELLOW = 0xFFFF00,
            BLUE = 0x0000FF,
            WHITE = 0xFFFFFF,
            BLACK = 0x000000,
            NUMBER_OF_COLORS
        };

        HurriRing();

        void init(const char* deviceName);

        void loop();
        void setBrightness(uint8_t brightness);
        void setDebugFlag(bool debugEnable);
        void setState(HurriRing::State);
        void handleBleCommand(const uint8_t* data, size_t length);

    private:
        // Add friend declaration
        friend class HurriRingCallbacks;

        Adafruit_NeoPixel hurriRingLeds;

        static HurriRing* instance;

        bool debugEnabled = false;
        uint8_t brightness = 150;

        uint32_t colorParam = 0xFFFFFF; 
        uint8_t numberParam = 0;

        unsigned long wifiCheckCounter = 0;

        StaticJsonDocument<200> jsonBuffer;

        HurriRing::State _state = HurriRing::State::IDLE;
        HurriRing::State _lastState = HurriRing::State::IDLE;
        bool _stateChange = true;

        uint8_t rouletteIdx = 0;
        uint8_t rouletteSpeed = 5;

        BLECharacteristic* pCharacteristic = nullptr;
        static const char* SERVICE_UUID;
        static const char* CHARACTERISTIC_UUID;

        // led functions
        void _setColor(uint32_t color, uint8_t brightness, bool once=false);
        void _fadeOut(uint8_t stepDelay = 20);
        void _fadeIn(uint8_t stepDelay = 20, uint8_t brightness = 255);
        void _rainbowWipe(int wait);
        void _theaterChaseRainbow(int wait);
        void _pulse(int wait, uint32_t color);
        void _roulette(uint32_t color);
        void _randomNumber(uint8_t randomNumber, uint32_t color);
        void _shuffleArray(int* array, int size);
        void _static(uint32_t color);
        void _showSection(uint8_t section, uint32_t color);
        void _showRingPart(uint8_t total_parts, uint8_t part, u_int32_t color);
        void _shuffleSections();

        // common functions
        State _stringToState(const std::string& stateStr);
        void _printd(const char *msg);
        template<typename... Args>
        void _printd(const char *msg, Args... args);
};
#endif