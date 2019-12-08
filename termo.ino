#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TM1638plus.h>
#include "Display.h"

#define TEMP_PIN 3
#define SERVO_OPEN_PIN 5
#define SERVO_CLOSE_PIN 4
#define WINTER_PIN 11
#define ERROR_LED_PIN 13

#define SERVO_TIME 300	//seconds
#define REFRESH_TIME 20	//seconds

#define FURNACES_COUNT 2
int pumpPins[FURNACES_COUNT] = {6, 10};
DeviceAddress addresses[FURNACES_COUNT+1] = {0x00, 0x00, 0x00};

OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);
TM1638plus TMmodul(8, 9, 7);

Termostat termostat(sensors, SERVO_OPEN_PIN, SERVO_CLOSE_PIN, WINTER_PIN, ERROR_LED_PIN, SERVO_TIME, FURNACES_COUNT, pumpPins, addresses);
Display display(&TMmodul, &termostat);

unsigned long lastUpdateTime = 0;

void setup() {
	Serial.begin(9600);

	delay(500);

	updateTermo();
}

void loop() {
	if (millis() - lastUpdateTime >= REFRESH_TIME*1000){
		updateTermo();
	}
	
	display.doAction();

	if (termostat.getError()){
		Serial.print("error:");Serial.println(termostat.getError());
	}
}

void updateTermo(){
	display.setBlocked(1);
	termostat.update();
	lastUpdateTime = millis();
	display.setBlocked(0);
}
