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

/*----------------------------------------------------------------------*
/ execute initialization routine to blink brew option leds              *
/ 1. turn on 1-5 on group 1 for 1 second                                *
/ 2. turn off 1-4 on group 1 turn on 1-5 on group 2 for 1 second        *
/ 3. turn off 1-4 on group 2 and let 5 on both groups for 300ms         *
/ 4. turn off all leds on both groups                                   *
/-----------------------------------------------------------------------*/
void visualInit() {

    int8_t blinkLedsCount = GROUP_PINS_LEN-1;

    for (int8_t i = 0; i < blinkLedsCount; i++) {
        pinMode(GROUP1_PINS[i], INPUT_PULLUP);
        pinMode(GROUP2_PINS[i], INPUT_PULLUP);
    }

    digitalWrite(GROUP1_PINS[blinkLedsCount], LOW);
    digitalWrite(GROUP2_PINS[blinkLedsCount], LOW);

    pinMode(GROUP1_PINS[blinkLedsCount], OUTPUT);
    pinMode(GROUP2_PINS[blinkLedsCount], OUTPUT);

    for (int8_t i = 0; i < blinkLedsCount; i++) {
        digitalWrite(GROUP1_PINS[i], LOW);
        pinMode(GROUP1_PINS[i], OUTPUT);
    }
    delay(1000);
    for (int8_t i = 0; i < blinkLedsCount; i++) {
        pinMode(GROUP1_PINS[i], INPUT_PULLUP);
    }
    for (int8_t i = 0; i < blinkLedsCount; i++) {
        digitalWrite(GROUP2_PINS[i], LOW);
        pinMode(GROUP2_PINS[i], OUTPUT);
    }
    delay(1000);
    for (int8_t i = 0; i < blinkLedsCount; i++) {
        pinMode(GROUP2_PINS[i], INPUT_PULLUP);
    }
    delay(300);
    pinMode(GROUP1_PINS[blinkLedsCount], INPUT_PULLUP);
    pinMode(GROUP2_PINS[blinkLedsCount], INPUT_PULLUP);
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
