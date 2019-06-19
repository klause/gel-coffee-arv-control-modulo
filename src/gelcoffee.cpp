#include <Arduino.h>
#include <JC_Button.h>          // https://github.com/JChristensen/JC_Button

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

const byte
  RELAY_GROUP1_PIN(11),
  RELAY_GROUP2_PIN(12);

ToggleButtonLed buttons[10];
// byte buttonPins[10] = {1,2,3,4,5,6,7,8,9,10};
bool buttonEnabledArray[10] = {0,0,0,0,0,0,0,0,0,0};
bool buttonLastStateArray[10] = {HIGH,HIGH,HIGH,HIGH,HIGH,HIGH,HIGH,HIGH,HIGH,HIGH};

byte buttonGrp1Down = -1;                     //!< which button is pressed down on groupNumber 1
byte buttonGrp2Down = -1;                     //!< which button is pressed down on groupNumber 2

bool buttonEnabled;

byte pinOfActiveOptionOnGroup1 = -1;
byte pinOfActiveOptionOnGroup2 = -1;

long lastPressedTime = -1;

void setup() {
  // Initialize a serial connection for reporting values to the host
  Serial.begin(38400);
  Serial.println("Initializing coffee machine...");

  for (size_t pin = 1; pin <= 10; pin++) {
    buttons[pin-1] = ToggleButtonLed(pin);
  }

  // pinMode(grp1OneShortPin, INPUT_PULLUP);
  // pinMode(grp1OneLongPin, INPUT_PULLUP);
  // pinMode(grp1TwoShortPin, INPUT_PULLUP);
  // pinMode(grp1TwoLongPin, INPUT_PULLUP);
  // pinMode(grp1ContinuosPin, INPUT_PULLUP);

  // pinMode(grp2OneShortPin, INPUT_PULLUP);
  // pinMode(grp2OneLongPin, INPUT_PULLUP);
  // pinMode(grp2TwoShortPin, INPUT_PULLUP);
  // pinMode(grp2TwoLongPin, INPUT_PULLUP);
  // pinMode(grp2ContinuosPin, INPUT_PULLUP);

  Serial.println("Initialization complete. ");
}

void loop() {

  // for each button check wheter it was toggle
  for (byte i = 0; i < 10; i++) {
    ToggleButton btn = buttons[i];
    btn.read();
    if (btn.changed()){
      handleButtonToggle(i+1, btn);
    }
  }

  if (pinOfActiveOptionOnGroup1 > 0) {
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

void handleButtonToggle(byte buttonPin, ToggleButton btn) {

    if (buttonPin <= 5) {
      pinOfActiveOptionOnGroup1 = !buttonEnabled ? buttonPin : -1;
      commandGroup(1, btn.toggleState());
    } else {
      pinOfActiveOptionOnGroup2 = !buttonEnabled ? buttonPin : -1;
      commandGroup(2, btn.toggleState());
    }

}