// Arduino Expresso Coffee Machine Classes
// https://github.com/klause/gel-coffee-avr-control-module
// Copyright (C) 2019 by Klause Nascimento and licensed under
// GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html

#include "ExpressoCoffee.h"
#include <EEPromUtils.h>

BrewGroup::BrewGroup(int8_t groupNumber, int8_t pinArray[], SimpleFlowMeter* flowMeter, int8_t solenoidPin) {

    m_groupNumber = groupNumber;
    m_flowMeter = flowMeter;
    m_solenoidPin = solenoidPin;
    m_brewOptionPins = pinArray;

    DEBUG3_VALUELN("Instantiating BrewGroup ", m_groupNumber);
}

/*----------------------------------------------------------------------*
/ this method must be frequently called in loop function to check       *
/ buttons's press, dosage timeouts etc.                                 *
/-----------------------------------------------------------------------*/
void BrewGroup::loop()
{

    DEBUG4_PRINTLN("BrewGroup::loop()");

    BrewOption* bopt = NULL;

    // for each brew option check whether the button was pushed
    for (int8_t i = 0; i < BREW_OPTIONS_LEN; i++)
    {

        bopt = m_brewOptions[i];

        ButtonAction pressed = bopt->loop();

        DEBUG5_VALUE("BrewOption ", i+1);
        DEBUG5_VALUELN(" returned ", pressed);

        #ifdef DEBUG_MID
            if (pressed != BUTTON_NOT_PRESSED) {
                DEBUG3_VALUE("BrewOption ", i+1);
                DEBUG3_VALUELN(" returned ", pressed);
            }
        #endif

        // 
        /* If brewing in this group, only process button command for the current brewing option or continuous brew option,
           pressing other buttons on the group will be ignored */
        if (ptrCurrentBrewingOption != NULL && ptrCurrentBrewingOption != bopt && CONTINUOUS_BREW_OPTION_INDEX != i) {
            continue;
        }

        /* Short pressing brewing option is for:
         * - Start brewing (out of prog mode) if group is not brewing for other option
         * - Stop brewing (out of prog mode) if pressed option is currently brewing
         * - Start brewing (in prog mode) if group is not brewing for other option
         * - Stop brewing (in prog mode) if pressed option is currently brewing
         * - Stop brewing (out of prog mode) if pressed option is continuous option
         * - Exit programming mode (continuous button) if this mode is active
         */
        if (BUTTON_PRESSED_FOR_CONTINUOUS_BREWING == pressed || BUTTON_PRESSED_FOR_BREWING == pressed) {

            if (BUTTON_PRESSED_FOR_CONTINUOUS_BREWING == pressed && m_ptrExpressoMachine->isOnProgrammingMode) {

                if (m_ptrExpressoMachine->ptrFirstCompletedProgramming != NULL && allOptionsWaitingForProgramming()) {
                    copyDosageConfig(m_ptrExpressoMachine->ptrFirstCompletedProgramming);
                    setStatusLeds(ON, ONLY_PROGRAMMED);
                } else {
                    // programming button was pressed to exit programmin mode
                    DEBUG3_VALUELN("Button pressed to exit programming mode on group ", m_groupNumber);
                    exitProgrammingMode();
                }
            } else {
				DEBUG3_VALUE("Button pressed for brewing. Option ", i+1);
				DEBUG3_VALUELN(" on group ", m_groupNumber);
				if (ptrCurrentBrewingOption == NULL) {
                    startBrewing(bopt);
                } else {
                    stopBrewing();
                }
            }
            break;  //!< Exit for loop since no other command will be detected so fast in this group

        } else if (BUTTON_PRESSED_FOR_PROGRAM == pressed && !m_ptrExpressoMachine->isOnProgrammingMode) {
            DEBUG3_VALUELN("Button pressed to enter programming mode on group ", m_groupNumber);
            enterProgrammingMode();
            break;  //!< Exit for loop since no other command will be detected so fast in this group
        }

    }

    unsigned long currentMillis = millis();
    if (ptrCurrentBrewingOption != NULL) {
        if (!m_ptrExpressoMachine->isOnProgrammingMode && ptrCurrentBrewingOption->canFinishBrewing(currentMillis - m_brewingStartTime, m_flowMeter->getPulseCount())) {
            stopBrewing();
        }
    } else if (m_ptrExpressoMachine->isOnProgrammingMode && !m_ptrExpressoMachine->isBrewing && m_toggleBlinkLeds) {
        m_blinkLedsStatus = m_blinkLedsStatus == ON ? OFF : ON;
        setStatusLeds(m_blinkLedsStatus, ONLY_NOT_PROGRAMMED);
    } else if (m_ptrExpressoMachine->isBrewing) {
        setStatusLeds(OFF, ALL);
    }
}

ButtonAction BrewOption::loop()
{

    DEBUG5_VALUELN("BrewOption::loop() option on pin ", m_pin);

    /* INPUT_PULLUP need to be set before reading button value */
	if (m_pinMode != INPUT_PULLUP) {
    	pinMode(m_pin, INPUT_PULLUP);
        m_pinMode = INPUT_PULLUP;
        DEBUG5_VALUELN("  Set INPUT_PULLUP for pin", m_pin);
	}
    
    m_btn->read();

    if (ledStatus == ON) {
        turnOnLed();
    }

    if (m_btn->wasReleased() && millis() - m_lastActionMs > 500) {
        m_lastActionMs = millis();
        return BUTTON_PRESSED_FOR_BREWING;
    }

    return BUTTON_NOT_PRESSED;
}

ButtonAction ContinuousBrewOption::loop()
{

    ButtonAction ret = BrewOption::loop();

    if (BUTTON_PRESSED_FOR_BREWING == ret) {
        if (m_btnReleasedAfterPressedForProgram) {
            ret = BUTTON_PRESSED_FOR_CONTINUOUS_BREWING;
        } else {
            m_btnReleasedAfterPressedForProgram = true;
            ret = BUTTON_NOT_PRESSED;
        }
    } else if (m_btn->pressedFor(MILLIS_TO_ENTER_PROGRAM_MODE) && m_btnReleasedAfterPressedForProgram) {
        m_btnReleasedAfterPressedForProgram = false;
        m_lastActionMs = millis();
        ret = BUTTON_PRESSED_FOR_PROGRAM;
    }

    return ret;
}

bool BrewOption::canFinishBrewing(unsigned long elapsedBrewMillis, long pulseCount) {

    if(pulseCount >= doseFlowmeterCount) {
        DEBUG3_VALUELN("Flow count reached: ", pulseCount);
        return true;
    } else if (pulseCount <= 3 && elapsedBrewMillis >= doseDurationMillis) {
        /* if flowmeter pulses are not being incremented for malfunction
         * stop brewing based on duration */
        DEBUG3_PRINTLN("No flowmeter activity detected. Brewing timed out by duration.");
        return true;
    } else if (elapsedBrewMillis >= doseDurationMillis * 2) {
        /* maybe water is not flowing because of too fine ground coffee
         * stop brewing after 2 times the duration config */
        DEBUG3_PRINTLN("Flowmeter count is not evolving. Stoping brewing after 2 X duration config.");
        return true;
    }

    return false;
}

bool ContinuousBrewOption::canFinishBrewing(unsigned long elapsedBrewMillis, long pulseCount) {
    return false;
}

void BrewGroup::startBrewing(BrewOption* brewOption) {
    // start brewing
    DEBUG3_VALUELN("Start brewing on group ", m_groupNumber);
    ptrCurrentBrewingOption = brewOption;                               //!< set brewing option on correponding group
    setStatusLeds(OFF, ALL);                                            //!< set all led status to OFF
    ptrCurrentBrewingOption->onStartBrewing(m_ptrExpressoMachine->isOnProgrammingMode);
    m_flowMeter->reset();                                               //!< reset flowmeter count
    m_brewingStartTime = millis();                                      //!< store brewing start time
    m_ptrExpressoMachine->turnOffBoilerSolenoid();                      //!< ensure boiler solenoid is OFF before start pumping water
    turnOnGroupSolenoid();                                              //!< turn ON solenoid on corresponding group
    m_ptrExpressoMachine->turnOnPump();                                 //!< turn ON water pump
}

void BrewGroup::stopBrewing() {
    // stop brewing
    DEBUG3_VALUE("Stop brewing on group ", m_groupNumber);
    DEBUG3_VALUE(". Brew time: ", ((millis() - m_brewingStartTime) / 1000));
    DEBUG3_VALUELN(". Flowmeter count: ", m_flowMeter->getPulseCount());
    // only turn off pump if other groups are not brewing
    m_ptrExpressoMachine->turnOffPump(this);
    turnOffGroupSolenoid();
    ptrCurrentBrewingOption->onEndBrewing(m_brewingStartTime, m_flowMeter->getPulseCount(), m_ptrExpressoMachine->isOnProgrammingMode);
    if (m_ptrExpressoMachine->isOnProgrammingMode)
    {
        setStatusLeds(ON, ONLY_PROGRAMMED);
        saveDosageRecord();
        m_programmedCount = 0;
        for (int8_t i = 0; i < BREW_OPTIONS_LEN; i++) {
            if (i != CONTINUOUS_BREW_OPTION_INDEX && m_brewOptions[i]->flagProgrammed) {
                m_programmedCount++;
            }
        }
        if (m_programmedCount == BREW_OPTIONS_LEN-1 && m_ptrExpressoMachine->ptrFirstCompletedProgramming == NULL)
        {
            m_ptrExpressoMachine->setFirstCompletedProgramming(this);
        }
        
    }
    ptrCurrentBrewingOption = NULL;
}

void BrewGroup::turnOnGroupSolenoid() {
    DEBUG3_VALUELN("Turning ON solenoid of group ", m_groupNumber);
    digitalWrite(m_solenoidPin, LOW);                              //!< LOW turns solenoid ON
}

void BrewGroup::turnOffGroupSolenoid() {
    DEBUG3_VALUELN("Turning OFF solenoid of group ", m_groupNumber);
    digitalWrite(m_solenoidPin, HIGH);                             //!< HIGH turns solenoid OFF
}

void BrewGroup::setup(){

    DEBUG3_VALUELN("setup() on group ", m_groupNumber);

    DosageRecord dosageConfig = loadDosageRecord();

    m_brewOptions = new BrewOption*[BREW_OPTIONS_LEN];

    for (int8_t i = 0; i < BREW_OPTIONS_LEN; i++)
    {
        long flowMeterPulseConfig = 0;
        long durationConfig = 0;
        if (CONTINUOUS_BREW_OPTION_INDEX != i) {
            flowMeterPulseConfig = dosageConfig.flowMeterPulseArray[i];
            durationConfig = dosageConfig.durationArray[i];
            m_brewOptions[i] = new BrewOption(m_brewOptionPins[i], flowMeterPulseConfig, durationConfig, this);
        } else {
            m_brewOptions[i] = new ContinuousBrewOption(m_brewOptionPins[i], this);
        }
    }

    pinMode(m_solenoidPin, OUTPUT);

    for (int8_t i = 0; i < BREW_OPTIONS_LEN; i++)
    {
        m_brewOptions[i]->setup();
    }
    m_flowMeter->reset();
    m_flagSetup = true;
}

void BrewGroup::enterProgrammingMode() {
    DEBUG2_PRINT("Entering programming mode");
    m_ptrExpressoMachine->enterProgrammingMode();
    m_programmedCount = 0;
    for (int8_t i = 0; i < BREW_OPTIONS_LEN; i++)
    {
        m_brewOptions[i]->flagProgrammed = false;
    }
    
}

void BrewGroup::exitProgrammingMode() {
    DEBUG2_PRINT("Exiting programming mode");
    m_ptrExpressoMachine->exitProgrammingMode();
    for (int8_t i = 0; i < BREW_OPTIONS_LEN;i++)
    {
        m_brewOptions[i]->flagProgrammed = false;
        m_brewOptions[i]->ledStatus = OFF;
    }
}

void BrewGroup::setStatusLeds(LedStatus s, FilterOption filter) {
    DEBUG4_VALUE("Setting leds of group ", m_groupNumber);
    DEBUG4_VALUELN(" to ", s == ON ? "ON" : "OFF" );
    
    for(int8_t i=0; i<BREW_OPTIONS_LEN; i++){
        if ( (filter == ONLY_PROGRAMMED && m_brewOptions[i]->flagProgrammed)
            || (filter == ONLY_NOT_PROGRAMMED && !m_brewOptions[i]->flagProgrammed)
            || filter == ALL ) {
            DEBUG5_VALUELN("  -> brew option ", i+1 );
            m_brewOptions[i]->ledStatus = s;
        }
    }
}

DosageRecord BrewGroup::loadDosageRecord() {

    DosageRecord rec = DosageRecord();
    int8_t ret = -1;

    if (EEPROM_init()) {
        DEBUG3_VALUELN("Loading dosage config from EEPROM for group ", m_groupNumber);
        size_t dataLen = sizeof(DosageRecord);
        size_t location = EEPROM_SIZE( dataLen ) * (m_groupNumber-1);

        #if DEBUG_LEVEL >= DEBUG_ERROR
            ret = EEPROM_safe_read(location, (uint8_t*) &rec, dataLen);
            if (ret < 0) {
                DEBUG1_VALUE("Error reading dosage record for group ", m_groupNumber);
                DEBUG1_VALUE(" at location: ", location);
                DEBUG1_VALUELN(". EEPROM_safe_write returned: ", ret);
            }
        #else
            ret = EEPROM_safe_read(location, (uint8_t*) &rec, dataLen);
        #endif

        if (ret < 0) {
            rec = DosageRecord();
        }

    }

    #if DEBUG_LEVEL >= DEBUG_MID
        Serial.print(F("Dosage config for group "));
        Serial.println(m_groupNumber);
        Serial.print(F("  flowMeterPulseArray = [ "));
        Serial.print(rec.flowMeterPulseArray[0]);
        Serial.print(F(", "));
        Serial.print(rec.flowMeterPulseArray[1]);
        Serial.print(F(", "));
        Serial.print(rec.flowMeterPulseArray[2]);
        Serial.print(F(", "));
        Serial.print(rec.flowMeterPulseArray[3]);
        Serial.println(F(" ]"));
        Serial.print(F("  durationArray = [ "));
        Serial.print(rec.durationArray[0]);
        Serial.print(F(", "));
        Serial.print(rec.durationArray[1]);
        Serial.print(F(", "));
        Serial.print(rec.durationArray[2]);
        Serial.print(F(", "));
        Serial.print(rec.durationArray[3]);
        Serial.println(F(" ]"));
    #endif

    return rec;
}


void BrewGroup::saveDosageRecord() {

    DosageRecord rec = DosageRecord();

    for (int8_t i = 0; i < BREW_OPTIONS_LEN; i++) {
        if (i != CONTINUOUS_BREW_OPTION_INDEX) {
            rec.durationArray[i] = m_brewOptions[i]->doseDurationMillis / 1000;
            rec.flowMeterPulseArray[i] = m_brewOptions[i]->doseFlowmeterCount;
        }
    }
    
    size_t dataLen = sizeof(rec);
    int location = 0;
    if (m_groupNumber > 1) {
        location = EEPROM_SIZE( dataLen ) * (m_groupNumber-1);
    }

    DEBUG2_VALUE("Saving dosage record for group ", m_groupNumber);
    DEBUG2_VALUELN(" on EEPROM @ location ", location);

    EEPROM_init();

    #if DEBUG_LEVEL >= DEBUG_ERROR
        int ret = EEPROM_safe_write(location, (uint8_t*) &rec, dataLen);
        #if DEBUG_LEVEL >= DEBUG_LEVEL_LOW
            if (ret > 0) {
                DEBUG2_VALUE("Success saving dosage record for group ", m_groupNumber);
                DEBUG2_VALUE(". ", dataLen);
                DEBUG2_VALUELN(" bytes writen to EEPROM at location: ", location);

                DEBUG3_PRINT("  [");
                for (size_t i = 0; i < dataLen; i++)
                {
                    DEBUG3_HEXVAL(" ", ((uint8_t*) &rec)[i]);
                }
                DEBUG3_PRINTLN(" ]");
            }
        #endif

        if (ret < 0) {
            DEBUG1_VALUELN("Error saving dosage record. EEPROM_safe_write returned: ", ret);
        }
    #else
        EEPROM_safe_write(location, (uint8_t*) &rec, dataLen);
    #endif

}

bool BrewGroup::allOptionsWaitingForProgramming() {
    for (int8_t i = 0; i < BREW_OPTIONS_LEN; i++) {
        if (i != CONTINUOUS_BREW_OPTION_INDEX && m_brewOptions[i]->flagProgrammed) {
            return false;
        }
    }
    return true;
}

void BrewGroup::copyDosageConfig(BrewGroup* from) {
    for (int8_t i = 0; i < BREW_OPTIONS_LEN; i++) {
        if (i != CONTINUOUS_BREW_OPTION_INDEX) {
            m_brewOptions[i]->setDosageConfig(from->m_brewOptions[i]->doseDurationMillis, from->m_brewOptions[i]->doseFlowmeterCount);
            m_brewOptions[i]->flagProgrammed = true;
        }
    }
    saveDosageRecord();
}

void BrewOption::onEndBrewing(long brewingStartMillis, long lastFlowmeterCount, bool isProgramming) {
    DEBUG3_VALUELN("End brewing. Option's pin: ", m_pin);

    m_btn->begin();     //!< reset button status
    if (isProgramming) {
        setDosageConfig(millis() - brewingStartMillis, lastFlowmeterCount);
        flagProgrammed = true;
        ledStatus = ON;
    } else {
        ledStatus = OFF;
    }
}

void BrewOption::onStartBrewing(bool isProgramming) {
    if (isProgramming) {
        flagProgrammed = false;
    }
    ledStatus = ON;
}

void BrewOption::turnOnLed()
{
	// turn LED on if button is released
    if (m_btn->isReleased())
    {
        DEBUG5_VALUELN("Turning ON LED on pin ", m_pin);
        digitalWrite(m_pin, LOW);
        pinMode(m_pin, OUTPUT);
        m_pinMode = OUTPUT;
    }
}

void BrewOption::setDosageConfig(unsigned long durationParamMillis, long flowmeterParamCount) {
    doseFlowmeterCount  = flowmeterParamCount < MIN_FLOWMETER_PULSE_CONFIG ? MIN_FLOWMETER_PULSE_CONFIG : flowmeterParamCount;
    doseDurationMillis = durationParamMillis < MIN_DOSE_DURATION_CONFIG ? MIN_DOSE_DURATION_CONFIG : durationParamMillis;
}

void ExpressoMachine::setup() {

    DEBUG4_PRINTLN("setup() on ExpressoMachine instance");

     pinMode(m_pumpPin, OUTPUT);
     pinMode(m_solenoidBoilderPin, OUTPUT);
     pinMode(m_waterLevelPin, INPUT);

    for (int8_t i = 0; i < m_lenBrewGroups; i++)
    {
        m_brewGroups[i].setup();
    }
    m_flagSetup = true;
}

void ExpressoMachine::turnOnPump() {
    DEBUG3_PRINTLN("Turning ON pump");
    digitalWrite(m_pumpPin, LOW);        //!< LOW turns pump ON
}

/*----------------------------------------------------------------------*
/ turn off water pump only if there is no other group brewing           *
/-----------------------------------------------------------------------*/
void ExpressoMachine::turnOffPump(BrewGroup* brewGroupAsking) {
    
    for (int8_t i = 0; i < m_lenBrewGroups; i++)
    {
        if (m_brewGroups[i].ptrCurrentBrewingOption != NULL && brewGroupAsking != &m_brewGroups[i])
        {
            DEBUG3_VALUELN("Not stopping pump, other group brewing: ", m_brewGroups[i].getGroupNumber());
            return;
        }
    }
    turnOffPump();
}

/*----------------------------------------------------------------------*
/ imediatelly turn off water pump                                       *
/-----------------------------------------------------------------------*/
void ExpressoMachine::turnOffPump() {
    DEBUG3_PRINTLN("Turning OFF pump");
    digitalWrite(m_pumpPin, HIGH);                   //!< HIGH turns pump OFF
}

void ExpressoMachine::turnOnBoilerSolenoid() {
    DEBUG3_PRINTLN("Turning ON boiler solenoid");
     digitalWrite(m_solenoidBoilderPin, LOW);        //!< LOW turns solenoid ON
}
void ExpressoMachine::turnOffBoilerSolenoid() {
    DEBUG3_PRINTLN("Turning OFF boiler solenoid");
    m_fillingBoiler = false;
    digitalWrite(m_solenoidBoilderPin, HIGH);        //!< HIGH turns solenoid OFF
}

/*----------------------------------------------------------------------*
/ check current value of boiler's water low level sensor.               *
/ return true if level is low otherwise false                           *
/-----------------------------------------------------------------------*/
bool ExpressoMachine::isBoilerWaterLevelLow() {
	return digitalRead(m_waterLevelPin) == HIGH;                            //!< HIGH means level is low
}

void ExpressoMachine::startFillingBoiler() {
    DEBUG3_PRINTLN("Starting to fill the boiler");
    turnOnBoilerSolenoid();
    turnOnPump();
    m_fillingBoiler = true;
    m_waterLevelReachedMs = 0;
}

void ExpressoMachine::stopFillingBoiler() {
    DEBUG3_PRINTLN("Stopping to fill the boiler");
    turnOffPump();
    turnOffBoilerSolenoid();
}

BrewGroup* ExpressoMachine::getBrewGroup(int8_t groupNumber) {
    for (int8_t i = 0; i < m_lenBrewGroups; i++)
    {
        if (m_brewGroups[i].getGroupNumber() == groupNumber)
        {
            return &m_brewGroups[i];
        }
    }
    return NULL;
}

void ExpressoMachine::loop() {

    static unsigned long currentMillis = 0;
    static unsigned long previousLedsBlinkMillis = 0;
    static bool toggleBlinkLeds = false;

    DEBUG5_PRINTLN("ExpressoMachine::loop()");

    if (!m_flagSetup) {
        DEBUG3_PRINTLN("setup not called for ExpressoMachine instance");
        return;
    }

    toggleBlinkLeds = false;
    currentMillis = millis();
    if (isOnProgrammingMode && !isBrewing) {
        if ((currentMillis - previousLedsBlinkMillis) >= LEDS_BLINK_INTERVAL) {
            toggleBlinkLeds = true;
            previousLedsBlinkMillis = currentMillis;
        }
    }

    isBrewing = false;
    for (int8_t i = 0; i < m_lenBrewGroups; i++) {
        m_brewGroups[i].setToggleBlinkLeds(toggleBlinkLeds);
        m_brewGroups[i].loop();
        isBrewing = m_brewGroups[i].ptrCurrentBrewingOption != NULL || this->isBrewing;
    }

  // if groups are not brewing
  if (!isBrewing) {
    bool lowLevel = isBoilerWaterLevelLow();
    // bool isFilling = isFillingBoiler();

    // check if bolier needs more water
    if (lowLevel && !m_fillingBoiler) {
        // bolier water level is low, start filling
        startFillingBoiler();
    } else if (m_fillingBoiler && !lowLevel) {
        
        currentMillis = millis();
        if (m_waterLevelReachedMs == 0) {
            m_waterLevelReachedMs = currentMillis;
        }

        if ((currentMillis - m_waterLevelReachedMs) > 2000) {    //!< pump water for more 2 seconds after water level is ok
            // stop filling if water level is OK now
            stopFillingBoiler();
        }

    }
  }

}

void ExpressoMachine::setFirstCompletedProgramming(BrewGroup* ptrBrewGroup) {
    DEBUG3_VALUELN("First group complete programmed: ", ptrBrewGroup->getGroupNumber());
    this->ptrFirstCompletedProgramming = ptrBrewGroup;
}

void ExpressoMachine::exitProgrammingMode() {
    isOnProgrammingMode = false;
    for (int8_t i = 0; i < m_lenBrewGroups; i++) {
        m_brewGroups[i].setStatusLeds(OFF, ALL);
    }
};

void SimpleFlowMeter::increment() {
    m_pulseCount++;                  //!< Increments flowmeter pulse counter.
    DEBUG4_VALUELN("Pulse Count: ", m_pulseCount)
}

void SimpleFlowMeter::reset() {
    cli();                               //!< going to change interrupt variable(s)
    m_pulseCount=0;                      //!< Prepares the flow meter for a fresh measurement. Resets pulse counter.
    sei();                               //!< done changing interrupt variable(s)
}
