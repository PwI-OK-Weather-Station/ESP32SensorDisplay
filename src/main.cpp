#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include "ArduinoJson.h"
#include <esp_log.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP3XX.h>
#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>

#include "config.h"
LiquidCrystal_I2C lcd(0x26, 16, 2);


#define I2C_SDA 8
#define I2C_SCL 9

#define CLK 1
#define DT 2
#define SW 6
	
#define DHT_TYPE DHT11
#define DHT_PIN 10
DHT dht(DHT_PIN, DHT_TYPE);

int currentStateCLK;
int lastStateCLK;
String currentDir ="";
unsigned long lastButtonPress = 0;
Adafruit_BMP3XX bmp;

const char* ssid = SSID_NAME;
const char* password = WIFI_PASSWORD;
String api = ENDPOINT;
String token = TOKEN;

enum Rotation {ClockWise, CounterClockWise, None};

Rotation checkEncoder(){
	currentStateCLK = digitalRead(CLK);
	if (currentStateCLK != lastStateCLK  && currentStateCLK == 1){

		if (digitalRead(DT) != currentStateCLK) {
			lastStateCLK = currentStateCLK;	
			return CounterClockWise;
		} else {
			lastStateCLK = currentStateCLK;	
			return ClockWise;
		}

	}

	lastStateCLK = currentStateCLK;	
	
	return None;
}
bool buttonPress(){
	if ( digitalRead(SW) == LOW) {
		if (millis() - lastButtonPress > 50) {
			return true;
		}
		lastButtonPress = millis();
	}
	return false;
}

bool ConnectWiFi(){
	int counter = 0;
	if(WiFi.status()!= WL_CONNECTED)
		WiFi.begin(ssid, password);
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("Connecting");
		while(WiFi.status() != WL_CONNECTED) {
			lcd.setCursor(counter, 1);
			counter++;
			delay(500);
			lcd.print("#");
			if(counter > 16){
				lcd.clear();
				lcd.print("Brak WiFi");
				delay(1000);
				return false;
			}
		}
		lcd.clear();
		lcd.print("WiFi connected");
		delay(1000);
	return true;
}


void setup() {
	Serial.begin(9600);
	Serial.setTimeout(30000);
	Wire.begin(I2C_SDA, I2C_SCL);

	lcd.init();
	lcd.backlight();
	
	bmp.begin_I2C();
	bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
	bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
	bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
	bmp.setOutputDataRate(BMP3_ODR_50_HZ);

	pinMode(CLK,INPUT);
	pinMode(DT,INPUT);
	pinMode(SW, INPUT_PULLUP);
	lastStateCLK = digitalRead(CLK);

	dht.begin();

	ConnectWiFi();	
}
int state = 0;
int page = 0;
int maxPage = 3;
bool update = true;
unsigned long timePage = 0;
unsigned long uploadTime = 0;
unsigned long syncTime = 0;
int indexes[MAX_DEVICES];
String names[MAX_DEVICES];
String tabNames[] = {"Temp.", "Press. ", "Humid.", "Temp. from Net", "Press. from Net", "Humid. from Net"};
String serverValues[] = {"", "", ""};
String sensorName = "";
int usedIndex = 0;

void displayMeasurement(){
	lcd.clear();
	lcd.print(tabNames[page]);
	lcd.setCursor(1,1);
	switch(page){
		case 0:{
			if (! bmp.performReading()) {
				Serial.println("Failed to perform reading");
			}
			lcd.print(String(bmp.temperature)+(char)223+"C");
			break;
		}

		case 1:{
			if (! bmp.performReading()) {
				Serial.println("Failed to perform reading");
			}
			lcd.print(String(bmp.pressure / 100.0)+"hPa");
			break;
		}

		case 2:{
			lcd.print(String(dht.readHumidity())+"%");
			break;
		}
		case 3:
		case 4:
		case 5:{
			lcd.print(serverValues[page-3]);
			break;
		}
	}
}

void loop() {
	if(uploadTime + 600000 <= millis()){
		if (bmp.performReading()) {
			if(ConnectWiFi()){
				HTTPClient http;
				String serverPath = api + "device/" + TOKEN;
				http.begin(serverPath.c_str());
				http.addHeader("Content-Type", "application/json");
				http.POST("{\"temperature\":"+String(bmp.temperature)+",\"pressure\":" + String(bmp.pressure / 100.0)
				+ ",\"humidity\":"+String(dht.readHumidity())+"}");
			}
		}
		uploadTime = millis();
		update = true;
	}
	bool btnState = buttonPress(); 
	Rotation rotation= checkEncoder();


	switch(rotation){
		case ClockWise:{
			page ++;
			if(page >= maxPage){
				page = 0;
			}
			update = true;
			timePage = millis();
			break;
		}
		case CounterClockWise:{
			page--;
			if(page <= 0){
				page = maxPage-1;
			}
			update = true;
			timePage = millis();
			break;
		}
	}
	if(timePage + 3000 < millis()){
		timePage = millis();
		page ++;
		if(page >= maxPage){
			page = 0;
		}
		update = true;
	}
	switch(state){
		case 0:{
			if(btnState){
				state = 1;
			}
			else if(update){
				displayMeasurement();
				update = false;
			}
			break;
		}
		case 1:{
			if(ConnectWiFi()){
				HTTPClient http;
				String serverPath = api + "mindevices";
				http.begin(serverPath.c_str());
				int httpResponseCode = http.GET();
				if(httpResponseCode == 200){
					String payload = http.getString();
					
					
					StaticJsonDocument<64*64> doc;

					DeserializationError error = deserializeJson(doc, payload);

					if (error) {
						Serial.print(F("deserializeJson() failed: "));
						Serial.println(error.f_str());
						return;
					}
					int count = 0;
					for (JsonObject device : doc["devices"].as<JsonArray>()) {
						indexes[count] = device["id"];
						const char* device_name = device["name"]; 
						names[count] = device_name;
						count ++;
						if(MAX_DEVICES <= count){
							break;
						}
						maxPage = count+1;

					}
					state = 2;
					break;
				}
				else{
					state = 0;
					break;
				}
			}
			else{
				state = 0;
			}
			update = true;
		}
		case 2:{
			if(btnState){
				state = 3;
			}
			else if(update){
				lcd.clear();
				lcd.print(names[page]);
				lcd.setCursor(0,1);
				lcd.print("ID: "+String(indexes[page]));
				update = false;
			}
			// maxPage = 3;
			// state = 0;
			break;
		}
		case 3:{
			if(ConnectWiFi()){
				HTTPClient http;
				String serverPath = api + "device/" + indexes[page] + "/1";
				Serial.println(serverPath);
				http.begin(serverPath.c_str());
				int httpResponseCode = http.GET();
				Serial.println(httpResponseCode);
				if(httpResponseCode == 200){
					String payload = http.getString();
					StaticJsonDocument<384> doc;
					DeserializationError error = deserializeJson(doc, payload);

					if (error) {
						Serial.print("deserializeJson() failed: ");
						Serial.println(error.c_str());
						return;
					}

					usedIndex = doc["id"]; // 7

					JsonObject measurements_0 = doc["measurements"][0];
					const char* humidity = measurements_0["humidity"]; // "28.200000"
					const char* pressure = measurements_0["pressure"]; // "961.100000"
					const char* temperature = measurements_0["temperature"]; // "25.600000"
					serverValues[0] = temperature;
					serverValues[1] = pressure;
					serverValues[2] = humidity;
					const char* name = doc["name"]; // "test1"
					sensorName = name;
					syncTime=millis();
					Serial.print("maxPage=6");
					maxPage = 6;
					state = 0;
				}
				else{
					maxPage = 3;
					state = 0;
				}
			}
			else{
				maxPage = 3;	
				state = 0;	
			}
			page = 0;
			break;
		}
	}

	delay(5);
	
  
}
