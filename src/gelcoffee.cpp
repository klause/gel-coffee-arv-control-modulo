#include <Arduino.h>
#include <EEPromUtils.h>
#include <ExpressoCoffee.h>

// #define DEBUG_LEVEL 3
#include <Debug.h>

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
    #endif
    
    delay(5 * 1000);

    DEBUG2_PRINTLN("");
    DEBUG2_PRINTLN("");
    DEBUG2_PRINTLN("");
    DEBUG2_PRINTLN("");
    DEBUG2_PRINTLN("");
    DEBUG2_PRINTLN("");
    DEBUG2_PRINTLN("");
    DEBUG2_PRINTLN(".");
    DEBUG2_PRINTLN("Initializing coffee machine module v1.0");

    DosageRecord groupDosageRecords[BREW_GROUPS_LEN] = { DosageRecord(), DosageRecord() };

    size_t location = 0;
    int8_t ret;
    if (EEPROM_init()) {
        DEBUG3_PRINTLN("Loading dosage config from EEPROM");
        for (int8_t i = 0; i < BREW_GROUPS_LEN; i++) {
            
            size_t dataLen = sizeof(DosageRecord);
            size_t location = EEPROM_SIZE( dataLen ) * (i);

            ret = EEPROM_safe_read(location, (uint8_t*) &groupDosageRecords[i], sizeof(dataLen));

            #if DEBUG_LEVEL >= DEBUG_ERROR
                if (ret) {
                    DEBUG1_VALUE("Error reading dosage record for group ", i+1);
                    DEBUG1_VALUE(" at location: ", location);
                    DEBUG1_VALUELN(". EEPROM_safe_write returned: ", ret);
                }
            #endif
        }
    }

    if (location <= 0) {
        for (int8_t i = 0; i < BREW_GROUPS_LEN; i++) {
            groupDosageRecords[i] = DosageRecord();
        }
    }

    #if DEBUG_LEVEL >= 3
        for (int8_t i = 0; i < BREW_GROUPS_LEN; i++) {
            groupDosageRecords[i] = DosageRecord();
            Serial.print(F("Dosage config for group "));
            Serial.println(i);
            Serial.print(F("  flowMeterPulseArray = [ "));
            Serial.print(groupDosageRecords[i].flowMeterPulseArray[0]);
            Serial.print(F(", "));
            Serial.print(groupDosageRecords[i].flowMeterPulseArray[1]);
            Serial.print(F(", "));
            Serial.print(groupDosageRecords[i].flowMeterPulseArray[2]);
            Serial.print(F(", "));
            Serial.print(groupDosageRecords[i].flowMeterPulseArray[3]);
            Serial.println(F(" ]"));
        }
    #endif

    BrewGroup* groups = new BrewGroup[BREW_GROUPS_LEN] { BrewGroup(1, GROUP1_PINS, FLOWMETER_GROUP1_PIN, SOLENOID_GROUP1_PIN, groupDosageRecords[0]),
    		   BrewGroup(2, GROUP2_PINS, FLOWMETER_GROUP1_PIN, SOLENOID_GROUP2_PIN, groupDosageRecords[1]) };

    // DEBUG3_HEXVALLN("setup() 1 - group 1, address: ", groups[0]);
    // DEBUG3_HEXVALLN("setup() 1 - group 2, address: ", groups[1]);

    ExpressoMachine::init(groups, BREW_GROUPS_LEN);

    expressoMachine = ExpressoMachine::getInstance();
    expressoMachine->begin();

    DEBUG2_PRINTLN("Initialization complete.");
}

void loop()
{

    ExpressoMachine::getInstance()->check();
}
