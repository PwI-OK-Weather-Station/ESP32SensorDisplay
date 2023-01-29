#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include "ArduinoJson.h"
#include <esp_log.h>
LiquidCrystal_I2C lcd(0x26, 16, 2);


#define I2C_SDA 8
#define I2C_SCL 9

#define CLK 1
#define DT 2
#define SW 6

int currentStateCLK;
int lastStateCLK;
String currentDir ="";
unsigned long lastButtonPress = 0;


void setup() {
  Serial.begin(9600);
  Serial.setTimeout(30000);
  Wire.begin(I2C_SDA, I2C_SCL);

  lcd.init();
  lcd.backlight();

  pinMode(CLK,INPUT);
	pinMode(DT,INPUT);
	pinMode(SW, INPUT_PULLUP);
  lastStateCLK = digitalRead(CLK);
}

void loop() {
  


	currentStateCLK = digitalRead(CLK);

	if (currentStateCLK != lastStateCLK  && currentStateCLK == 1){

		if (digitalRead(DT) != currentStateCLK) {
			currentDir ="CCW";
		} else {
			currentDir ="CW";
		}

		Serial.print("Direction: ");
		Serial.print(currentDir);
	}

	lastStateCLK = currentStateCLK;

	int btnState = digitalRead(SW);

	if (btnState == LOW) {
		if (millis() - lastButtonPress > 50) {
			Serial.println("Button pressed!");
      lcd.setCursor(1, 0);
      lcd.print("LCD Dziala!");
      lcd.setCursor(2, 1);
      lcd.print("ESP32-C3 I2C");
		}
		lastButtonPress = millis();
	}

	// Put in a slight delay to help debounce the reading
	delay(1);
  
}
