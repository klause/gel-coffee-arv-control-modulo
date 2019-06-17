
// group 1 button/led pins
byte grp1OneShortPin = 1; // one short espresso
byte grp1OneLongPin = 2; // one long espresso
byte grp1TwoShortPin = 3; // tow short espressos
byte grp1TwoLongPin = 4; // tow long espressos
byte grp1ContinuosPin = 5; // continuos brewing

// group 2 button/led pins
byte grp2OneShortPin = 6; // one short espresso
byte grp2OneLongPin = 7; // one long espresso
byte grp2TwoShortPin = 8; // tow short espressos
byte grp2TwoLongPin = 9; // tow long espressos
byte grp2ContinuosPin = 10; // continuos brewing

byte buttonPins[10] = {1,2,3,4,5,6,7,8,9,10};
bool buttonEnabledArray[10] = {0,0,0,0,0,0,0,0,0,0};
bool buttonLastValArray[10] = {HIGH,HIGH,HIGH,HIGH,HIGH,HIGH,HIGH,HIGH,HIGH,HIGH};

bool buttonEnabled;

byte grp1EnabledButton = -1;
byte grp2EnabledButton = -1;

long lastPressedTime = -1;

void setup() {
  // Initialize a serial connection for reporting values to the host
  Serial.begin(38400);
  Serial.println("Initializing coffee machine...");

  pinMode(grp1OneShortPin, INPUT_PULLUP);
  pinMode(grp1OneLongPin, INPUT_PULLUP);
  pinMode(grp1TwoShortPin, INPUT_PULLUP);
  pinMode(grp1TwoLongPin, INPUT_PULLUP);
  pinMode(grp1ContinuosPin, INPUT_PULLUP);

  pinMode(grp2OneShortPin, INPUT_PULLUP);
  pinMode(grp2OneLongPin, INPUT_PULLUP);
  pinMode(grp2TwoShortPin, INPUT_PULLUP);
  pinMode(grp2TwoLongPin, INPUT_PULLUP);
  pinMode(grp2ContinuosPin, INPUT_PULLUP);

  Serial.println("Initialization complete. ");
}

void loop() {

  for (byte i = 1; i < 10; i++) {

    // check if button is pressed
    bool currentButtonVal = checkButton(i);

  }

//  turnOnLed(grp1EnabledButton, currentButtonVal);
//  turnOnLed(grp2EnabledButton, currentButtonVal);

}

bool checkButton(byte buttonPin) {

  bool buttonEnabled = buttonEnabledArray[buttonPin-1];

  if (buttonEnabled) {
    pinMode(buttonPin, INPUT_PULLUP);
  }

  bool buttonVal = digitalRead(buttonPin);

  if (buttonVal == LOW && buttonLastValArray[buttonPin-1] == HIGH && (millis() - lastPressedTime) > 500) {
    lastPressedTime = millis();
    Serial.println("Button " + buttonPin + " pressed");
    buttonEnabledArray[buttonPin-1] = !buttonEnabled;
    if (buttonPin <= 5) {
      grp1EnabledButton = !buttonEnabled ? buttonPin : -1;
      commandGroup(1, buttonEnabled, buttonPin);
    } else {
      grp2EnabledButton = !buttonEnabled ? buttonPin : -1;
    }
  }

  buttonLastValArray[buttonPin-1] = buttonVal;
  return buttonVal;
}

void turnOnLed(byte buttonPin, bool currentButtonVal) {

  bool buttonEnabled = buttonEnabledArray[buttonPin-1];

  // turn LED on if button is active and button is released
  if (buttonEnabled && currentButtonVal == HIGH) {
    digitalWrite(buttonPin, LOW);
    pinMode(buttonPin, OUTPUT);
  }

}

void commandGroup(byte group, bool enable, byte buttonPin) {

      if (enable) {
        Serial.println("Start brewing on group " + group);
      } else {
        Serial.println("Stop brewing on group " + group);
      }

}
