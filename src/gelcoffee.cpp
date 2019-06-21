#include <Arduino.h>
#include <JC_Button.h>          // https://github.com/JChristensen/JC_Button
#include <EEPROM.h>

#include <ExpressoCoffee.h>          // https://github.com/JChristensen/JC_Button

// groupNumber 1 button/led pins
byte grp1OneShortPin = 1; // one short espresso
byte grp1OneLongPin = 2; // one long espresso
byte grp1TwoShortPin = 3; // tow short espressos
byte grp1TwoLongPin = 4; // tow long espressos
byte grp1ContinuosPin = 5; // continuos brewing

// groupNumber 2 button/led pins
byte grp2OneShortPin = 6; // one short espresso
byte grp2OneLongPin = 7; // one long espresso
byte grp2TwoShortPin = 8; // tow short espressos
byte grp2TwoLongPin = 9; // tow long espressos
byte grp2ContinuosPin = 10; // continuos brewing

byte
  FLOWMETER_GROUP1_PIN(2),
  FLOWMETER_GROUP2_PIN(3),
  SOLENOID_BOILER_PIN(12),
  PUMP_PIN(13),
  SOLENOID_GROUP1_PIN(A0),
  SOLENOID_GROUP2_PIN(A1),
  WATER_LEVEL_PIN(A2);

byte GROUP1_PINS[5] {0, 1, 4,  5,  6};
byte GROUP2_PINS[5] {7, 8, 9, 10, 11};

Button buttons[10];
// byte buttonPins[10] = {1,2,3,4,5,6,7,8,9,10};
bool buttonEnabledArray[10] = {0,0,0,0,0,0,0,0,0,0};
bool buttonLastStateArray[10] = {HIGH,HIGH,HIGH,HIGH,HIGH,HIGH,HIGH,HIGH,HIGH,HIGH};

byte buttonGrp1Down = -1;                     //!< which button is pressed down on groupNumber 1
byte buttonGrp2Down = -1;                     //!< which button is pressed down on groupNumber 2

bool buttonEnabled;

byte pinOfActiveOptionOnGroup1 = -1;
byte pinOfActiveOptionOnGroup2 = -1;

long lastPressedTime = -1;

BrewGroup brewGroup1;
BrewGroup brewGroup2;

void setup() {
  // Initialize a serial connection for reporting values to the host
  Serial.begin(38400);
  Serial.println("Initializing coffee machine...");

  for (size_t pin = 1; pin <= 10; pin++) {
    buttons[pin-1] = Button(pin);
  }

  DosageRecord group1DosageRecord;
  DosageRecord group2DosageRecord;

  EEPROM.get(0, group1DosageRecord);
  EEPROM.get(sizeof(DosageRecord), group2DosageRecord);

  brewGroup1 = BrewGroup(1, GROUP1_PINS, FLOWMETER_GROUP1_PIN, SOLENOID_GROUP1_PIN, PUMP_PIN);
  brewGroup2 = BrewGroup(2, GROUP1_PINS, FLOWMETER_GROUP1_PIN, SOLENOID_GROUP1_PIN, PUMP_PIN);

  Serial.println("Initialization complete. ");
}

void loop() {

  brewGroup1.check();
  brewGroup2.check();


  if (brewGroup1.currentOptionBrewing() > 0) {
    turnOnLed(pinOfActiveOptionOnGroup1);
  }
  if (pinOfActiveOptionOnGroup2 > 0) {
    turnOnLed(pinOfActiveOptionOnGroup2);
  }

}

void commandGroup(byte groupNumber, bool toggleState) {

  byte groupPin = groupNumber == 1 ? RELAY_GROUP1_PIN : RELAY_GROUP2_PIN;

  if (toggleState) {
    Serial.println("Start brewing on groupNumber " + groupNumber);
    digitalWrite(groupPin, HIGH);
  } else {
    Serial.println("Stop brewing on groupNumber " + groupNumber);
    digitalWrite(groupPin, LOW);
  }

}

void handleButtonToggle(byte buttonPin, Button btn) {

    if (buttonPin <= 5) {
      pinOfActiveOptionOnGroup1 = !buttonEnabled ? buttonPin : -1;
      commandGroup(1, btn.toggleState());
    } else {
      pinOfActiveOptionOnGroup2 = !buttonEnabled ? buttonPin : -1;
      commandGroup(2, btn.toggleState());
    }

}