// Arduino Expresso Coffee Machine Classes
// https://github.com/klause/gel-coffee-avr-control-module
// Copyright (C) 2019 by Klause Nascimento and licensed under
// GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html

#include "ExpressoCoffee.h"
#include <EEPromUtils.h>

/* Null, because instance will be initialized on demand. */
ExpressoMachine* ExpressoMachine::instance = NULL;

BrewGroup::BrewGroup(int8_t groupNumber, int8_t pinArray[], int8_t fluxometerPin, int8_t solenoidPin) {

    m_groupNumber = groupNumber;
    m_flowMeterPin = fluxometerPin;
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

    BrewOption* bopt = NULL;

    // for each brew option check whether the button was pushed
    for (int8_t i = 0; i < GROUP_PINS_LEN; i++)
    {

        bopt = &m_brewOptions[i];

        /* INPUT_PULLUP need to be set before reading button valeu when:
         * - Brewing is ocurring for that option
         * - Is in prog mode and blink toggled flag is ON
         * - Is in prog mode and that option was already programmed
         */
        // int8_t pressed = bopt->wasButtonPressed(m_ptrCurrentBrewingOption == bopt || (isProgramming() && ( m_ledsBlinkToggle || bopt->isProgrammed() )) );
        int8_t pressed = bopt->loop();

        // 
        /* If brewing, only process button command for the current brewing option,
           pressing other buttons on the group is ignored */
        if (isBrewing() && m_ptrCurrentBrewingOption != bopt) {
            continue;
        }

        /* Short pressing brewing option is for:
         * - Start brewing (out of prog mode) if group is not brewing for other option
         * - Stop brewing (out of prog mode) if pressed option is currently brewing
         * - Start brewing (in prog mode) if group is not brewing for other option
         * - Stop brewing (in prog mode) if pressed option is currently brewing
         * - Exit programming mode (continuos button) if this mode is active
         */
        if (BUTTON_PRESSED_FOR_BREWING == pressed)
        {

            if (bopt->isContinuos() && isProgramming()) {
                // programming button was pressed to exit programmin mode
                DEBUG3_VALUELN("Button pressed to exit programming mode on group ", m_groupNumber);
                exitProgrammingMode();
                break;  //!< Exit for loop since no other command will be detected so fast in this group
            } else {
				DEBUG3_VALUE("Button pressed for brewing. Option ", i+1);
				DEBUG3_VALUELN(" on group ", m_groupNumber);
				command(bopt);
                break;  //!< Exit for loop since no other command will be detected so fast in this group
            }

        } else if (BUTTON_PRESSED_FOR_PROGRAM == pressed && !ExpressoMachine::getInstance()->isProgramming()) {
            DEBUG3_VALUELN("Button pressed to enter programming mode on group ", m_groupNumber);
            enterProgrammingMode();
            break;  //!< Exit for loop since no other command will be detected so fast in this group

        // } else if(isProgramming() && bopt->isProgrammed() && !m_ledsBlinkToggle) { //!< Keep led ON to indicate that the coffee option was already programmed
        //     bopt->setLedStatus(ON);
        }

    }

    unsigned long currentMillis = millis();
    if (isBrewing())
    {
        if (!isProgramming() && !m_ptrCurrentBrewingOption->isContinuos()) {
            /* if flowmeter pulses are not being incremented for malfunction
            * stop brewing based on duration */
            long elapsedBrewMillis = currentMillis - m_brewingStartTime;
            if (m_lastFlowmeterCount <= m_ptrCurrentBrewingOption->getDoseFlowmeterCount() &&
                elapsedBrewMillis >= m_ptrCurrentBrewingOption->getDoseDuration()) {
            		DEBUG3_PRINTLN("Brewing timed out by duration, stop brewing...");
                    // command group to stop brewing
                    command(m_ptrCurrentBrewingOption);
            }
        }
    } else if (isProgramming()) {
        if ((currentMillis - m_previousLedsBlinkMillis) >= LEDS_BLINK_INTERVAL) {
            m_ledsBlinkToggle = !m_ledsBlinkToggle;
            m_previousLedsBlinkMillis = currentMillis;
            setStatusLeds(m_ledsBlinkToggle ? ON : OFF, ONLY_NOT_PROGRAMMED);
        }
    }
}

ButtonAction BrewOption::loop()
{

	if (m_pinMode != INPUT_PULLUP) {
    	pinMode(m_pin, INPUT_PULLUP);
        m_pinMode = INPUT_PULLUP;
        DEBUG5_VALUELN("Set INPUT_PULLUP for pin", m_pin);
	}
    
    m_btn->read();

    if (m_ledStatus == ON) {
        turnOnLed(true);
    }

    // if this is the programming button, check whether it was pressed long enough
    if (isContinuos() && m_btn->pressedFor(MILLIS_TO_ENTER_PROGRAM_MODE)) {
        m_btnReleasedAfterPressedForProgram = false;
        return BUTTON_PRESSED_FOR_PROGRAM;
    } else if (m_btn->wasReleased()) {
        if (m_btnReleasedAfterPressedForProgram) {
            return BUTTON_PRESSED_FOR_BREWING;
        } else {
            m_btnReleasedAfterPressedForProgram = true;
        }
    }

    return BUTTON_NOT_PRESSED;;
}


void BrewGroup::command(BrewOption* brewOption) {
    if (m_ptrCurrentBrewingOption == NULL && brewOption != NULL) {
        // start brewing
        DEBUG3_VALUELN("Start brewing on group ", m_groupNumber);
        m_ptrCurrentBrewingOption = brewOption;
        setStatusLeds(OFF, ALL);
        m_ptrCurrentBrewingOption->onStartBrewing();
        m_flowMeter->reset();
        m_lastFlowmeterCount = 0;
        m_brewingStartTime = millis();
        m_expressoMachine->turnOffBoilerSolenoid();
        turnOnGroupSolenoid();
        m_expressoMachine->turnOnPump();
    } else if (m_ptrCurrentBrewingOption == brewOption || brewOption == NULL) {
        // stop brewing
        DEBUG3_VALUE("Stop brewing on group ", m_groupNumber);
        DEBUG3_VALUELN(". Brew time: ", ((millis() - m_brewingStartTime) / 1000));
        // only turn off pump if other groups are not brewing
        m_expressoMachine->turnOffPump(this);
        turnOffGroupSolenoid();
    	m_ptrCurrentBrewingOption->onEndBrewing();
        if (isProgramming())
        {
            setStatusLeds(ON, ONLY_PROGRAMMED);
        }
        m_ptrCurrentBrewingOption = NULL;
    }
}

void BrewGroup::turnOnGroupSolenoid() {
    DEBUG3_VALUELN("Turning ON solenoid of group ", m_groupNumber);
    digitalWrite(m_solenoidPin, HIGH);
}

void BrewGroup::turnOffGroupSolenoid() {
    DEBUG3_VALUELN("Turning OFF solenoid of group ", m_groupNumber);
     digitalWrite(m_solenoidPin, LOW);
}

void meterISRGroup1() {
    DEBUG3_PRINTLN("meterISRGroup1()");
    BrewGroup* bg1 = ExpressoMachine::getInstance()->getBrewGroup(1);
    if (bg1 != NULL) {
        bg1->onFlowMeterPulse();
    }
}

void meterISRGroup2() {
    DEBUG3_PRINTLN("meterISRGroup2()");
    BrewGroup* bg2 = ExpressoMachine::getInstance()->getBrewGroup(2);
    if (bg2 != NULL) {
        bg2->onFlowMeterPulse();
    }
}

void BrewGroup::setup(){

    DEBUG3_VALUELN("setup() on group ", m_groupNumber);

    DosageRecord dosageConfig = loadDosageRecord();

    for (int8_t i = 0; i < GROUP_PINS_LEN; i++)
    {
        long flowMeterPulseConfig = 0;
        long durationConfig = 0;
        if (NON_STOP_BREW_OPTION != i+1) {
            flowMeterPulseConfig = dosageConfig.flowMeterPulseArray[i];
            durationConfig = dosageConfig.durationArray[i] * 1000;
        }
        
        m_brewOptions[i] = BrewOption(m_brewOptionPins[i], flowMeterPulseConfig, durationConfig, this);
    }

    pinMode(m_solenoidPin, OUTPUT);
    m_flowMeter = new SimpleFlowMeter(m_flowMeterPin);

    if (m_groupNumber == 1) {
         attachInterrupt(digitalPinToInterrupt(m_flowMeterPin), meterISRGroup1, RISING);
    } else if (m_groupNumber == 2) {
         attachInterrupt(digitalPinToInterrupt(m_flowMeterPin), meterISRGroup2, RISING);
    }

    for (int8_t i = 0; i < GROUP_PINS_LEN; i++)
    {
        m_brewOptions[i].setup();
    }
    m_flowMeter->reset();
    m_flagSetup = true;
}

void BrewGroup::onFlowMeterPulse(){
    if (m_ptrCurrentBrewingOption) {
        m_flowMeter->increment();
        m_lastFlowmeterCount = m_flowMeter->getPulseCount();
        DEBUG3_VALUE("Group ", m_groupNumber);
        DEBUG3_VALUELN(" flowmeter count: ", m_lastFlowmeterCount);
        if(!isProgramming() && m_lastFlowmeterCount >= m_ptrCurrentBrewingOption->getDoseFlowmeterCount()) {
            DEBUG3_VALUE("Flow count reached (", m_lastFlowmeterCount);
            DEBUG3_PRINTLN("), commanding brewing to stop");
            command(NULL);
        }
    }
}

void BrewGroup::enterProgrammingMode() {
    DEBUG2_PRINT("Entering programming mode");
    m_programmingMode = true;
    m_previousLedsBlinkMillis = millis();
    m_ledsBlinkToggle = true;
    for (int8_t i = 0; i < GROUP_PINS_LEN; i++)
    {
        m_brewOptions[i].setProgrammed(false);
    }
    
}

void BrewGroup::exitProgrammingMode() {
    DEBUG2_PRINT("Exiting programming mode");
    m_programmingMode = false;
    for (int8_t i = 0; i < GROUP_PINS_LEN;i++)
    {
        m_brewOptions[i].setProgrammed(false);
        m_brewOptions[i].setLedStatus(OFF);
    }
}

void BrewGroup::setStatusLeds(LedStatus s, FilterOption filter) {
    DEBUG4_VALUE("Setting leds of group ", m_groupNumber);
    DEBUG4_VALUELN(" to ", s == ON ? "ON" : "OFF" );
    
    for(int8_t i=0; i<GROUP_PINS_LEN; i++){
        if ( (filter == ONLY_PROGRAMMED && m_brewOptions[i].isProgrammed())
            || (filter == ONLY_NOT_PROGRAMMED && !m_brewOptions[i].isProgrammed())
            || filter == ALL ) {
            DEBUG5_VALUELN("  -> brew option ", i+1 );
            m_brewOptions[i].setLedStatus(s);
        }
    }
}

// void BrewGroup::setAllInputPullup() {
//     for (int8_t i = 0; i < GROUP_PINS_LEN;i++)
//     {
//         pinMode(m_brewOptions[i].getLedPin(), INPUT_PULLUP);
//     }
// }

DosageRecord BrewGroup::loadDosageRecord() {

    DosageRecord rec = DosageRecord();

    if (EEPROM_init()) {
        DEBUG3_VALUELN("Loading dosage config from EEPROM for group ", m_groupNumber);
        size_t dataLen = sizeof(DosageRecord);
        size_t location = EEPROM_SIZE( dataLen ) * (m_groupNumber-1);

        #if DEBUG_LEVEL >= DEBUG_ERROR
            int8_t ret = EEPROM_safe_read(location, (uint8_t*) &rec, dataLen);
            if (ret < 0) {
                DEBUG1_VALUE("Error reading dosage record for group ", m_groupNumber);
                DEBUG1_VALUE(" at location: ", location);
                DEBUG1_VALUELN(". EEPROM_safe_write returned: ", ret);
            }
        #else
            EEPROM_safe_read(location, (uint8_t*) &rec, dataLen);
        #endif

        if (ret < 0) {
            rec = DosageRecord();
        }

    }

    #if DEBUG_LEVEL >= 3
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

    for (int8_t i = 0; i < GROUP_PINS_LEN-1; i++) {
        rec.durationArray[i] = m_brewOptions[i].getDoseDuration() / 1000;
        rec.flowMeterPulseArray[i] = m_brewOptions[i].getDoseFlowmeterCount();
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

                DEBUG4_PRINT("  [");
                for (size_t i = 0; i < dataLen; i++)
                {
                    DEBUG4_HEXVAL(" ", ((uint8_t*) &rec)[i]);
                }
                DEBUG4_PRINTLN(" ]");
            }
        #endif

        if (ret < 0) {
            DEBUG1_VALUELN("Error saving dosage record. EEPROM_safe_write returned: ", ret);
        }
    #else
        EEPROM_safe_write(location, (uint8_t*) &rec, dataLen);
    #endif

}

void BrewOption::onEndBrewing() {
    DEBUG3_VALUELN("End brewing. Option's pin: ", m_pin);

    m_btn->begin();     //!< reset button status
    if (m_parentBrewGroup->isProgramming()) {
        m_doseDuration = millis() - m_parentBrewGroup->getBrewingStartTime();
        m_doseFlowmeterCount = m_parentBrewGroup->getLastFlowmeterCount();
        m_parentBrewGroup->saveDosageRecord();
        m_flagProgrammed = true;
        m_ledStatus = ON;
    } else {
        m_ledStatus = OFF;
    }
    delay(100);
}

void BrewOption::onStartBrewing() {
    if (m_parentBrewGroup->isProgramming()) {
        setProgrammed(false);
    }
    m_ledStatus = ON;
}

void BrewOption::turnOnLed(bool delayAfterOn)
{
	// turn LED on if button is released
    if (m_btn->isReleased())
    {
        DEBUG5_VALUELN("Turning ON LED on pin ", m_pin);
        digitalWrite(m_pin, LOW);
        pinMode(m_pin, OUTPUT);
        m_pinMode = OUTPUT;
        // if (delayAfterOn) delay(100);
    }
}

// void ExpressoMachine::loadDosageConfig() {

//     if (EEPROM_init()) {
//         size_t location = 0;
//         for (int8_t i = 0; i < m_lenBrewGroups; i++)
//         {
//             DosageRecord groupDosageRecord = DosageRecord();
//             location = EEPROM_safe_read(location, groupDosageRecord.durationArray, sizeof(groupDosageRecord.durationArray));
//             if (location > 0) {
//                 location = EEPROM_safe_read(location, groupDosageRecord.flowMeterPulseArray, sizeof(groupDosageRecord.flowMeterPulseArray));
//                 if (location > 0) {
//                     m_brewGroups[i].setDosageConfig(groupDosageRecord);
//                 } else {
//                     return;
//                 }
//             } else
//             {
//                 return;
//             }
            
//         }
//     }

// }

void ExpressoMachine::setup() {

    DEBUG4_PRINTLN("setup() on ExpressoMachine instance");

     pinMode(PUMP_PIN, OUTPUT);
     pinMode(SOLENOID_BOILER_PIN, OUTPUT);
     pinMode(WATER_LEVEL_PIN, INPUT_PULLUP);

    for (int8_t i = 0; i < m_lenBrewGroups; i++)
    {
        m_brewGroups[i].setup();
    }
    m_flagSetup = true;
}

void ExpressoMachine::turnOnPump() {
    DEBUG3_PRINTLN("Turning ON pump");
     digitalWrite(PUMP_PIN, HIGH);
}

/*----------------------------------------------------------------------*
/ turn off water pump only if there is no other group brewing           *
/-----------------------------------------------------------------------*/
void ExpressoMachine::turnOffPump(BrewGroup* brewGroupAsking) {
    
    for (int8_t i = 0; i < m_lenBrewGroups; i++)
    {
        if (m_brewGroups[i].isBrewing() && brewGroupAsking != &m_brewGroups[i])
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
    digitalWrite(PUMP_PIN, LOW);
}

void ExpressoMachine::turnOnBoilerSolenoid() {
    DEBUG3_PRINTLN("Turning ON boiler solenoid");
     digitalWrite(SOLENOID_BOILER_PIN, HIGH);
}
void ExpressoMachine::turnOffBoilerSolenoid() {
    DEBUG3_PRINTLN("Turning OFF boiler solenoid");
     digitalWrite(SOLENOID_BOILER_PIN, LOW);
}

bool ExpressoMachine::isProgramming() {
	for (int i = 0; i < BREW_GROUPS_LEN; ++i) {
		if (m_brewGroups[i].isProgramming()) {
			return true;
		}
	}
	return false;
}

/*----------------------------------------------------------------------*
/ check current value of boiler's water low level sensor.               *
/ return true if level is low otherwise false                           *
/-----------------------------------------------------------------------*/
bool ExpressoMachine::isBoilerWaterLevelLow() {
	return digitalRead(WATER_LEVEL_PIN) == LOW;
}

/*----------------------------------------------------------------------*
/ check whether boiler is being filled with whater,                     *
/ that is, if boiler's soleniod and pump as both on                     *
/-----------------------------------------------------------------------*/
bool ExpressoMachine::isFillingBoiler() {
     return digitalRead(SOLENOID_BOILER_PIN) == HIGH && digitalRead(PUMP_PIN) == HIGH;
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

    DEBUG5_PRINTLN("ExpressoMachine::loop()\n");

    if (!m_flagSetup) {
        DEBUG3_PRINTLN("setup not called for ExpressoMachine instance");
        return;
    }

    bool isBrewing = false;

    for (int8_t i = 0; i < m_lenBrewGroups; i++) {
        m_brewGroups[i].loop();
        isBrewing = m_brewGroups[i].isBrewing() || isBrewing;
    }

  // if groups are not brewing
  if (!isBrewing) {
    bool lowLevel = isBoilerWaterLevelLow();
    bool isFilling = isFillingBoiler();

    // check if bolier needs more water
    if (lowLevel && !isFilling) {
      // bolier water level is low, start filling
      DEBUG3_PRINTLN("Starting to fill the boiler");
      turnOnBoilerSolenoid();
      turnOnPump();
    } else if (isFilling && !lowLevel) {
      // stop filling if water level is OK now
      DEBUG3_PRINTLN("Stopping to fill the boiler");
      turnOffPump();
      turnOffBoilerSolenoid();
    }

  }

}

void SimpleFlowMeter::increment() {
    unsigned long currMillis = millis();
    if ((currMillis - m_lastPulseMs) > 80) {
        m_pulseCount++;                  //!< Increments flowmeter pulse counter.
    }
    m_lastPulseMs = currMillis;
}
