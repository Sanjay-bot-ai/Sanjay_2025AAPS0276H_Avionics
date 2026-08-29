#include <Adafruit_LiquidCrystal.h>

/*
 * SEDS BPHC - Avionics Induction Task 2: Keeping Watch Over Odysseus
 * Name: Sanjay D
 * ID Number: 2025AAPS0288H
 */


// Tinkercad's native I2C LCD initialization (0 = standard I2C communication)
Adafruit_LiquidCrystal lcd(0);

// Pin Definitions matching your schematic
const int BUTTON_PIN = 7;
const int TRIG_PIN   = 9;
const int ECHO_PIN   = 10;
const int LDR_PIN    = A0;
const int LED_PIN    = 6;
const int BUZZER_PIN = 8;

// Thresholds
const int LIGHT_THRESHOLD = 512;           // Below half = Storm[cite: 1]
const float DISTANCE_THRESHOLD = 100.0;    // Distance < 100cm = Charybdis[cite: 1]
const unsigned long DANGER_LIMIT_MS = 5000; // 5s continuous danger = Wrecked[cite: 1]

enum State {
  OPEN_SEA,
  ANCHOR_DROPPED,
  STORM,
  CHARYBDIS,
  WRECKED
};

State currentState = OPEN_SEA;
State previousState = OPEN_SEA;

bool anchorDropped = false;
bool lastButtonState = HIGH;
unsigned long dangerStartTime = 0;
unsigned long lastBlinkTime = 0;
bool ledState = LOW;

float getDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return 999.0;
  return duration * 0.0343 / 2.0;
}

void printState(const char* stateText) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ODYSSEUS STATUS:");
  lcd.setCursor(0, 1);
  lcd.print(stateText);
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Initialize Native Tinkercad LCD
  lcd.begin(16, 2);
  lcd.setBacklight(1);
  printState("OPEN SEA");
}

void loop() {
  // If WRECKED, lock permanently[cite: 1]
  if (currentState == WRECKED) {
    digitalWrite(LED_PIN, LOW);
    noTone(BUZZER_PIN);
    return;
  }

  // 1. Button Handling (Debounced Toggle)
  bool currentButton = digitalRead(BUTTON_PIN);
  if (lastButtonState == HIGH && currentButton == LOW) {
    delay(40);
    if (digitalRead(BUTTON_PIN) == LOW) {
      anchorDropped = !anchorDropped;
      if (anchorDropped) {
        currentState = ANCHOR_DROPPED;
        dangerStartTime = 0;
      } else {
        currentState = OPEN_SEA;
      }
    }
  }
  lastButtonState = currentButton;

  // 2. Read Sensors
  int lightLevel = analogRead(LDR_PIN);
  float distance = getDistanceCM();

  bool isStorm = (lightLevel < LIGHT_THRESHOLD);
  bool isCharybdis = (distance < DISTANCE_THRESHOLD);

  // 3. State Machine Logic[cite: 1]
  if (!anchorDropped) {
    if (currentState == OPEN_SEA) {
      if (isStorm) {
        currentState = STORM;
        dangerStartTime = millis();
      } else if (isCharybdis) {
        currentState = CHARYBDIS;
        dangerStartTime = millis();
      }
    }
    else if (currentState == STORM) {
      if (!isStorm) {
        currentState = OPEN_SEA;
        dangerStartTime = 0;
      } else if (millis() - dangerStartTime >= DANGER_LIMIT_MS) {
        currentState = WRECKED;
      }
    }
    else if (currentState == CHARYBDIS) {
      if (!isCharybdis) {
        currentState = OPEN_SEA;
        dangerStartTime = 0;
      } else if (millis() - dangerStartTime >= DANGER_LIMIT_MS) {
        currentState = WRECKED;
      }
    }
  }

  // 4. Actuators
  if (currentState == STORM) {
    if (millis() - lastBlinkTime >= 250) {
      lastBlinkTime = millis();
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState);
    }
    noTone(BUZZER_PIN);
  } else if (currentState == CHARYBDIS) {
    tone(BUZZER_PIN, 1000);
    digitalWrite(LED_PIN, LOW);
  } else {
    digitalWrite(LED_PIN, LOW);
    noTone(BUZZER_PIN);
  }

  // 5. Update Screen Display on State Change
  if (currentState != previousState) {
    switch (currentState) {
      case OPEN_SEA:       printState("OPEN SEA"); break;
      case ANCHOR_DROPPED: printState("ANCHOR DROPPED"); break;
      case STORM:          printState("STORM"); break;
      case CHARYBDIS:      printState("CHARYBDIS"); break;
      case WRECKED:        printState("WRECKED"); break;
    }
    previousState = currentState;
  }

  delay(20);
}