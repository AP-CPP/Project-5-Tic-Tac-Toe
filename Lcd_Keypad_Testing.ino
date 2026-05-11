#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

#define SDA_PIN 13
#define SCL_PIN 14
LiquidCrystal_I2C lcd(0x27, 16, 2);

const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {15, 27, 26, 25};
byte colPins[COLS] = {2, 21, 22, 23};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

bool i2cAddrTest(uint8_t addr) {
  Wire.beginTransmission(addr);
  return (Wire.endTransmission() == 0);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\nLCD + Keypad test starting...");

  Wire.begin(SDA_PIN, SCL_PIN);
  if (!i2cAddrTest(0x27) && i2cAddrTest(0x3F)) {
    lcd = LiquidCrystal_I2C(0x3F, 16, 2);
  }

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Press a key:");
  lcd.setCursor(0, 1);
  lcd.print("(waiting...)");

  Serial.println("Ready.");
}

void loop() {
  char key = keypad.getKey();
  if (key) {
    Serial.print("Key: ");
    Serial.println(key);

    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print("Last key: ");
    lcd.print(key);
  }
}
