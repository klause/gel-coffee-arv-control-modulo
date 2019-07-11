// Arduino Expresso Coffee Machine Classes
// https://github.com/klause/gel-coffee-avr-control-module
// Copyright (C) 2019 by Klause Nascimento and licensed under
// GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html

#ifndef EXPRESSO_COFFEE_H_INCLUDED
#define EXPRESSO_COFFEE_H_INCLUDED

#include "JC_Button.h"

#define DEBUG_LEVEL DEBUG_MID
#include <Debug.h>

const int8_t BREW_GROUPS_LEN = 2;
const int8_t GROUP_PINS_LEN = 5;

const int8_t NON_STOP_BREW_OPTION = 5;

const int16_t MILLIS_TO_ENTER_PROGRAM_MODE = 3000;

const unsigned long LEDS_BLINK_INTERVAL = 800;                             //!< interval at which to blink leds on programming mode (milliseconds)

const bool DELAY_AFTER_ON = true;
const bool DONT_DELAY_AFTER_ON = false;

const int8_t 
  PUMP_PIN(A4),
  WATER_LEVEL_PIN(A2),
  SOLENOID_BOILER_PIN(A3);

enum ButtonAction {
    BUTTON_NOT_PRESSED = 0,
    BUTTON_PRESSED_FOR_BREWING = 1,
    BUTTON_PRESSED_FOR_PROGRAM = 2
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
 * 
 */
struct DosageRecord {
    // int8_t groupNumber;                             //!< Group number.
    uint8_t flowMeterPulseArray[4] = { 30, 60, 60, 120 };                  //!< Each element holds pulse count for a brew option.
    uint8_t durationArray[4] = { 30, 30, 30, 30 };                         //!< Each element holds dosage duration (seconds) for a brew option.
};

class ExpressoMachine;
class BrewGroup;

//class Button {
//    public:
//        Button(int8_t pin) {};
//        void begin() {};
//        bool isReleased() {return false;};
//        bool wasPressed() {return false;};
//        bool pressedFor(long ms) { return false; };
//        void read() {};
//
//};

class BrewOption {
    public:
        BrewOption() { };
        BrewOption(int8_t pin, long doseFlowmeterCount, long doseDuration, BrewGroup* parentBrewGroup)
            : m_pin(pin),  m_doseFlowmeterCount(doseFlowmeterCount), m_doseDuration(doseDuration), m_parentBrewGroup(parentBrewGroup) {
                m_btn = new Button(pin);
                DEBUG3_VALUELN("BrewOption constructor, pin=", m_pin);
            };
        ButtonAction loop();
        int8_t getLedPin() { return m_pin; };
        void setup() {
            DEBUG3_VALUELN("begin() on brew option of pin ", m_pin);
            m_btn->begin();
            m_pinMode = INPUT_PULLUP;
        };
        long getDoseFlowmeterCount() { return m_doseFlowmeterCount; };
        long getDoseDuration() { return m_doseDuration; };
        bool isContinuos() { return m_doseFlowmeterCount <= 0 && m_doseDuration <= 0; };
        void onStartBrewing();
        void onEndBrewing();
        bool isProgrammed() { return m_flagProgrammed; };
        void setProgrammed(bool flagProgrammed) { m_flagProgrammed = flagProgrammed; };
        void setLedStatus(LedStatus s) { m_ledStatus = s; };
    private:
        void turnOnLed(bool delayAfterOn);
        int8_t m_pin = -1;
        long m_doseFlowmeterCount = 30;
        long m_doseDuration = 30 * 1000;
        Button* m_btn = NULL;
        BrewGroup* m_parentBrewGroup = NULL;
        bool m_btnReleasedAfterPressedForProgram = false;
        bool m_flagProgrammed = false;
        LedStatus m_ledStatus = OFF;
        int8_t m_pinMode;
};

class SimpleFlowMeter {
    public:
        SimpleFlowMeter(int8_t pin) : m_pin(pin) {};
        void count() { m_pulseCount++; };            //!< Increments flowmeter pulse counter.
        void reset() { m_pulseCount=0; }             //!< Prepares the flow meter for a fresh measurement. Resets pulse counter.
        int8_t getPin() { return m_pin; };
        long getPulseCount() { return m_pulseCount; };
    protected:
        int8_t m_pin;
        long m_pulseCount = 0;
};

class BrewGroup {
    public:
        BrewGroup() { };
        BrewGroup(int8_t groupNumber, int8_t pinArray[GROUP_PINS_LEN], int8_t fluxometerPin, int8_t solenoidPin);
        void command(BrewOption* brewOption);
        void loop();
        void setup();
        void onFlowMeterPulse();
        bool isBrewing() { return m_ptrCurrentBrewingOption; };
        bool isProgramming() { return m_programmingMode; };
        int8_t getGroupNumber() { return m_groupNumber; };
        BrewOption* currentOptionBrewing() { return m_ptrCurrentBrewingOption; };
        void setParent(ExpressoMachine* expressoMachine) { m_expressoMachine = expressoMachine; };
        void setDosageConfig(DosageRecord dosageConfig);
        void saveDosageRecord();
        unsigned long getBrewingStartTime() { return m_brewingStartTime; };
        unsigned long getLastFlowmeterCount() { return m_lastFlowmeterCount; };
    private:
        int8_t m_groupNumber = 0;
        int8_t m_flowMeterPin = -1;
        int8_t m_solenoidPin = -1;
        int8_t* m_brewOptionPins;
//        int8_t m_lenBrewOptions;
        unsigned long m_brewingStartTime = -1;
        long m_lastFlowmeterCount = 0;
        unsigned long m_previousLedsBlinkMillis = 0;
        bool m_programmingMode = false;
        ExpressoMachine* m_expressoMachine = NULL;
        bool m_flagSetup = false;
        bool m_ledsBlinkToggle;

        SimpleFlowMeter* m_flowMeter = NULL;
        BrewOption* m_ptrCurrentBrewingOption = NULL;
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
        static void init(BrewGroup* brewGroups, int8_t lenBrewGroups) {
            if (instance == 0)
            {
            	DEBUG3_PRINTLN("ExpressoMachine::init()");
                
                instance = new ExpressoMachine(brewGroups, lenBrewGroups);
            }
        }
        static ExpressoMachine* getInstance() { return instance; };
        static ExpressoMachine* instance;

        BrewGroup* getBrewGroup(int8_t groupNumber);
        BrewGroup* getBrewGroups() { return m_brewGroups; };

        void setup();

        void turnOffPump(BrewGroup* brewGroupAsking);

        void turnOnBoilerSolenoid();
        void turnOffBoilerSolenoid();

        void turnOnPump();

        void loop();

        bool isProgramming();

    private:
        ExpressoMachine();
        ExpressoMachine(BrewGroup* brewGroups, int8_t lenBrewGroups)
            : m_brewGroups(brewGroups), m_lenBrewGroups (lenBrewGroups) {

                for (int8_t i = 0; i < lenBrewGroups; i++)
                {
                	brewGroups[i].setParent(this);
                    DEBUG3_VALUELN("ExpressoMachine() - group ", m_brewGroups[i].getGroupNumber());
                    // DEBUG3_HEXVALLN("; address: ", &m_brewGroups[i]);
                }

             };
        BrewGroup *m_brewGroups;
        int8_t m_lenBrewGroups;
        void turnOffPump();
        bool isBoilerWaterLevelLow();
        bool isFillingBoiler();
        bool m_flagSetup = false;
};

#endif
