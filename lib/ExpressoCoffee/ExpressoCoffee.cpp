// Arduino Expresso Coffee Machine Classes
// https://github.com/klause/gel-coffee-avr-control-module
// Copyright (C) 2019 by Klause Nascimento and licensed under
// GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html

#include "ExpressoCoffee.h"

/*----------------------------------------------------------------------*
/ returns the state of the button, true if pressed, false if released.  *
/ does debouncing, captures and maintains times, previous state, etc.   *
/-----------------------------------------------------------------------*/
void BrewGroup::check()
{

    // for each brew option check wheter the button was pressed
    for (byte i = 0; i < m_lenBrewOptions; i++)
    {
        BrewOption bopt = m_brewOptions[i];
        if (bopt.wasButtonPressed())
        {
            command(bopt);
            break;
        }
    }

    if (m_ptrCurrentBrewingOption)
    {
        m_ptrCurrentBrewingOption->turnOnLed();
    }
}

void BrewGroup::command(BrewOption brewOption) {
    if (!m_ptrCurrentBrewingOption) {
        m_ptrCurrentBrewingOption = &brewOption;
        m_ptrCurrentBrewingOption->turnOnLed();
        m_flowMeter.reset();
        m_brewingStartTime = millis();
        turnOnSolenoid();
        turnOnPump();
    } else {
        // stop if any group's button is pressed during brewing
        m_ptrCurrentBrewingOption = NULL;
        turnOffPump();
        turnOffSolenoid();
    }
}
void BrewGroup::turnOnPump() {
    digitalWrite(m_pumpPin, HIGH);
}

void BrewGroup::turnOnSolenoid() {
    digitalWrite(m_solenoidPin, HIGH);
}

void BrewGroup::turnOffPump() {
    digitalWrite(m_pumpPin, LOW);
}

void BrewGroup::turnOffSolenoid() {
    digitalWrite(m_solenoidPin, LOW);
}

void meterISRGroup1() {
    BrewGroup bg1 = BrewGroup::getInstance(1);
    bg1.onFlowMeterPulse();
}

void meterISRGroup2() {
    BrewGroup bg2 = BrewGroup::getInstance(2);
    bg2.onFlowMeterPulse();
}

void BrewGroup::begin(){
    pinMode(m_solenoidPin, OUTPUT);
    pinMode(m_pumpPin, OUTPUT);
    m_flowMeter = SimpleFlowMeter(m_flowMeterPin);

    if (m_groupNumber == 1) {
        attachInterrupt(digitalPinToInterrupt(m_flowMeterPin), meterISRGroup1, RISING);
    } else if (m_groupNumber == 2) {
        attachInterrupt(digitalPinToInterrupt(m_flowMeterPin), meterISRGroup2, RISING);
    }

    for (size_t i = 0; i < m_lenBrewOptions; i++)
    {
        m_brewOptions[i].begin();
    }
    m_flowMeter.reset();
}

void BrewGroup::onFlowMeterPulse(){
    if (m_ptrCurrentBrewingOption) {
        m_flowMeter.count();
        if(m_flowMeter.getPulseCount() >= m_ptrCurrentBrewingOption->getDoseFlowmeterCount()) {
            turnOffPump();
            turnOffSolenoid();
        }
    }
}

bool BrewOption::wasButtonPressed()
{
    pinMode(m_pin, INPUT_PULLUP);
    m_btn.read();
    return m_btn.wasPressed();
}

void BrewOption::turnOnLed()
{
    // turn LED on if button is released
    if (m_btn.isReleased())
    {
        digitalWrite(m_pin, LOW);
        pinMode(m_pin, OUTPUT);
    }
}
