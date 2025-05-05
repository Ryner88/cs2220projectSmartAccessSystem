#include <LiquidCrystal.h>
#include <Servo.h>
#include <Keypad.h>
#include <EEPROM.h>

// ─── EEPROM ADDRESSES ─────────────────────────────────────────
#define EEPROM_USER_PIN_ADDR  0   // uses 0 and 1
#define EEPROM_ADMIN_PIN_ADDR 2   // uses 2 and 3

// ─── HARDWARE PINS ────────────────────────────────────────────
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
Servo        doorServo;
const int    SERVO_PIN  = A4;
const int    BUZZER_PIN = A5;

// ─── KEYPAD SETUP ─────────────────────────────────────────────
const byte ROWS = 4, COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},{'4','5','6','B'},
  {'7','8','9','C'},{'*','0','#','D'}
};
byte rowPins[ROWS] = {6, 8, 9, 10};
byte colPins[COLS] = {A3, A2, A1, A0};
Keypad keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ─── ACCESS CONTROL SETTINGS ─────────────────────────────────
int    userPin;                    // numeric
char   userPinStr[5];              // zero-padded ASCII
int    adminPin;                   // numeric
char   adminPinStr[5];             // ASCII
String inputBuffer = "";
int    tries = 0;

const int    MAX_TRIES     = 3;
const unsigned long LOCKOUT_MS    = 30UL*1000;
const unsigned long AUTO_LOCK_MS  = 10UL*1000;

unsigned long lockoutUntil = 0;
unsigned long autoLockTime = 0;

enum Mode { NORMAL, ADMIN_OLD, ADMIN_NEW, ADMIN_CONFIRM };
Mode mode = NORMAL;
char newPinStr[5];

// ─── EEPROM HELPERS ──────────────────────────────────────────
uint16_t readPinFromEEPROM(int addr) {
  uint16_t high = EEPROM.read(addr);
  uint16_t low  = EEPROM.read(addr + 1);
  return (high << 8) | low;
}
void writePinToEEPROM(int addr, uint16_t pin) {
  EEPROM.write(addr,     (pin >> 8) & 0xFF);
  EEPROM.write(addr + 1, pin & 0xFF);
}

// ─── MELODY & ANIMATION ──────────────────────────────────────
int miMelody[]   = {329,329,329,262,329,329,329,262,329,329,329,262,196,329,329,329,262};
int miDur[]      = {200,200,200,600,200,200,200,600,200,200,200,600,600,200,200,200,600};
int marioMelody[]= {659,659,   0,659,   0,523,659,784,   0,392};
int marioDur[]   = {125,125,125,125,125,125,125,125,125,125};
int marchMelody[]= {440,440,440,349,523,440,349,523,440};
int marchDur[]   = {375,125,500,375,125,500,375,125,500};

void playMelody(int *mel, int *dur, int len) {
  for (int i = 0; i < len; i++) {
    if (mel[i] == 0) noTone(BUZZER_PIN);
    else tone(BUZZER_PIN, mel[i], dur[i]);
    delay(dur[i] * 1.2);
  }
  noTone(BUZZER_PIN);
}

void customUnlockAnimation() {
  // cinematic servo sweep + rising whoosh
  for (int pos = 0; pos <= 90; pos += 2) {
    doorServo.write(pos);
    tone(BUZZER_PIN, 200 + pos * 4, 15);
    delay(20);
  }
  delay(200);
  // random theme
  switch (random(3)) {
    case 0: playMelody(miMelody,    miDur,    sizeof(miMelody)/sizeof(int));    break;
    case 1: playMelody(marioMelody, marioDur, sizeof(marioMelody)/sizeof(int)); break;
    case 2: playMelody(marchMelody, marchDur, sizeof(marchMelody)/sizeof(int)); break;
  }
  noTone(BUZZER_PIN);
}

void warningBeep() {
  tone(BUZZER_PIN, 300, 200);
  delay(250);
  tone(BUZZER_PIN, 300, 200);
}

// ─── PIN GENERATION ──────────────────────────────────────────
void generateUserPin() {
  randomSeed(analogRead(A0));
  userPin = random(1000, 10000);
  snprintf(userPinStr, sizeof(userPinStr), "%04d", userPin);
}

// ─── SETUP ───────────────────────────────────────────────────
void setup() {
  lcd.begin(16,2);
  Serial.begin(9600);

  // — Load or initialize Admin PIN —
  uint16_t storedAdmin = readPinFromEEPROM(EEPROM_ADMIN_PIN_ADDR);
  if (storedAdmin >= 1000 && storedAdmin < 10000) {
    adminPin = storedAdmin;
  } else {
    adminPin = 1234;
    writePinToEEPROM(EEPROM_ADMIN_PIN_ADDR, adminPin);
  }
  snprintf(adminPinStr, sizeof(adminPinStr), "%04d", adminPin);

  // — Load or generate User PIN —
  uint16_t storedUser = readPinFromEEPROM(EEPROM_USER_PIN_ADDR);
  if (storedUser >= 1000 && storedUser < 10000) {
    userPin = storedUser;
  } else {
    generateUserPin();
    writePinToEEPROM(EEPROM_USER_PIN_ADDR, userPin);
  }
  snprintf(userPinStr, sizeof(userPinStr), "%04d", userPin);

  // Show User PIN once
  lcd.print("Your PIN is:");
  lcd.setCursor(0,1);
  lcd.print(userPinStr);
  delay(5000);

  // Prompt entry
  lcd.clear();
  lcd.print("Enter PIN:");
  lcd.setCursor(0,1);

  // Init servo + buzzer
  doorServo.attach(SERVO_PIN);
  doorServo.write(0);
  pinMode(BUZZER_PIN, OUTPUT);
}

// ─── MAIN LOOP ───────────────────────────────────────────────
void loop() {
  unsigned long now = millis();

  // Auto-relock
  if (autoLockTime && now >= autoLockTime) {
    doorServo.write(0);
    lcd.clear(); lcd.print("Door Locked");
    delay(2000);
    mode = NORMAL;
    tries = 0;
    autoLockTime = 0;
    lcd.clear(); lcd.print("Enter PIN:"); lcd.setCursor(0,1);
    return;
  }

  // Lock-out
  if (lockoutUntil) {
    if (now < lockoutUntil) return;
    lockoutUntil = 0;
    tries = 0;
    mode = NORMAL;
    lcd.clear(); lcd.print("Enter PIN:"); lcd.setCursor(0,1);
  }

  // Read key
  char k = keypad.getKey();
  if (!k) return;

  // Clear entry
  if (k == '*') {
    inputBuffer = "";
    lcd.setCursor(0,1); lcd.print("    "); lcd.setCursor(0,1);
    return;
  }

  // Enter Admin Mode
  if (mode == NORMAL && k == '#') {
    mode = ADMIN_OLD;
    inputBuffer = "";
    lcd.clear(); lcd.print("Enter Admin PIN"); lcd.setCursor(0,1);
    return;
  }

  // Only digits
  if (k < '0' || k > '9') return;
  inputBuffer += k;
  lcd.print(k);

  // On 4 digits, dispatch by mode
  if (inputBuffer.length() == 4) {
    switch (mode) {
      // ─ NORMAL PIN CHECK & UNLOCK ─────────
      case NORMAL:
        if (inputBuffer == userPinStr) {
          lcd.clear(); lcd.print("Access Granted");
          customUnlockAnimation();
          autoLockTime = now + AUTO_LOCK_MS;
        } else {
          tries++;
          warningBeep();
          lcd.clear(); lcd.print("Access Denied");
          delay(2000);
          if (tries >= MAX_TRIES) {
            lockoutUntil = now + LOCKOUT_MS;
            lcd.clear();
            lcd.print("Too many attempts");
            lcd.setCursor(0,1); lcd.print("Try again 30s");
          } else {
            lcd.clear(); lcd.print("Enter PIN:"); lcd.setCursor(0,1);
          }
        }
        break;

      // ─ ADMIN: VERIFY OLD ADMIN PIN ────────
      case ADMIN_OLD:
        if (inputBuffer == adminPinStr) {
          mode = ADMIN_NEW;
          lcd.clear(); lcd.print("Enter New PIN"); lcd.setCursor(0,1);
        } else {
          warningBeep();
          lcd.clear(); lcd.print("Bad Admin PIN");
          delay(2000);
          mode = NORMAL;
          lcd.clear(); lcd.print("Enter PIN:"); lcd.setCursor(0,1);
        }
        break;

      // ─ ADMIN: ENTER NEW USER PIN ──────────
      case ADMIN_NEW:
        strncpy(newPinStr, inputBuffer.c_str(), 5);
        mode = ADMIN_CONFIRM;
        lcd.clear(); lcd.print("Confirm New PIN"); lcd.setCursor(0,1);
        break;

      // ─ ADMIN: CONFIRM & STORE USER PIN ────
      case ADMIN_CONFIRM:
        if (inputBuffer == newPinStr) {
          // Update in RAM & EEPROM
          strncpy(userPinStr, newPinStr, 5);
          userPin = atoi(userPinStr);
          writePinToEEPROM(EEPROM_USER_PIN_ADDR, userPin);

          lcd.clear(); lcd.print("PIN Changed!");
          delay(2000);
        } else {
          warningBeep();
          lcd.clear(); lcd.print("Mismatch!");
          delay(2000);
        }
        mode = NORMAL;
        tries = 0;
        lcd.clear(); lcd.print("Enter PIN:"); lcd.setCursor(0,1);
        break;
    }
    // Reset for next
    inputBuffer = "";
    lcd.setCursor(0,1); lcd.print("    "); lcd.setCursor(0,1);
  }
}