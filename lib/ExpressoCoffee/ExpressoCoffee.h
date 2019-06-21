// Arduino Expresso Coffee Machine Classes
// https://github.com/klause/gel-coffee-avr-control-module
// Copyright (C) 2019 by Klause Nascimento and licensed under
// GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html

#ifndef EXPRESSO_COFFEE_H_INCLUDED
#define EXPRESSO_COFFEE_H_INCLUDED

#include <Arduino.h>
#include <JC_Button.h>          // https://github.com/JChristensen/JC_Button
#include <FlowMeter.h>

class BrewGroup {
    public:
        BrewGroup(byte groupNumber, byte ledPins[5], byte fluxometerPin, byte solenoidPin, byte pumpPin)
            :  m_groupNumber(groupNumber), m_flowMeterPin(fluxometerPin), m_solenoidPin(solenoidPin), m_pumpPin(pumpPin) {
                m_lenBrewOptions = sizeof(ledPins);
                // m_ledPins = new byte[noPins];
                BrewOption** ptrBrewOptions = new BrewOption*[m_lenBrewOptions];
                for (size_t i = 0; i < m_lenBrewOptions; i++)
                {
                    BrewOption bopt = BrewOption(ledPins[i]);
                    ptrBrewOptions[i] = &bopt;
                }
                m_brewOptions = *ptrBrewOptions;
            }
        void command(BrewOption brewOption);
        BrewOption currentOptionBrewing();
        void check();
        void turnOnPump();
        void turnOnSolenoid();
        void turnOffPump();
        void turnOffSolenoid();
        void begin();
        void onFlowMeterPulse();
        static BrewGroup getInstance(int8_t groupNumber);
    private:
        int8_t m_groupNumber;
        int8_t *m_ledPins;
        int8_t m_flowMeterPin;
        int8_t m_solenoidPin;
        int8_t m_pumpPin;
        size_t m_lenBrewOptions;
        long m_brewingStartTime;
        SimpleFlowMeter m_flowMeter;

        BrewOption* m_ptrCurrentBrewingOption;
        BrewOption* m_brewOptions;
};

class BrewOption {
    public:
        BrewOption(int8_t pin)
            : m_pin(pin), m_btn(Button(pin)) {};
        bool wasButtonPressed();
        int8_t getLedPin();
        void turnOnLed();
        void begin(){ m_btn.begin(); };
        long getDoseFlowmeterCount();
    private:
        int8_t m_pin;
        Button m_btn;
        long m_doseFlowmeterCount;
        long m_doseDuration;
};

class SimpleFlowMeter {
    public:
        SimpleFlowMeter(int8_t pin)
            : m_pin(pin) {};
        void count() { m_pulseCount++; };                                //!< Increments the internal pulse counter. Serves as an interrupt callback routine.
        void reset();                                //!< Prepares the flow meter for a fresh measurement. Resets all values.
        int8_t getPin() { return m_pin; };
        long getPulseCount() { return m_pulseCount; };
    protected:
        int8_t m_pin;
        long m_pulseCount = 0;
   
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
    int8_t groupNumber;                             //!< Group number.
    int8_t flowMeterPulseArray[5];                  //!< Each element holds pulse count for a brew option.
    int8_t durationArray[5];                        //!< Each element holds dosage duration for a brew option.
};

#endif
