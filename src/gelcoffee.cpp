#include <Arduino.h>
#include <EEPromUtils.h>

#define FLOWMETER_GROUP1_PIN    2 //!< INT0
#define FLOWMETER_GROUP2_PIN    3 //!< INT1

#define GROUP1_OPTION1_PIN      A0
#define GROUP1_OPTION2_PIN      5
#define GROUP1_OPTION3_PIN      1
#define GROUP1_OPTION4_PIN      0
#define GROUP1_OPTION5_PIN      4

#define WATER_LEVEL_PIN         8

#define GROUP2_OPTION1_PIN      A5
#define GROUP2_OPTION2_PIN      A4
#define GROUP2_OPTION3_PIN      A3
#define GROUP2_OPTION4_PIN      A2
#define GROUP2_OPTION5_PIN      A1

#define SOLENOID_GROUP1_PIN     11
#define SOLENOID_GROUP2_PIN     12
#define SOLENOID_BOILER_PIN     10
#define PUMP_PIN                9

#include <ExpressoCoffee.h>

#include <Debug.h>

const uint8_t FLOWMETER_DEBOUNCE_INVERVAL_MS = 30;

int8_t GROUP1_PINS[BREW_OPTIONS_LEN] { GROUP1_OPTION1_PIN, GROUP1_OPTION2_PIN, GROUP1_OPTION3_PIN, GROUP1_OPTION4_PIN, GROUP1_OPTION5_PIN };    //!< short single coffee, long single coffee, short double coffee, long double coffee, continuous
int8_t GROUP2_PINS[BREW_OPTIONS_LEN] { GROUP2_OPTION1_PIN, GROUP2_OPTION2_PIN, GROUP2_OPTION3_PIN, GROUP2_OPTION4_PIN, GROUP2_OPTION5_PIN };    //!< short single coffee, long single coffee, short double coffee, long double coffee, continuous

ExpressoMachine* expressoMachine;

SimpleFlowMeter flowMeterGroup1;
SimpleFlowMeter flowMeterGroup2;

void meterISRGroup1() {
    static unsigned long lastInterruptMillis = 0;
    volatile unsigned long interruptMillis = millis();
    DEBUG5_PRINTLN("meterISRGroup1()");
    if (interruptMillis - lastInterruptMillis > FLOWMETER_DEBOUNCE_INVERVAL_MS)
    {
        flowMeterGroup1.increment();
    }
    lastInterruptMillis = interruptMillis;
}

void meterISRGroup2() {
    static unsigned long lastInterruptMillis = 0;
    volatile unsigned long interruptMillis = millis();
    DEBUG5_PRINTLN("meterISRGroup2()");
    if (interruptMillis - lastInterruptMillis > FLOWMETER_DEBOUNCE_INVERVAL_MS)
    {
        flowMeterGroup2.increment();
    }
    lastInterruptMillis = interruptMillis;
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

    // start with all leds off
    for (int8_t i = 0; i < BREW_OPTIONS_LEN; i++) {
        pinMode(GROUP1_PINS[i], INPUT_PULLUP);
        DEBUG3_VALUELN("Pin mode INPUT_PULLUP on pin: ", GROUP1_PINS[i]);
        pinMode(GROUP2_PINS[i], INPUT_PULLUP);
        DEBUG3_VALUELN("Pin mode INPUT_PULLUP on pin: ", GROUP2_PINS[i]);
    }

    // turn on 5th led on group 1
    digitalWrite(GROUP1_PINS[CONTINUOUS_BREW_OPTION_INDEX], LOW);
    pinMode(GROUP1_PINS[CONTINUOUS_BREW_OPTION_INDEX], OUTPUT);
    DEBUG3_VALUELN("Pin mode OUTPUT on pin: ", GROUP1_PINS[CONTINUOUS_BREW_OPTION_INDEX]);

    // turn on 5th led on group 2
    digitalWrite(GROUP2_PINS[CONTINUOUS_BREW_OPTION_INDEX], LOW);
    pinMode(GROUP2_PINS[CONTINUOUS_BREW_OPTION_INDEX], OUTPUT);
    DEBUG3_VALUELN("Pin mode OUTPUT on pin: ", GROUP2_PINS[CONTINUOUS_BREW_OPTION_INDEX]);

    // turn on first 4 leds on group 1 for 1 second
    for (int8_t i = 0; i < BREW_OPTIONS_LEN; i++) {
        if (i != CONTINUOUS_BREW_OPTION_INDEX) {
            digitalWrite(GROUP1_PINS[i], LOW);
            pinMode(GROUP1_PINS[i], OUTPUT);
            DEBUG3_VALUELN("Pin mode OUTPUT on pin: ", GROUP1_PINS[i]);
        }
    }
    DEBUG3_PRINTLN("Delay...");
    delay(1000);

    // turn off first 4 leds on group 1
    for (int8_t i = 0; i < BREW_OPTIONS_LEN; i++) {
        if (i != CONTINUOUS_BREW_OPTION_INDEX) {
            pinMode(GROUP1_PINS[i], INPUT_PULLUP);
            DEBUG3_VALUELN("Pin mode INPUT_PULLUP on pin: ", GROUP1_PINS[i]);
        }
    }

    // turn on first 4 leds on group 2 for 1 second
    for (int8_t i = 0; i < BREW_OPTIONS_LEN; i++) {
        if (i != CONTINUOUS_BREW_OPTION_INDEX) {
            digitalWrite(GROUP2_PINS[i], LOW);
            pinMode(GROUP2_PINS[i], OUTPUT);
            DEBUG3_VALUELN("Pin mode OUTPUT on pin: ", GROUP2_PINS[i]);
        }
    }
    DEBUG3_PRINTLN("Delay...");
    delay(1000);

    // turn off first 4 leds on group 2
    for (int8_t i = 0; i < BREW_OPTIONS_LEN; i++) {
        if (i != CONTINUOUS_BREW_OPTION_INDEX) {
            pinMode(GROUP2_PINS[i], INPUT_PULLUP);
            DEBUG3_VALUELN("Pin mode INPUT_PULLUP on pin: ", GROUP2_PINS[i]);
        }
    }
    DEBUG3_PRINTLN("Delay...");
    delay(800);

    // turn off 5th led both groups
    pinMode(GROUP1_PINS[CONTINUOUS_BREW_OPTION_INDEX], INPUT_PULLUP);
    DEBUG3_VALUELN("Pin mode INPUT_PULLUP on pin: ", GROUP1_PINS[CONTINUOUS_BREW_OPTION_INDEX]);
    pinMode(GROUP2_PINS[CONTINUOUS_BREW_OPTION_INDEX], INPUT_PULLUP);
    DEBUG3_VALUELN("Pin mode INPUT_PULLUP on pin: ", GROUP2_PINS[CONTINUOUS_BREW_OPTION_INDEX]);

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

    // Pin 13 connected to ground
    pinMode(13, INPUT);

	// Initialize a serial connection for reporting values to the host
    #if DEBUG_LEVEL > DEBUG_NONE
        Serial.begin(9600);
        while (!Serial);
    #endif


    DEBUG1_VALUELN("FLOWMETER_GROUP1_PIN: ", FLOWMETER_GROUP1_PIN);
    DEBUG1_VALUELN("FLOWMETER_GROUP2_PIN: ", FLOWMETER_GROUP2_PIN);
    DEBUG1_VALUELN("GROUP1_OPTION1_PIN: ", GROUP1_OPTION1_PIN);
    DEBUG1_VALUELN("GROUP1_OPTION2_PIN: ", GROUP1_OPTION2_PIN);
    DEBUG1_VALUELN("GROUP1_OPTION3_PIN: ", GROUP1_OPTION3_PIN);
    DEBUG1_VALUELN("GROUP1_OPTION4_PIN: ", GROUP1_OPTION4_PIN);
    DEBUG1_VALUELN("GROUP1_OPTION5_PIN: ", GROUP1_OPTION5_PIN);
    DEBUG1_VALUELN("WATER_LEVEL_PIN: ", WATER_LEVEL_PIN);
    DEBUG1_VALUELN("GROUP2_OPTION1_PIN: ", GROUP2_OPTION1_PIN);
    DEBUG1_VALUELN("GROUP2_OPTION2_PIN: ", GROUP2_OPTION2_PIN);
    DEBUG1_VALUELN("GROUP2_OPTION3_PIN: ", GROUP2_OPTION3_PIN);
    DEBUG1_VALUELN("GROUP2_OPTION4_PIN: ", GROUP2_OPTION4_PIN);
    DEBUG1_VALUELN("GROUP2_OPTION5_PIN: ", GROUP2_OPTION5_PIN);
    DEBUG1_VALUELN("SOLENOID_GROUP1_PIN: ", SOLENOID_GROUP1_PIN);
    DEBUG1_VALUELN("SOLENOID_GROUP2_PIN: ", SOLENOID_GROUP2_PIN);
    DEBUG1_VALUELN("SOLENOID_BOILER_PIN: ", SOLENOID_BOILER_PIN);
    DEBUG1_VALUELN("PUMP_PIN: ", PUMP_PIN);

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

    expressoMachine = new ExpressoMachine(groups, BREW_GROUPS_LEN, PUMP_PIN, SOLENOID_BOILER_PIN, WATER_LEVEL_PIN);
    expressoMachine->setup();

    sei();
    DEBUG2_PRINTLN("Initialization complete.");
}

void loop()
{
    expressoMachine->loop();
}
