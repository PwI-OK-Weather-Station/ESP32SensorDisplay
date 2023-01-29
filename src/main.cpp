#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <esp_log.h>
LiquidCrystal_I2C lcd(0x26, 16, 2);


#define I2C_SDA 8
#define I2C_SCL 9

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(30000);
  Wire.begin(I2C_SDA, I2C_SCL);

  lcd.init();
  lcd.backlight();
}

void loop() {
  delay(10000);
  lcd.setCursor(1, 0);
  lcd.print("LCD Dziala!");
  lcd.setCursor(2, 1);
  lcd.print("ESP32-C3 I2C");

  
}
