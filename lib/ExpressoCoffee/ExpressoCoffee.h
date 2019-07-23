// Arduino Expresso Coffee Machine Classes
// https://github.com/klause/gel-coffee-avr-control-module
// Copyright (C) 2019 by Klause Nascimento and licensed under
// GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html

#ifndef EXPRESSO_COFFEE_H_INCLUDED
#define EXPRESSO_COFFEE_H_INCLUDED

#include "JC_Button.h"

#define DEBUG_LEVEL DEBUG_NONE
#include <Debug.h>

const int8_t BREW_GROUPS_LEN = 2;
const int8_t GROUP_PINS_LEN = 5;

const int8_t NON_STOP_BREW_OPTION_INDEX = 4;                        //!< index in brew option array correspoding to continuos brew option

const int16_t MILLIS_TO_ENTER_PROGRAM_MODE = 3000;

const unsigned long LEDS_BLINK_INTERVAL = 800;                      //!< interval at which to blink leds on programming mode (milliseconds)

const bool DELAY_AFTER_ON = true;
const bool DONT_DELAY_AFTER_ON = false;

const long MIN_FLOWMETER_PULSE_CONFIG = 60;                          //!< min valeu allowed to set for flowmeter pulse config (count)
const long MIN_DOSE_DURATION_CONFIG = 20 * 1000;                     //!< min valeu allowed to set for duration config (ms)

#ifndef PUMP_PIN
#define PUMP_PIN A5
#endif

#ifndef WATER_LEVEL_PIN
#define WATER_LEVEL_PIN 9
#endif

#ifndef SOLENOID_BOILER_PIN
#define SOLENOID_BOILER_PIN A4
#endif

enum ButtonAction {
    BUTTON_NOT_PRESSED = 0,
    BUTTON_PRESSED_FOR_BREWING = 1,
    BUTTON_PRESSED_FOR_PROGRAM = 2,
    BUTTON_PRESSED_FOR_CONTINUOS_BREWING = 3
};

enum LedStatus {
    OFF = 0,
    ON = 1
};

enum FilterOption {
    ALL = 0,
    ONLY_PROGRAMMED = 1,
    ONLY_NOT_PROGRAMMED = 2
};

/**
 * DosageRecord
 *
 * Structure that holds dosage settings for each brew option.
 * For each group this record will be loaded from EEPROM on startup.
 * These data will be writen to EEPROM whenever user ajusts the settings for a brew option.
 */
struct DosageRecord {
    uint8_t flowMeterPulseArray[4] = { 30, 60, 60, 120 };           //!< Each element holds pulse count for a brew option.
    uint8_t durationArray[4] = { 30, 30, 30, 30 };                  //!< Each element holds dosage duration (seconds) for a brew option.
};

class ExpressoMachine;
class BrewGroup;

class BrewOption {
public:
    BrewOption(){};
    BrewOption(int8_t pin, long doseFlowmeterCount, int8_t doseDurationSec, BrewGroup* parentBrewGroup)
        : m_pin(pin), m_parentBrewGroup(parentBrewGroup)
    {
        m_btn = new Button(pin);
        setDosageConfig(doseDurationSec * 1000, doseFlowmeterCount);
        DEBUG3_VALUE("BrewOption constructor, pin=", m_pin);
        DEBUG3_VALUE(". Dose duration(s): ", doseDurationSec);
        DEBUG3_VALUELN(". Flowmeter count: ", doseFlowmeterCount);
        // DEBUG3_VALUELN(". Continuos: ", isContinuos ? "TRUE" : "FALSE");
    };
    ButtonAction loop();
    void setup()
    {
        DEBUG3_VALUELN("begin() on brew option of pin ", m_pin);
        m_btn->begin();
        m_pinMode = INPUT_PULLUP;
    };
    // bool isContinuos = false;
    bool flagProgrammed = false;
    LedStatus ledStatus = OFF;
    long doseFlowmeterCount = MIN_FLOWMETER_PULSE_CONFIG;
    unsigned long doseDurationMillis = MIN_DOSE_DURATION_CONFIG;
    void onStartBrewing(bool isProgramming);
    void onEndBrewing(long brewingStartTime, long lastFlowmeterCount, bool isProgramming);
    void setDosageConfig(int8_t doseDurationSec, long doseFlowmeterCount);
    bool canFinishBrewing(unsigned long elapsedBrewMillis, long pulseCount);

protected:
    Button* m_btn = NULL;
    unsigned long m_lastActionMs = 0;

private:
    void turnOnLed();
    int8_t m_pin = -1;
    BrewGroup* m_parentBrewGroup = NULL;
    int8_t m_pinMode;
};

class SimpleFlowMeter {
public:
    SimpleFlowMeter(){};
    void increment();
    void reset();
    long getPulseCount() { return m_pulseCount; };

protected:
    volatile long m_pulseCount = 0;
    unsigned long m_lastPulseMs;
};

class NonStopBrewOption: public BrewOption {
public:
    NonStopBrewOption(){};
    NonStopBrewOption(int8_t pin, long doseFlowmeterCount, int8_t doseDurationSec, BrewGroup* parentBrewGroup)
        : BrewOption(pin, doseFlowmeterCount, doseDurationSec, parentBrewGroup)
    {
    };
    ButtonAction loop();
private:
    bool m_btnReleasedAfterPressedForProgram = true;
};

class BrewGroup {
public:
    BrewGroup(){};
    BrewGroup(int8_t groupNumber, int8_t pinArray[GROUP_PINS_LEN], SimpleFlowMeter* flowMeter, int8_t solenoidPin);
    void startBrewing(BrewOption* brewOption);
    void stopBrewing();
    void loop();
    void setup();
    int8_t getGroupNumber() { return m_groupNumber; };
    void setParent(ExpressoMachine* expressoMachine) { m_ptrExpressoMachine = expressoMachine; };
    void setDosageConfig(DosageRecord dosageConfig);
    void saveDosageRecord();
    BrewOption* ptrCurrentBrewingOption = NULL;
    bool onProgrammingMode = false;

private:
    int8_t m_groupNumber = 0;
    int8_t m_solenoidPin = -1;
    int8_t* m_brewOptionPins;
    unsigned long m_brewingStartTime = -1;
    unsigned long m_previousLedsBlinkMillis = 0;
    ExpressoMachine* m_ptrExpressoMachine = NULL;
    bool m_flagSetup = false;
    bool m_ledsBlinkToggle;

    SimpleFlowMeter* m_flowMeter = NULL;
    BrewOption* m_ptrProgrammingBrewOption = NULL;
    BrewOption m_brewOptions[GROUP_PINS_LEN];

    DosageRecord loadDosageRecord();
    void turnOnGroupSolenoid();
    void turnOffGroupSolenoid();
    void enterProgrammingMode();
    void exitProgrammingMode();
    void setStatusLeds(LedStatus s, FilterOption filter);
};

class ExpressoMachine {

public:
    ExpressoMachine(BrewGroup* brewGroups, int8_t lenBrewGroups)
        : m_brewGroups(brewGroups)
        , m_lenBrewGroups(lenBrewGroups)
    {

        for (int8_t i = 0; i < lenBrewGroups; i++) {
            brewGroups[i].setParent(this);
            DEBUG3_VALUELN("ExpressoMachine() - group ", m_brewGroups[i].getGroupNumber());
        }
    };

    BrewGroup* getBrewGroup(int8_t groupNumber);
    BrewGroup* getBrewGroups() { return m_brewGroups; };

    void setup();
    void loop();
    void turnOffPump(BrewGroup* brewGroupAsking);
    void turnOnBoilerSolenoid();
    void turnOffBoilerSolenoid();
    void turnOnPump();
    bool isProgramming();

private:
    BrewGroup* m_brewGroups;
    int8_t m_lenBrewGroups;
    void turnOffPump();
    bool m_flagSetup = false;
    unsigned long m_waterLevelReachedMs = 0;
    bool m_fillingBoiler = false;
    bool isBoilerWaterLevelLow();
    void startFillingBoiler();
    void stopFillingBoiler();
};

#endif
