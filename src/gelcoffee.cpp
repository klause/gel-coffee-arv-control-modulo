#include <Arduino.h>
#include <EEPromUtils.h>

#define FLOWMETER_GROUP1_PIN    2 //!< INT0
#define FLOWMETER_GROUP2_PIN    3 //!< INT1

#define GROUP1_OPTION1_PIN      4
#define GROUP1_OPTION2_PIN      5
#define GROUP1_OPTION3_PIN      6
#define GROUP1_OPTION4_PIN      8
#define GROUP1_OPTION5_PIN      7

#define WATER_LEVEL_PIN         9

#define GROUP2_OPTION1_PIN      10
#define GROUP2_OPTION2_PIN      11
#define GROUP2_OPTION3_PIN      12
#define GROUP2_OPTION4_PIN      13
#define GROUP2_OPTION5_PIN      A0

#define SOLENOID_GROUP1_PIN     A3
#define SOLENOID_GROUP2_PIN     A2
#define SOLENOID_BOILER_PIN     A4
#define PUMP_PIN                A5

#include <ExpressoCoffee.h>

#define DEBUG_LEVEL DEBUG_NONE

int8_t GROUP1_PINS[GROUP_PINS_LEN] { GROUP1_OPTION1_PIN, GROUP1_OPTION2_PIN, GROUP1_OPTION3_PIN, GROUP1_OPTION4_PIN, GROUP1_OPTION5_PIN };    //!< short single coffee, long single coffee, short double coffee, long double coffee, continuos
int8_t GROUP2_PINS[GROUP_PINS_LEN] { GROUP2_OPTION1_PIN, GROUP2_OPTION2_PIN, GROUP2_OPTION3_PIN, GROUP2_OPTION4_PIN, GROUP2_OPTION5_PIN };    //!< short single coffee, long single coffee, short double coffee, long double coffee, continuos

ExpressoMachine* expressoMachine;

SimpleFlowMeter flowMeterGroup1;
SimpleFlowMeter flowMeterGroup2;

void meterISRGroup1() {
    DEBUG5_PRINTLN("meterISRGroup1()");
    flowMeterGroup1.increment();
}

void meterISRGroup2() {
    DEBUG5_PRINTLN("meterISRGroup2()");
    flowMeterGroup2.increment();
}

/*----------------------------------------------------------------------*
/ execute initialization routine to blink brew option leds              *
/ 1. turn on 1-5 on group 1 for 1 second                                *
/ 2. turn off 1-4 on group 1 turn on 1-5 on group 2 for 1 second        *
/ 3. turn off 1-4 on group 2 and let 5 on both groups for 300ms         *
/ 4. turn off all leds on both groups                                   *
/-----------------------------------------------------------------------*/
void visualInit() {

    DEBUG2_PRINTLN("Init routine to blink leds");

    int8_t blinkLedsCount = GROUP_PINS_LEN-1;

    // start with all leds off
    for (int8_t i = 0; i < GROUP_PINS_LEN; i++) {
        pinMode(GROUP1_PINS[i], INPUT_PULLUP);
        pinMode(GROUP2_PINS[i], INPUT_PULLUP);
    }

    // turn on 5th led on group 1
    digitalWrite(GROUP1_PINS[blinkLedsCount], LOW);
    pinMode(GROUP1_PINS[blinkLedsCount], OUTPUT);

    // turn on 5th led on group 2
    digitalWrite(GROUP2_PINS[blinkLedsCount], LOW);
    pinMode(GROUP2_PINS[blinkLedsCount], OUTPUT);

    // turn on first 4 leds on group 1 for 1 second
    for (int8_t i = 0; i < blinkLedsCount; i++) {
        digitalWrite(GROUP1_PINS[i], LOW);
        pinMode(GROUP1_PINS[i], OUTPUT);
    }
    delay(1000);
    // turn off first 4 leds on group 1
    for (int8_t i = 0; i < blinkLedsCount; i++) {
        pinMode(GROUP1_PINS[i], INPUT_PULLUP);
    }

    // turn on first 4 leds on group 2 for 1 second
    for (int8_t i = 0; i < blinkLedsCount; i++) {
        digitalWrite(GROUP2_PINS[i], LOW);
        pinMode(GROUP2_PINS[i], OUTPUT);
    }
    delay(1000);
    // turn off first 4 leds on group 2
    for (int8_t i = 0; i < blinkLedsCount; i++) {
        pinMode(GROUP2_PINS[i], INPUT_PULLUP);
    }
    delay(300);
    
    // turn off 5th led both groups
    pinMode(GROUP1_PINS[blinkLedsCount], INPUT_PULLUP);
    pinMode(GROUP2_PINS[blinkLedsCount], INPUT_PULLUP);

    DEBUG2_PRINTLN("End blinking leds");

}


void setup()
{

    // turn off all
    pinMode(PUMP_PIN, OUTPUT);
    digitalWrite(PUMP_PIN, HIGH);
    pinMode(SOLENOID_BOILER_PIN, OUTPUT);
    digitalWrite(SOLENOID_BOILER_PIN, HIGH);
    pinMode(SOLENOID_GROUP1_PIN, OUTPUT);
    digitalWrite(SOLENOID_GROUP1_PIN, HIGH);
    pinMode(SOLENOID_GROUP2_PIN, OUTPUT);
    digitalWrite(SOLENOID_GROUP2_PIN, HIGH);

	// Initialize a serial connection for reporting values to the host
    #ifdef DEBUG_LEVEL
        Serial.begin(9600);
        while (!Serial);
        delay(2 * 1000);
    #endif
    
    visualInit();

    DEBUG2_PRINTLN("");
    DEBUG2_PRINTLN("");
    DEBUG2_PRINTLN("");
    DEBUG2_PRINTLN(".");
    DEBUG2_PRINTLN("Initializing coffee machine module v1.0");

    cli();

    attachInterrupt(digitalPinToInterrupt(FLOWMETER_GROUP1_PIN), meterISRGroup1, RISING);
    attachInterrupt(digitalPinToInterrupt(FLOWMETER_GROUP2_PIN), meterISRGroup2, RISING);

    BrewGroup* groups = new BrewGroup[BREW_GROUPS_LEN] {
        BrewGroup(1, GROUP1_PINS, &flowMeterGroup1, SOLENOID_GROUP1_PIN),
    	BrewGroup(2, GROUP2_PINS, &flowMeterGroup2, SOLENOID_GROUP2_PIN)
    };

    expressoMachine = new ExpressoMachine(groups, BREW_GROUPS_LEN);
    expressoMachine->setup();

    sei();
    DEBUG2_PRINTLN("Initialization complete.");
}

void loop()
{
    expressoMachine->loop();
}
