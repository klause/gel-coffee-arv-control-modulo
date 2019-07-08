// Arduino Expresso Coffee Machine Classes
// https://github.com/klause/gel-coffee-avr-control-module
// Copyright (C) 2019 by Klause Nascimento and licensed under
// GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html

#include "ExpressoCoffee.h"
#include <EEPromUtils.h>

// #define DEBUG_LEVEL 3

/* Null, because instance will be initialized on demand. */
ExpressoMachine* ExpressoMachine::instance = NULL;

BrewGroup::BrewGroup(int8_t groupNumber, int8_t* pinArray, int8_t fluxometerPin, int8_t solenoidPin, struct DosageRecord dosageConfig) {

    m_groupNumber = groupNumber;
    m_flowMeterPin = fluxometerPin;
    m_solenoidPin = solenoidPin;

    DEBUG3_VALUELN("Instantiating BrewGroup ", m_groupNumber);
    // DEBUG3_HEXVALLN("; address: ", this);

    for (int8_t i = 0; i < GROUP_PINS_LEN; i++)
    {
        long flowMeterPulseConfig = -1;
        long durationConfig = -1;
        if (NON_STOP_BREW_OPTION != i+1) {
            flowMeterPulseConfig = dosageConfig.flowMeterPulseArray[i];
            durationConfig = dosageConfig.durationArray[i] * 1000;
        }
        
        m_brewOptions[i] = BrewOption(pinArray[i], flowMeterPulseConfig, durationConfig, this);
    }
}

/*----------------------------------------------------------------------*
/ this method must be frequently called in loop function to check       *
/ buttons's press, dosage timeouts etc.                                 *
/-----------------------------------------------------------------------*/
void BrewGroup::check()
{
//    if (!m_begin) {
//        DEBUG3_VALUE("begin not called for brew group ", m_groupNumber);
//        DEBUG3_VALUELN("; address: ", this);
//        return;
//    }

//    fprintf(stderr, "%p BrewGroup.groupNumber=%u, .brewingStartTime=%lu, m_ptrCurrentBrewingOption=%p \n", this, m_groupNumber, m_brewingStartTime, m_ptrCurrentBrewingOption);

    BrewOption* bopt = NULL;

    // for each brew option check whether the button was pushed
    for (int8_t i = 0; i < GROUP_PINS_LEN; i++)
    {

        bopt = &m_brewOptions[i];

        int8_t pressed = bopt->wasButtonPressed(m_ptrCurrentBrewingOption == bopt || (isProgramming() && m_ledsBlinkToggle));

        if (BUTTON_PRESSED_FOR_BREWING == pressed)
        {
        	if (bopt->isContinuos() && ExpressoMachine::getInstance()->isProgramming() && !isBrewing()) {
        		// programming button was pressed to exit programmin mode
                exitProgrammingMode();
            } else {
				DEBUG3_VALUE("Button pressed for brewing. btn ", i+1);
				DEBUG3_VALUELN(" on group ", m_groupNumber);
				command(bopt);
				break;
            }
        } else if (BUTTON_PRESSED_FOR_PROGRAM == pressed && !ExpressoMachine::getInstance()->isProgramming()) {
            DEBUG3_VALUE("Button pressed for programming. btn ", i+1);
            DEBUG3_VALUELN(" on group ", m_groupNumber);
            enterProgrammingMode();
        }
    }

    unsigned long currentMillis = millis();
    if (isBrewing())
    {
        m_ptrCurrentBrewingOption->turnOnLed(DELAY_AFTER_ON);

        if (!isProgramming() && !m_ptrCurrentBrewingOption->isContinuos()) {
            /* if flowmeter pulses are not being incremented for malfunction
            * stop brewing based on duration */
            unsigned long elapsedBrewMillis = currentMillis - m_brewingStartTime;
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
            // during leds off, set all pins to INPUT_PULLUP
            setAllInputPullup();
        }
        if (m_ledsBlinkToggle) {
            turnOnAllLeds();
        }
    }
}

int8_t BrewOption::wasButtonPressed(bool setInputPullup)
{

	if (setInputPullup) {
    	pinMode(m_pin, INPUT_PULLUP);
        DEBUG3_VALUELN("Set INPUT_PULLUP for pin", m_pin);
	}
    
    m_btn->read();

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
    if (!m_ptrCurrentBrewingOption && brewOption != NULL) {
        // start brewing
        DEBUG3_VALUELN("Start brewing on group ", m_groupNumber);
        m_ptrCurrentBrewingOption = brewOption;
        m_ptrCurrentBrewingOption->turnOnLed(DELAY_AFTER_ON);
        m_flowMeter->reset();
        m_lastFlowmeterCount = m_flowMeter->getPulseCount();
        m_brewingStartTime = millis();
        m_expressoMachine->turnOffBoilerSolenoid();
        turnOnGroupSolenoid();
        m_expressoMachine->turnOnPump();
    } else {
        // stop brewing
        DEBUG3_VALUE("Stop brewing on group ", m_ptrCurrentBrewingOption->getLedPin());
        DEBUG3_VALUE(", option's pin: ", m_ptrCurrentBrewingOption->getLedPin());
        DEBUG3_VALUELN(". brew time: ", ((millis() - m_brewingStartTime) / 1000));
        // only turn off pump if other groups are not brewing
        m_expressoMachine->turnOffPump(this);
        turnOffGroupSolenoid();
    	m_ptrCurrentBrewingOption->onEndBrewing();
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

void BrewGroup::begin(){
    DEBUG3_VALUELN("begin() on group ", m_groupNumber);
    // DEBUG3_HEXVALLN("; address: ", this);

     pinMode(m_solenoidPin, OUTPUT);
     m_flowMeter = new SimpleFlowMeter(m_flowMeterPin);

    if (m_groupNumber == 1) {
         attachInterrupt(digitalPinToInterrupt(m_flowMeterPin), meterISRGroup1, RISING);
    } else if (m_groupNumber == 2) {
         attachInterrupt(digitalPinToInterrupt(m_flowMeterPin), meterISRGroup2, RISING);
    }

    for (int8_t i = 0; i < GROUP_PINS_LEN; i++)
    {
        m_brewOptions[i].begin();
    }
    m_flowMeter->reset();
    m_begin = true;
}

void BrewGroup::onFlowMeterPulse(){
    if (m_ptrCurrentBrewingOption) {
        m_flowMeter->count();
        DEBUG3_VALUE("Group ", m_groupNumber);
        DEBUG3_VALUELN("flowmeter count: ", m_flowMeter->getPulseCount());
        if(!isProgramming() && m_flowMeter->getPulseCount() >= m_ptrCurrentBrewingOption->getDoseFlowmeterCount()) {
            command(NULL);
        }
    }
}

void BrewGroup::enterProgrammingMode() {
    DEBUG2_PRINT("Entering programming mode");
    m_programmingMode = true;
    m_previousLedsBlinkMillis = millis();
    m_ledsBlinkToggle = true;
}

void BrewGroup::exitProgrammingMode() {
    DEBUG2_PRINT("Exiting programming mode");
    m_programmingMode = false;
}

void BrewGroup::turnOnAllLeds() {
    DEBUG3_VALUELN("Turning all leds ON for group ", m_groupNumber);
    for(int8_t i=0; i<GROUP_PINS_LEN; i++){
        m_brewOptions[i].turnOnLed(DONT_DELAY_AFTER_ON);
    }
    delay(100);
}

void BrewGroup::setAllInputPullup() {
    for (int8_t i = 0; i < GROUP_PINS_LEN;i++)
    {
        pinMode(m_brewOptions[i].getLedPin(), INPUT_PULLUP);
    }
}


void BrewGroup::saveDosageRecord() {

    DEBUG2_VALUELN("Saving dosage record for group ", m_groupNumber);

    DosageRecord rec = DosageRecord();

    for (int8_t i = 0; i < GROUP_PINS_LEN-1; i++) {
        rec.durationArray[i] = m_brewOptions[i].getDoseDuration();
        rec.flowMeterPulseArray[i] = m_brewOptions[i].getDoseFlowmeterCount();
    }
    
    size_t dataLen = sizeof(rec);
    size_t location = 0;
    if (m_groupNumber > 1) {
        location = EEPROM_SIZE( dataLen ) * (m_groupNumber-1);
    }

    EEPROM_init();
    size_t ret = EEPROM_safe_write(location, (uint8_t*) &rec, dataLen);

    #if DEBUG_LEVEL >= DEBUG_LEVEL_LOW
        if (!ret) {
            DEBUG2_VALUE("Success saving dosage record for group ", m_groupNumber);
            DEBUG2_VALUE(". ", dataLen);
            DEBUG2_VALUELN(" bytes writen to EEPROM at location: ", location);
        }
    #endif

    #if DEBUG_LEVEL >= DEBUG_ERROR
        if (ret) {
            DEBUG1_VALUELN("Error saving dosage record. EEPROM_safe_write returned: ", ret);
        }
    #endif
}

void BrewOption::onEndBrewing() {
    m_btn->begin();     //!< reset button status
    if (m_parentBrewGroup->isProgramming()) {
        m_parentBrewGroup->saveDosageRecord();
    }
    delay(800);
}

void BrewOption::turnOnLed(bool delayAfterOn)
{
	// turn LED on if button is released
    if (m_btn->isReleased())
    {
        DEBUG3_VALUELN("Turning ON LED on pin ", m_pin);
        digitalWrite(m_pin, LOW);
        pinMode(m_pin, OUTPUT);
        if (delayAfterOn) delay(100);
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

void ExpressoMachine::begin() {

    DEBUG3_PRINTLN("begin() on ExpressoMachine instance");

     pinMode(PUMP_PIN, OUTPUT);
     pinMode(SOLENOID_BOILER_PIN, OUTPUT);
     pinMode(WATER_LEVEL_PIN, INPUT_PULLUP);

    for (int8_t i = 0; i < m_lenBrewGroups; i++)
    {
        m_brewGroups[i].begin();
    }
    m_begin = true;
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
return false;
}

/*----------------------------------------------------------------------*
/ check whether boiler is being filled with whater,                     *
/ that is, if boiler's soleniod and pump as both on                     *
/-----------------------------------------------------------------------*/
bool ExpressoMachine::isFillingBoiler() {
     return digitalRead(SOLENOID_BOILER_PIN) == HIGH && digitalRead(PUMP_PIN) == HIGH;
    return false;
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

void ExpressoMachine::check() {

//    DEBUG3_PRINTLN("ExpressoMachine::check()\n");

    if (!m_begin) {
        DEBUG3_PRINTLN("begin not called for ExpressoMachine instance");
        return;
    }

    bool isBrewing = false;

    for (int8_t i = 0; i < m_lenBrewGroups; i++) {
//        DEBUG3_VALUE("  Calling check() on group %u; address: %p \n", m_brewGroups[i].getGroupNumber()); DEBUG3_PRINTLN(&m_brewGroups[i]);
        m_brewGroups[i].check();
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
