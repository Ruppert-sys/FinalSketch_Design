#include "HX711.h"
#include <Stepper.h>

// --- Load Cell Configuration ---
const int LOADCELL_DOUT_PIN = A4;
const int LOADCELL_SCK_PIN = A5;
const int loadLedPin = 7;
HX711 scale;

const int buzzerPin = 9;

// --- Stepper Motor Configuration ---
const float STEPS_PER_REVOLUTION = 32;
const float GEAR_REDUCTION = 64;
const float STEPS_PER_OUT_REV = STEPS_PER_REVOLUTION * GEAR_REDUCTION;
const float DEGREES = 60.0;
const int STEPS_FOR_30_DEGREES = (DEGREES / 360.0) * STEPS_PER_OUT_REV;
const int stepperButtonPin = 2;
Stepper steppermotor(STEPS_PER_REVOLUTION, 10, 12, 11, 13);
bool stepperCurrent = LOW;
bool stepperLast = LOW;

// --- Heartbeat LED ---
const int heartbeatLedPin = 0;
unsigned long previousMillis = 0;
const long heartbeatInterval = 500;
bool heartbeatLedState = false;

// --- Light Relays ---
const int lightButtonPin1 = 3;
const int relayPin1 = 5;
const int lightButtonPin2 = 4;
const int relayPin2 = 6;
bool lightButtonState1 = LOW;
bool lastLightButtonState1 = LOW;
bool lightButtonState2 = LOW;
bool lastLightButtonState2 = LOW;

// --- Binary Countdown Timer ---
const int binaryLedPins[4] = {A0, A1, A2, A3};
int count = 9;
bool timerRunning = false;
unsigned long lastTick = 0;
bool buttonPreviouslyPressed = false;
bool buzzerActive = false;
bool buzzerState = false;
unsigned long lastBuzzerToggle = 0;
const unsigned long buzzerInterval = 5000; // 5 sec

void setup() {
  pinMode(0, OUTPUT);
  digitalWrite(0, HIGH);

  // Load Cell
  pinMode(loadLedPin, OUTPUT);
  digitalWrite(loadLedPin, LOW);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale();
  scale.tare();

  // Stepper
  pinMode(stepperButtonPin, INPUT_PULLUP);
  steppermotor.setSpeed(800);

  // Heartbeat LED
  pinMode(heartbeatLedPin, OUTPUT);

  // Light toggle relays
  pinMode(lightButtonPin1, INPUT_PULLUP);
  pinMode(relayPin1, OUTPUT);
  digitalWrite(relayPin1, LOW);

  pinMode(lightButtonPin2, INPUT_PULLUP);
  pinMode(relayPin2, OUTPUT);
  digitalWrite(relayPin2, LOW);

  // Binary LEDs
  for (int i = 0; i < 4; i++) {
    pinMode(binaryLedPins[i], OUTPUT);
    digitalWrite(binaryLedPins[i], LOW);
  }

  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  // Start in 1111 state (15)
  displayBinary(15);
  buttonPreviouslyPressed = digitalRead(lightButtonPin1) == LOW;
}

void loop() {
  // Load Cell
  if (scale.is_ready()) {
    float weight = scale.get_units(5);
    digitalWrite(loadLedPin, weight > 2000 ? HIGH : LOW);
  }

  // Stepper
  stepperCurrent = debounce(stepperLast, stepperButtonPin);
  if (stepperCurrent == HIGH && stepperLast == LOW) {
    rotateClockwise30Degrees();
  }
  stepperLast = stepperCurrent;

  // Heartbeat
  if (millis() - previousMillis >= heartbeatInterval) {
    previousMillis = millis();
    heartbeatLedState = !heartbeatLedState;
    digitalWrite(heartbeatLedPin, heartbeatLedState);
  }

  // Relay buttons
  lightButtonState1 = digitalRead(lightButtonPin1);
  if (lightButtonState1 == LOW && lastLightButtonState1 == HIGH) {
    digitalWrite(relayPin1, !digitalRead(relayPin1));
    delay(100);
  }
  lastLightButtonState1 = lightButtonState1;

  lightButtonState2 = digitalRead(lightButtonPin2);
  if (lightButtonState2 == LOW && lastLightButtonState2 == HIGH) {
    digitalWrite(relayPin2, !digitalRead(relayPin2));
    delay(100);
  }
  lastLightButtonState2 = lightButtonState2;

  // Countdown logic
  bool buttonPressed = digitalRead(lightButtonPin1) == LOW;
  if (buttonPressed && !buttonPreviouslyPressed) {
    delay(20);
    if (digitalRead(lightButtonPin1) == LOW) {
      if (timerRunning || buzzerActive) {
        // Stop everything and clear
        timerRunning = false;
        buzzerActive = false;
        buzzerState = false;
        digitalWrite(buzzerPin, LOW);
        clearBinaryLEDs(); // All OFF
      } else {
        // Start countdown
        count = 9;
        timerRunning = true;
        lastTick = millis();
        displayBinary(count);
      }
    }
  }
  buttonPreviouslyPressed = buttonPressed;

  if (timerRunning && millis() - lastTick >= 60000UL) {
    lastTick = millis();
    count--;
    if (count > 0) {
      displayBinary(count);
    } else {
      // End countdown
      timerRunning = false;
      displayBinary(15); // 1111
      buzzerActive = true;
      buzzerState = true;
      digitalWrite(buzzerPin, HIGH);
      lastBuzzerToggle = millis();
    }
  }

  if (buzzerActive && millis() - lastBuzzerToggle >= buzzerInterval) {
    buzzerState = !buzzerState;
    digitalWrite(buzzerPin, buzzerState ? HIGH : LOW);
    lastBuzzerToggle = millis();
  }

  delay(10);
}

bool debounce(bool lastState, int pin) {
  bool currentState = digitalRead(pin);
  if (lastState != currentState) {
    delay(5);
    currentState = digitalRead(pin);
  }
  return currentState;
}

void rotateClockwise30Degrees() {
  steppermotor.step((int)STEPS_FOR_30_DEGREES);
}

void displayBinary(int value) {
  for (int i = 0; i < 4; i++) {
    digitalWrite(binaryLedPins[i], (value >> i) & 1);
  }
}

void clearBinaryLEDs() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(binaryLedPins[i], LOW);
  }
}
