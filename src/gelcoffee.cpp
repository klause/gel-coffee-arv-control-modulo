#include <Arduino.h>
#include <EEPromUtils.h>
#include <ExpressoCoffee.h>

#define DEBUG_LEVEL DEBUG_MID

int8_t
    FLOWMETER_GROUP1_PIN(2), //!< INT0
    FLOWMETER_GROUP2_PIN(3), //!< INT1
    SOLENOID_GROUP1_PIN(A0),
    SOLENOID_GROUP2_PIN(A1);

int8_t GROUP1_PINS[GROUP_PINS_LEN] { 0, 4, 5, 6, 7 };       //!< short single coffee, long single coffee, short double coffee, long double coffee, continuos
int8_t GROUP2_PINS[GROUP_PINS_LEN] { 8, 9, 10, 11, 12 };    //!< short single coffee, long single coffee, short double coffee, long double coffee, continuos

ExpressoMachine* expressoMachine;

SimpleFlowMeter flowMeterGroup1;
SimpleFlowMeter flowMeterGroup2;

void meterISRGroup1() {
    DEBUG3_PRINTLN("meterISRGroup1()");
    flowMeterGroup1.increment();
}

void meterISRGroup2() {
    DEBUG3_PRINTLN("meterISRGroup2()");
    flowMeterGroup1.increment();
}

void setup()
{
	// Initialize a serial connection for reporting values to the host
    #ifdef DEBUG_LEVEL
        Serial.begin(9600);
        while (!Serial);
        delay(2 * 1000);
    #endif
    
    DEBUG2_PRINTLN("");
    DEBUG2_PRINTLN("");
    DEBUG2_PRINTLN("");
    DEBUG2_PRINTLN(".");
    DEBUG2_PRINTLN("Initializing coffee machine module v1.0");

    attachInterrupt(digitalPinToInterrupt(FLOWMETER_GROUP1_PIN), meterISRGroup1, RISING);
    attachInterrupt(digitalPinToInterrupt(FLOWMETER_GROUP2_PIN), meterISRGroup2, RISING);

    BrewGroup* groups = new BrewGroup[BREW_GROUPS_LEN] {
        BrewGroup(1, GROUP1_PINS, &flowMeterGroup1, SOLENOID_GROUP1_PIN),
    	BrewGroup(2, GROUP2_PINS, &flowMeterGroup2, SOLENOID_GROUP2_PIN)
    };

    expressoMachine = new ExpressoMachine(groups, BREW_GROUPS_LEN);
    expressoMachine->setup();

    DEBUG2_PRINTLN("Initialization complete.");
}

void loop()
{
    expressoMachine->loop();
}
