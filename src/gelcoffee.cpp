#include <Arduino.h>
#include <EEPromUtils.h>
#include <ExpressoCoffee.h>

#define DEBUG_LEVEL DEBUG_MID

int8_t
    FLOWMETER_GROUP1_PIN(2), //!< INT0
    FLOWMETER_GROUP2_PIN(3), //!< INT1
    SOLENOID_GROUP1_PIN(A0),
    SOLENOID_GROUP2_PIN(A1);

int8_t GROUP1_PINS[GROUP_PINS_LEN] { 0, 4, 5, 6, 7 }; //!< short single coffee, long single coffee, short double coffee, long double coffee, continuos
int8_t GROUP2_PINS[GROUP_PINS_LEN] { 8, 9, 10, 11, 12 }; //!< short single coffee, long single coffee, short double coffee, long double coffee, continuos

ExpressoMachine* expressoMachine;

void setup()
{
	// Initialize a serial connection for reporting values to the host
    #ifdef DEBUG_LEVEL
        Serial.begin(9600);
        while (!Serial);
        delay(5 * 1000);
    #endif
    
    DEBUG2_PRINTLN("");
    DEBUG2_PRINTLN("");
    DEBUG2_PRINTLN("");
    DEBUG2_PRINTLN("");
    DEBUG2_PRINTLN("");
    DEBUG2_PRINTLN("");
    DEBUG2_PRINTLN("");
    DEBUG2_PRINTLN(".");
    DEBUG2_PRINTLN("Initializing coffee machine module v1.0");

    BrewGroup* groups = new BrewGroup[BREW_GROUPS_LEN] { BrewGroup(1, GROUP1_PINS, FLOWMETER_GROUP1_PIN, SOLENOID_GROUP1_PIN),
    		   BrewGroup(2, GROUP2_PINS, FLOWMETER_GROUP2_PIN, SOLENOID_GROUP2_PIN) };

    // DEBUG3_HEXVALLN("setup() 1 - group 1, address: ", groups[0]);
    // DEBUG3_HEXVALLN("setup() 1 - group 2, address: ", groups[1]);

    ExpressoMachine::init(groups, BREW_GROUPS_LEN);

    expressoMachine = ExpressoMachine::getInstance();
    expressoMachine->setup();

    DEBUG2_PRINTLN("Initialization complete.");
}

void loop()
{

    ExpressoMachine::getInstance()->loop();
}
